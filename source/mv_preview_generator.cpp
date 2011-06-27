#include "mv_preview_generator.h"

#include <memory>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <windows.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}
#include "jpeg_tool.h"

using std::list;
using std::unique_ptr;
using std::string;
using std::wstring;
using std::ofstream;
using boost::filesystem3::path;
using boost::filesystem3::exists;
using boost::filesystem3::directory_iterator;
using boost::filesystem3::is_directory;
using boost::filesystem3::is_regular_file;
using boost::filesystem3::filesystem_error;
using boost::algorithm::iequals;

namespace {
string WideCharToMultiByte(const wstring& from)
{
    int size = ::WideCharToMultiByte(CP_ACP, 0, from.c_str(), -1, NULL, 0, NULL,
                                     NULL);
    if (1 > size)
        return string();

    unique_ptr<char[]> dst(new char[size]);
    ::WideCharToMultiByte(CP_ACP, 0, from.c_str(), -1, dst.get(), size, NULL,
                          NULL);
    return string(dst.get());
}

void SaveToBMPFile(const path& bmpPath, AVFrame* frame, int width, int height)
{
    assert(frame);
    const int dataSize = ((width * 3 + 3) / 4) * 4 * height;

    BITMAPFILEHEADER fileHeader = {0};
    BITMAPINFOHEADER infoHeader = {0};

    fileHeader.bfType = 'MB';
    fileHeader.bfSize = dataSize + sizeof(fileHeader) + sizeof(infoHeader);
    fileHeader.bfOffBits = sizeof(fileHeader);

    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = width;
    infoHeader.biHeight = -height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = dataSize;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    ofstream bmpFile(bmpPath.wstring(), std::ios_base::binary);
    bmpFile.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    bmpFile.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    for (int y = 0; y < height; ++y) {
        char* data = reinterpret_cast<char*>(
            frame->data[0] + y * frame->linesize[0]);

        for (int x = 0; x < frame->linesize[0] - 3; x += 3)
            std::swap(data[x], data[x + 1]);

        bmpFile.write(data, width * 3);
    }
}

void ReportDone(GeneratorCallback* c)
{
    if (c)
        c->Done();
}
}

MvPreviewGenerator::MvPreviewGenerator(GeneratorCallback* callback)
    : callback_(callback)
    , cancelFlag_(false)
{
}

MvPreviewGenerator::~MvPreviewGenerator()
{
}

void MvPreviewGenerator::StartGenerating(const path& mvPath, int64 time,
                                         bool recursive)
{
    unique_ptr<GeneratorCallback, void (*)(GeneratorCallback*)> autoReportDone(
        callback_, ReportDone);
    if (callback_)
        callback_->Initializing(this, CalculateNumFiles(mvPath, recursive));

    if (!exists(mvPath))
        return;

    av_register_all();
    avcodec_register_all();

    if (is_regular_file(mvPath)) {
        if (iequals(mvPath.extension().string(), L".mkv"))
            Generate(mvPath, time);

        return;
    }

    list<path> pending;
    pending.push_back(mvPath);
    do {
        const path& t = pending.front();
        SearchAndGenerate(mvPath, &pending, time, recursive);
        pending.pop_front();
    } while (pending.size());
}

int MvPreviewGenerator::CalculateNumFiles(const path& mvPath, bool recursive)
{
    if (!exists(mvPath))
        return 0;

    if (is_regular_file(mvPath)) {
        if (iequals(mvPath.extension().string(), L".mkv"))
            return 1;

        return 0;
    }

    int fileCount = 0;
    try {
        for (directory_iterator i(mvPath), e = directory_iterator(); i != e;
            ++i) {
            if (is_directory(i->path())) {
                if (recursive)
                    fileCount += CalculateNumFiles(i->path(), recursive);

                continue;
            }

            if (!iequals(i->path().extension().string(), L".mkv"))
                continue;

            fileCount++;
        }
    } catch (const filesystem_error& ex) {
        (ex);
    }

    return fileCount;
}

void MvPreviewGenerator::SearchAndGenerate(const path& mvPath,
                                           list<path>* pending, int64 time,
                                           bool recursive)
{
    try {
        for (directory_iterator i(mvPath), e = directory_iterator(); i != e;
            ++i) {
            if (is_directory(i->path())) {
                if (recursive)
                    pending->push_back(i->path());

                continue;
            }

            if (!iequals(i->path().extension().string(), L".mkv"))
                continue;

            Generate(i->path(), time);
        }
    } catch (const filesystem_error& ex) {
        (ex);
    }
}

void MvPreviewGenerator::Generate(const path& mvPath, int64 time)
{
    assert(is_regular_file(mvPath));
    assert(iequals(mvPath.extension().string(), L".mkv"));

    if (cancelFlag_)
        return;

    AVFormatContext* media;
    if (av_open_input_file(&media,
                           WideCharToMultiByte(mvPath.wstring()).c_str(), NULL,
                           0, NULL))
        return;

    unique_ptr<AVFormatContext, void (*)(AVFormatContext*)> autoRelease(
        media, av_close_input_file);

    if (av_find_stream_info(media) < 0)
        return;

    // Get video stream.
    int videoStreamIndex = -1;
    for (int i = 0; i < static_cast<int>(media->nb_streams); ++i) {
        if (AVMEDIA_TYPE_VIDEO == media->streams[i]->codec->codec_type) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex < 0)
        return;

    // Initialize decoding context.
    AVCodec* codec = avcodec_find_decoder(
        media->streams[videoStreamIndex]->codec->codec_id);
    if (!codec)
        return;

    unique_ptr<AVCodecContext, int (*)(AVCodecContext*)> codecCont(
        media->streams[videoStreamIndex]->codec, avcodec_close);

    if (avcodec_open(codecCont.get(), codec) < 0)
        return;

    // Seek to the specified position.
    if (av_seek_frame(media, videoStreamIndex, time, 0) < 0)
        return;

    unique_ptr<AVFrame, void (*)(void*)> frame(avcodec_alloc_frame(),
                                               av_free);
    do {
        AVPacket packet = {0};
        if (av_read_frame(media, &packet) < 0)
            return;

        if (packet.stream_index != videoStreamIndex)
            continue;
        
        // Decode.
        int frameFinished;
        int bytesDecoded = avcodec_decode_video2(codecCont.get(), frame.get(),
                                                 &frameFinished, &packet);
        av_free_packet(&packet);
        if (bytesDecoded < 0)
            return;

        if (frameFinished && bytesDecoded)
            break;
    } while (!cancelFlag_);

    // Initialize RGB format frame.
    unique_ptr<AVFrame, void (*)(void*)> frameRGB(avcodec_alloc_frame(),
                                                  av_free);
    const int bufSize = avpicture_get_size(PIX_FMT_RGB24, codecCont->width,
                                           codecCont->height);
    avpicture_fill(reinterpret_cast<AVPicture*>(frameRGB.get()),
                    reinterpret_cast<uint8_t*>(av_malloc(bufSize)),
                    PIX_FMT_RGB24, codecCont->width, codecCont->height);

    SwsContext* scaleCont = sws_getContext(codecCont->width,
                                           codecCont->height,
                                           codecCont->pix_fmt,
                                           codecCont->width,
                                           codecCont->height,
                                           PIX_FMT_RGB24, SWS_BICUBIC,
                                           NULL, NULL, NULL);
    if (!scaleCont)
        return;

    sws_scale(scaleCont, frame.get()->data, frame.get()->linesize, 0,
              codecCont->height, frameRGB.get()->data,
              frameRGB.get()->linesize);

    path previewPath(mvPath);
    previewPath.replace_extension(L".bmp");
//     SaveToBMPFile(previewPath, frameRGB.get(), codecCont->width,
//                   codecCont->height);
    previewPath.replace_extension(L".jpg");
    Jpeg::SaveToJPEGFile(previewPath, frameRGB.get(), codecCont->width,
                         codecCont->height);
    if (callback_)
        callback_->Progress(mvPath.wstring());
}
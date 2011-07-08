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
#include "util.h"

#import "msado15.dll" no_namespace rename("EOF", "ADOEOF")
#include "msado15.tlh"

using std::list;
using std::unique_ptr;
using std::string;
using std::wstring;
using std::ofstream;
using boost::filesystem3::path;
using boost::filesystem3::directory_iterator;
using boost::filesystem3::exists;
using boost::filesystem3::is_directory;
using boost::filesystem3::is_regular_file;
using boost::filesystem3::filesystem_error;
using boost::algorithm::iequals;
using boost::algorithm::replace_head;

namespace {
void FreeAVFrame(AVFrame* f)
{
    if (f) {
        avpicture_free(reinterpret_cast<AVPicture*>(f));
        av_free(f);
    }
}

void SaveToBMPFile(const path& bmpPath, AVFrame* frame)
{
    assert(frame);
    const int dataSize = ((frame->width * 3 + 3) / 4) * 4 * frame->height;

    BITMAPFILEHEADER fileHeader = {0};
    BITMAPINFOHEADER infoHeader = {0};

    fileHeader.bfType = 'MB';
    fileHeader.bfSize = dataSize + sizeof(fileHeader) + sizeof(infoHeader);
    fileHeader.bfOffBits = sizeof(fileHeader);

    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = frame->width;
    infoHeader.biHeight = -frame->height;
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

    for (int y = 0; y < frame->height; ++y) {
        char* data = reinterpret_cast<char*>(
            frame->data[0] + y * frame->linesize[0]);

        for (int x = 0; x < frame->linesize[0] - 3; x += 3)
            std::swap(data[x], data[x + 1]);

        bmpFile.write(data, frame->width * 3);
    }
}

void ReportDone(GeneratorCallback* c)
{
    if (c)
        c->Done();
}

void UnifySlash(wstring* s)
{
    assert(s);
    if (!s->empty()) {
        auto i = s->back();
        if (('/' != i) && ('\\' != i))
            *s += '/';
    }
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

void MvPreviewGenerator::StartGenerating(const path& mvPath,
                                         const path& previewPath, int64 time,
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
            Generate(mvPath, previewPath, time);

        return;
    }

    list<path> pending;
    pending.push_back(mvPath);
    do {
        const path& t = pending.front();
        SearchAndGenerate(mvPath, previewPath, &pending, time, recursive);
        pending.pop_front();
    } while (pending.size() && !cancelFlag_);
}

void MvPreviewGenerator::StartGenerating(const wstring& dbServer,
                                         const wstring& userName,
                                         const wstring& password,
                                         const path& previewPath, int64 time)
{
    unique_ptr<GeneratorCallback, void (*)(GeneratorCallback*)> autoReportDone(
        callback_, ReportDone);

    av_register_all();
    avcodec_register_all();

    _ConnectionPtr connection;
    _RecordsetPtr records;
    try {
        HRESULT r = connection.CreateInstance(__uuidof(Connection));
        if (!connection)
            return;

        wstring s = L"Driver=SQL Server;Server=" + dbServer +
            L";Database=KG_MediaInfo;Uid=" + userName + L";Pwd=" +
            password + L";";
        _bstr_t forConn(s.c_str());
        connection->Open(forConn, "", "", adConnectUnspecified);
        records.CreateInstance(__uuidof(Recordset));

        do {
            if (!records)
                break;

            records->PutCacheSize(200000);
            _bstr_t forExec(L"SELECT 文件路径, 旧哈希值 FROM "
                            L"CHECKED_ENCODE_FILE_INFO");
            records->Open(forExec,
                          _variant_t(static_cast<IDispatch*>(connection), true),
                          adOpenStatic, adLockReadOnly, adCmdText);

            if (callback_)
                callback_->Initializing(this, records->GetRecordCount());

            while (!records->ADOEOF && !cancelFlag_) {
                _variant_t index;
                index.vt = VT_I2;
                index.iVal = 0;
                FieldPtr field0 = records->GetFields()->GetItem(&index);
                _variant_t fileName = field0->GetValue();
                index.iVal = 1;
                FieldPtr field1 = records->GetFields()->GetItem(&index);
                _variant_t md5 = field1->GetValue();
                if ((VT_NULL != fileName.vt) && (VT_NULL != md5.vt)) {
                    wstring url =
                        static_cast<wchar_t*>(static_cast<_bstr_t>(fileName));
                    if (url.length() > 27) {
                        if ('_' == url[26])
                            replace_head(url, 15, L"h:");
                        else
                            replace_head(url, 15, L"d:");

                        wstring s = previewPath.wstring();
                        UnifySlash(&s);
                        s += static_cast<wchar_t*>(static_cast<_bstr_t>(md5));
                        s += L".jpg";
                        if (!exists(s))
                            Generate(url, s, time);
                        else if (callback_)
                            callback_->Progress(url);
                    }
                }

                records->MoveNext();
            }
        } while (0);
    } catch (_com_error& e) {
        (e);
    }

    if (connection)
        if (connection->State == adStateOpen)
            connection->Close();

    if (records)
        if (records->State == adStateOpen)
            records->Close();
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
                                           const path& previewPath, 
                                           list<path>* pending, int64 time,
                                           bool recursive)
{
    try {
        for (directory_iterator i(mvPath), e = directory_iterator();
            ((i != e) && !cancelFlag_); ++i) {
            if (is_directory(i->path())) {
                if (recursive)
                    pending->push_back(i->path());

                continue;
            }

            if (!iequals(i->path().extension().string(), L".mkv"))
                continue;

            Generate(i->path(), previewPath, time);
        }
    } catch (const filesystem_error& ex) {
        (ex);
    }
}

void MvPreviewGenerator::Generate(const path& mvPath, const path& previewPath,
                                  int64 time)
{
    assert(is_regular_file(mvPath));
    assert(iequals(mvPath.extension().wstring(), L".mkv"));

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

        if (packet.stream_index != videoStreamIndex) {
            av_free_packet(&packet);
            continue;
        }
        
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
    unique_ptr<AVFrame, void (*)(AVFrame*)> frameRGB(avcodec_alloc_frame(),
                                                     FreeAVFrame);
    const int previewWidth = codecCont->width / 6;
    const int previewHeight = codecCont->height / 6;
    const int bufSize = avpicture_get_size(PIX_FMT_RGB24, previewWidth,
                                           previewHeight);
    avpicture_fill(reinterpret_cast<AVPicture*>(frameRGB.get()),
                   reinterpret_cast<uint8_t*>(av_malloc(bufSize)),
                   PIX_FMT_RGB24, previewWidth, previewHeight);
    frameRGB->width = previewWidth;
    frameRGB->height = previewHeight;

    unique_ptr<SwsContext, void (*)(SwsContext*)> scaleCont(
        sws_getContext(codecCont->width, codecCont->height, codecCont->pix_fmt,
                       previewWidth, previewHeight, PIX_FMT_RGB24, SWS_BICUBIC,
                       NULL, NULL, NULL),
        sws_freeContext);
    if (!scaleCont)
        return;

    sws_scale(scaleCont.get(), frame.get()->data, frame.get()->linesize, 0,
              codecCont->height, frameRGB.get()->data,
              frameRGB.get()->linesize);

    wstring s = previewPath.wstring();
    if (is_directory(previewPath)) {
        UnifySlash(&s);
        s += mvPath.stem().wstring() + L".jpg";
    }

    Jpeg::SaveToJPEGFile(s, frameRGB.get());
    if (callback_)
        callback_->Progress(mvPath.wstring());
}
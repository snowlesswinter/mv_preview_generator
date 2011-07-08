#include "jpeg_tool.h"

#include <cstdio>
#include <memory>

#include <boost/filesystem.hpp>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "jpeglib.h"
}

using std::unique_ptr;
using boost::filesystem3::path;

void Jpeg::SaveToJPEGFile(const path& jpegPath, AVFrame* frame)
{
    assert(frame);
    unique_ptr<FILE, int (*)(FILE*)> jpegFile(
        _wfopen(jpegPath.wstring().c_str(), L"wb"), fclose);
    if (!jpegFile)
        return;

    jpeg_error_mgr errHandler;
    jpeg_compress_struct comp;
    comp.err = jpeg_std_error(&errHandler);
    jpeg_create_compress(&comp);
    jpeg_stdio_dest(&comp, jpegFile.get());

    comp.image_width = frame->width;
    comp.image_height = frame->height;
    comp.input_components = 3;
    comp.in_color_space = JCS_RGB;
    jpeg_set_defaults(&comp);
    jpeg_set_quality(&comp, 80, true);
    jpeg_start_compress(&comp, true);
    while (comp.next_scanline < comp.image_height) {
        JSAMPROW data[1];
        data[0] = reinterpret_cast<JSAMPROW>(
            frame->data[0] + comp.next_scanline * frame->linesize[0]);
        jpeg_write_scanlines(&comp, data, 1);
    }

    jpeg_finish_compress(&comp);
    jpeg_destroy_compress(&comp);
}
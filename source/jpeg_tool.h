#ifndef _JPEG_TOOL_H_
#define _JPEG_TOOL_H_

namespace boost
{
namespace filesystem3
{
class path;
}
}

struct AVFrame;

class Jpeg
{
public:
    static void SaveToJPEGFile(const boost::filesystem3::path& jpegPath,
                               AVFrame* frame, int width, int height);

private:
    Jpeg();
    ~Jpeg();
};

#endif  // _JPEG_TOOL_H_
#ifndef _PREVIEW_UPLOAD_H_
#define _PREVIEW_UPLOAD_H_

namespace boost
{
namespace filesystem3
{
class path;
}
}

class CPreviewUpload
{
public:
    static bool Upload(const boost::filesystem3::path& previewPath);

private:
    CPreviewUpload();
    ~CPreviewUpload();
};

#endif  // _PREVIEW_UPLOAD_H_
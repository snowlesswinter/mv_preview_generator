#include "preview_upload.h"

#include <memory>
#include <sstream>

#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>

#include "third_party/curl/include/curl/curl.h"
extern "C" {
#include "third_party/curl/lib/curl_md5.h"
};
#include "util.h"

using std::unique_ptr;
using std::string;
using std::wstring;
using std::stringstream;
using boost::gregorian::date;
using boost::gregorian::day_clock;
using boost::gregorian::to_iso_string;
using boost::filesystem3::path;
using boost::filesystem3::file_size;

CPreviewUpload::CPreviewUpload()
{
}

CPreviewUpload::~CPreviewUpload()
{
}

bool CPreviewUpload::Upload(const path& previewPath)
{
    unique_ptr<CURL, void(*)(CURL*)> curl(curl_easy_init(), curl_easy_cleanup);
    if (!curl)
        return false;

    date today = day_clock::local_day();
    string md5Raw = to_iso_string(today) + "hewry678WEK23D";

    unsigned char md5Buf[16];
    Curl_md5it(md5Buf, reinterpret_cast<const unsigned char*>(md5Raw.c_str()));
    string md5;
    for (int i = 0; i < 16; ++i) {
        stringstream s;
        s.setf(std::ios::hex, std::ios::basefield);
        s.fill('0');
        s.width(2);
        s << static_cast<int>(md5Buf[i]);
        md5 += s.str();
    }

    const int fileSize = static_cast<int>(file_size(previewPath));
    unique_ptr<FILE, int (*)(FILE*)> previewFile(
        _wfopen(previewPath.wstring().c_str(), L"rb"), fclose);
    if (!previewFile)
        return false;

    stringstream url;
    url << "http://image4.kugou.com/imageupload/stream.php?"
        "type=mvpic&extendName=.jpg&fileName=" <<
        WideCharToMultiByte(previewPath.filename().wstring()).c_str() <<
        "&md5=" << md5;

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1);
    curl_easy_setopt(curl.get(), CURLOPT_READDATA, previewFile.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, fileSize);
    CURLcode r = curl_easy_perform(curl.get());
    return (CURLE_OK == r);
}
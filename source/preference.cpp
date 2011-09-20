#include "preference.h"

#include <memory>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using std::wstring;
using std::endl;
using std::unique_ptr;
using boost::lexical_cast;
using boost::bad_lexical_cast;
using boost::algorithm::iequals;

namespace {
wstring GetStorageFilePathName()
{
    const int bufSize = GetCurrentDirectory(0, NULL);
    unique_ptr<wchar_t[]> buf(new wchar_t[bufSize]);
    GetCurrentDirectory(bufSize, buf.get());
    return wstring(buf.get()) + L"/preference.ini";
}
}

Preference::Preference()
    : storageFile_(GetStorageFilePathName())
    , mvLocation_()
    , previewLocation_()
    , previewAtMin_(0)
    , previewAtSec_(0)
    , recursive_(false)
{
    const wchar_t* appName = L"CONFIG";
    mvLocation_ = GetProfileString(appName, L"mv_location", L"");
    previewLocation_ = GetProfileString(appName, L"preview_location", L"");

    try {
        previewAtMin_ = lexical_cast<int>(
            GetProfileString(appName, L"preview_at_min", L"0"));
    } catch (const bad_lexical_cast&) {
        previewAtMin_ = 0;
    }

    try {
        previewAtSec_ = lexical_cast<int>(
            GetProfileString(appName, L"preview_at_sec", L"0"));
    } catch (const bad_lexical_cast&) {
        previewAtSec_ = 0;
    }

    recursive_ =
        iequals(GetProfileString(appName, L"recursive", L"false"), L"true");
}

Preference::~Preference()
{
    const wchar_t* appName = L"CONFIG";
    WritePrivateProfileString(appName, L"mv_location", mvLocation_.c_str(),
                              storageFile_.c_str());
    WritePrivateProfileString(appName, L"preview_location",
                              previewLocation_.c_str(), storageFile_.c_str());
    WritePrivateProfileString(appName, L"preview_at_min",
                              lexical_cast<wstring>(previewAtMin_).c_str(),
                              storageFile_.c_str());
    WritePrivateProfileString(appName, L"preview_at_sec",
                              lexical_cast<wstring>(previewAtSec_).c_str(),
                              storageFile_.c_str());
    WritePrivateProfileString(appName, L"recursive",
                              recursive_ ? L"true" : L"false",
                              storageFile_.c_str());
}

wstring Preference::GetProfileString(const wchar_t* appName,
                                     const wchar_t* keyName,
                                     const wchar_t* defaultValue)
{
    int bufSize = 128;
    unique_ptr<wchar_t[]> buf;
    int charCopied;
    const int terminatorLength = (!appName || !keyName) ? 2 : 1;
    do {
        bufSize *= 2;
        buf.reset(new wchar_t[bufSize]);
        charCopied = GetPrivateProfileString(appName, keyName, defaultValue, 
                                             buf.get(), bufSize,
                                             storageFile_.c_str());
    } while (charCopied >= (bufSize - terminatorLength));

    wstring result;
    if (!charCopied)
        return result;

    result.resize(charCopied);
    memcpy(&result[0], buf.get(), charCopied * sizeof(buf[0]));
    return result;
}
#include "preference.h"

using std::wstring;
using std::endl;

namespace {
const wchar_t* storageFileName = L"preference";
}

Preference::Preference()
    : storage_(storageFileName, std::ios_base::in|std::ios_base::out)
    , mvLocation_()
    , previewLocation_()
    , previewAtMin_(0)
    , previewAtSec_(0)
    , recursive_(false)
{
    if (!storage_.is_open()) {
        storage_.open(storageFileName, std::ios_base::out);
        storage_.flush();
        return;
    }

    wstring field;
    while (!storage_.eof()) {
        storage_ >> field;
        if (L"[1]" == field)
            storage_ >> mvLocation_;
        else if (L"[2]" == field)
            storage_ >> previewLocation_;
        else if (L"[3]" == field)
            storage_ >> previewAtMin_;
        else if (L"[4]" == field)
            storage_ >> previewAtSec_;
        else if (L"[5]" == field)
            storage_ >> recursive_;
    }

//     storage_.close();
//     storage_.open(storageFileName, std::ios_base::out);
    storage_.clear();
}

Preference::~Preference()
{
    storage_.seekp(0);
    storage_
        << L"[1]" << endl << mvLocation_ << endl
        << L"[2]" << endl << previewLocation_ << endl
        << L"[3]" << endl << previewAtMin_ << endl
        << L"[4]" << endl << previewAtSec_ << endl
        << L"[5]" << endl << recursive_;
}

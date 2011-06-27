#ifndef _PREFERENCE_H_
#define _PREFERENCE_H_

#include <fstream>
#include <string>

#include "third_party/chromium/base/singleton.h"

class Preference : public Singleton<Preference>
{
public:
    Preference();
    ~Preference();

    const std::wstring& GetMvLocation() const { return mvLocation_; }
    void SetMvLocation(const std::wstring& l) { mvLocation_ = l; }
    const std::wstring& GetPreviewLocation() const { return previewLocation_; }
    void SetPreviewLocation(const std::wstring& l) { previewLocation_ = l; }
    int GetPreviewMin() const { return previewAtMin_; }
    void SetPreviewMin(int t) { previewAtMin_ = t; }
    int GetPreviewSec() const { return previewAtSec_; }
    void SetPreviewSec(int t) { previewAtSec_ = t; }
    bool IsRecursive() const { return recursive_; }
    void SetRecursive(bool r) { recursive_ = r; }

private:
    std::wfstream storage_;
    std::wstring mvLocation_;
    std::wstring previewLocation_;
    int previewAtMin_;
    int previewAtSec_;
    bool recursive_;
};

#endif  // _PREFERENCE_H_
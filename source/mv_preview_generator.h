#ifndef _MV_PREVIEW_GENERATOR_H_
#define _MV_PREVIEW_GENERATOR_H_

#include <list>
#include <string>

#include "third_party/chromium/base/basictypes.h"

namespace boost
{
namespace filesystem3
{
class path;
}
}

class MvPreviewGenerator;
class GeneratorCallback
{
public:
    virtual void Initializing(MvPreviewGenerator* gen, int totalFiles) = 0;
    virtual void Progress(const std::wstring& current) = 0;
    virtual void Done() = 0;
};

class MvPreviewGenerator
{
public:
    explicit MvPreviewGenerator(GeneratorCallback* callback);
    ~MvPreviewGenerator();

    void StartGenerating(const boost::filesystem3::path& mvPath, int64 time,
                         bool recursive);
    void Cancel() { cancelFlag_ = true; }

private:
    int CalculateNumFiles(const boost::filesystem3::path& mvPath,
                          bool recursive);
    void SearchAndGenerate(const boost::filesystem3::path& mvPath,
                           std::list<boost::filesystem3::path>* pending,
                           int64 time, bool recursive);
    void Generate(const boost::filesystem3::path& mvPath, int64 time);

    GeneratorCallback* callback_;
    bool cancelFlag_;
};

#endif  // _MV_PREVIEW_GENERATOR_H_
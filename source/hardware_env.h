#ifndef _HARDWARE_ENV_H_
#define _HARDWARE_ENV_H_

#include "third_party/chromium/base/singleton.h"

class CHardwareEnv : public Singleton<CHardwareEnv>
{
public:
    enum KProcessorVendor
    {
        PROCESSOR_VENDOR_AMD,
        PROCESSOR_VENDOR_INTEL,
        PROCESSOR_VENDOR_UNKNOWN
    };

    enum KProcessorFeatures
    {
        PROCESSOR_FEATURE_MMX = 0x0001,
        PROCESSOR_FEATURE_3DNOW = 0x0004,
        PROCESSOR_FEATURE_MMXEXT = 0x0002,
        PROCESSOR_FEATURE_SSE = 0x0008,
        PROCESSOR_FEATURE_SSE2 = 0x0010,
        PROCESSOR_FEATURE_3DNOWEXT = 0x0020,
        PROCESSOR_FEATURE_SSE3 = 0x0040,
        PROCESSOR_FEATURE_SSSE3 = 0x0080
    };

    CHardwareEnv();
    ~CHardwareEnv();

    int GetProcessorFeatures() const { return m_processorFeatures; }
    int GetNumOfLogicalProcessors() const;

private:
    void detectProcessor();

    int m_processorVendor;
    int m_processorFeatures;
};

#endif  // _HARDWARE_ENV_H_
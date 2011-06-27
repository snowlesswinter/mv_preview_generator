#include "hardware_env.h"

#include <intrin.h>

CHardwareEnv::CHardwareEnv()
    : Singleton<CHardwareEnv>()
    , m_processorVendor(PROCESSOR_VENDOR_UNKNOWN)
    , m_processorFeatures(0)
{
    detectProcessor();
}

CHardwareEnv::~CHardwareEnv()
{
}

int CHardwareEnv::GetNumOfLogicalProcessors() const
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

void CHardwareEnv::detectProcessor()
{
    int buf[4];
    __cpuid(buf, 0);
    uint32 highestFeature = static_cast<uint32>(buf[0]);

    std::swap(buf[2], buf[3]);
    char manufacturer[13];
    memcpy(manufacturer, &buf[1], 12);
    manufacturer[12] = '\0';
    if (!strcmp(manufacturer, "AuthenticAMD"))
        m_processorVendor = PROCESSOR_VENDOR_AMD;
    else if (!strcmp(manufacturer, "GenuineIntel"))
        m_processorVendor = PROCESSOR_VENDOR_INTEL;
    else
        m_processorVendor = PROCESSOR_VENDOR_UNKNOWN;

    __cpuid(buf, 0x80000000);
    uint32 highestFeatureEx = static_cast<uint32>(buf[0]);

    m_processorFeatures = 0;
    if (highestFeature >= 1)
    {
        __cpuid(buf, 1);
        if (buf[3] & (1 << 23))
            m_processorFeatures |= PROCESSOR_FEATURE_MMX;

        if (buf[3] & (1 << 25))
            m_processorFeatures |= PROCESSOR_FEATURE_SSE;

        if (buf[3] & (1 << 26))
            m_processorFeatures |= PROCESSOR_FEATURE_SSE2;

        if (buf[3] & (1 << 0))
            m_processorFeatures |= PROCESSOR_FEATURE_SSE3;

        if (PROCESSOR_VENDOR_INTEL == m_processorVendor)
            if (buf[2] & (1 << 9))
                m_processorFeatures |= PROCESSOR_FEATURE_SSSE3;
    }

    if (PROCESSOR_VENDOR_AMD == m_processorVendor)
    {
        __cpuid(buf, 0x80000000);
        if (highestFeatureEx >= 0x80000001)
        {
            __cpuid(buf, 0x80000001);
            if (buf[3] & (1 << 31))
                m_processorFeatures |= PROCESSOR_FEATURE_3DNOW;

            if (buf[3] & (1 << 22))
                m_processorFeatures |= PROCESSOR_FEATURE_MMXEXT;
        }
    }
}
#pragma once

#include "mfc_predefine.h"
#include "resource/resource.h" // main symbols

// CMvPreviewGeneratorApp:
// See mv_preview_generator.cpp for the implementation of this class
//
class CMvPreviewGeneratorApp : public CWinAppEx
{
public:
    CMvPreviewGeneratorApp();
    virtual BOOL InitInstance();
    virtual int ExitInstance();

protected:
    DECLARE_MESSAGE_MAP()
};

extern CMvPreviewGeneratorApp theApp;
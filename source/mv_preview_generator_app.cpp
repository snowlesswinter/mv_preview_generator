#include "mv_preview_generator_app.h"

#include <cassert>

#include "third_party/chromium/base/at_exit.h"
#include "mv_preview_generator_dialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
base::AtExitManager* atExit = NULL;
}

// CMvPreviewGeneratorApp
BEGIN_MESSAGE_MAP(CMvPreviewGeneratorApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// CMvPreviewGeneratorApp construction
CMvPreviewGeneratorApp::CMvPreviewGeneratorApp()
{
}

// The one and only CMvPreviewGeneratorApp object
CMvPreviewGeneratorApp theApp;

// CMvPreviewGeneratorApp initialization
BOOL CMvPreviewGeneratorApp::InitInstance()
{
    assert(!atExit);
    atExit = new base::AtExitManager;

    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX initControls;
    initControls.dwSize = sizeof(initControls);

    // Set this to include all the common control classes you want to use
    // in your application.
    initControls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&initControls);

    CWinAppEx::InitInstance();

    CMvPreviewGeneratorDialog dialog;
    m_pMainWnd = &dialog;
    INT_PTR response = dialog.DoModal();
    if (response == IDOK) {
        // TODO: Place code here to handle when the dialog is
        // dismissed with OK
    } else if (response == IDCANCEL) {
        // TODO: Place code here to handle when the dialog is
        // dismissed with Cancel
    }

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}

int CMvPreviewGeneratorApp::ExitInstance()
{
    int result = CWinAppEx::ExitInstance();

    assert(atExit);
    if (atExit) {
        delete atExit;
        atExit = NULL;
    }

    return result;
}
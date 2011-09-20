#include <vld.h>

#include "mv_preview_generator_dialog.h"

#include <memory>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "afxdialogex.h"

#include "mv_preview_generator_app.h"
#include "mv_preview_generator.h"
#include "progress_dialog.h"
#include "preference.h"
#include "preview_dialog.h"

using std::wstring;
using std::wstringstream;
using std::unique_ptr;
using boost::filesystem3::path;
using boost::algorithm::trim_right;
using boost::algorithm::is_digit;
using boost::algorithm::all;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {
int __stdcall BrowseCallbackProc(HWND winHandle, UINT message, LPARAM param,
                                 LPARAM data)
{
    switch (message) {
        case BFFM_INITIALIZED:
            SendMessage(winHandle, BFFM_SETSELECTION, TRUE, data);
            SendMessage(winHandle, BFFM_SETEXPANDED, TRUE, data);
            break;
        case BFFM_VALIDATEFAILED:
            return 1;
        default:
            break;
    }

    return 0;
}

wstring TranslatePreviewTime(int fromPersistence)
{
    if ((fromPersistence < 0) || (fromPersistence > 59))
        return L"00";

    wstringstream s;
    s.fill('0');
    s.width(2);
    s << fromPersistence;
    return s.str();
}

void OnEditUpdate(CEdit* editControl, wstring* record)
{
    assert(editControl);
    assert(record);

    CString t;
    editControl->GetWindowText(t);
    wstring s(t.GetBuffer());
    if (s.length() < 3) {
        if (all(s, is_digit())) {
            wstringstream f(s);
            int n;
            f >> n;
            if ((n >= 0) && (n < 60)) {
                *record = s;
                return;
            }
        }
    }

    editControl->SetWindowText(record->c_str());
}

int OnEditKillFocus(CEdit* editControl, wstring* record)
{
    assert(editControl);
    assert(record);

    int n;
    wstringstream s(*record);
    s >> n;
    if (record->length() != 2) {
        s.clear();
        s.fill('0');
        s.width(2);
        s << *record;
        *record = s.str();
        editControl->SetWindowText(record->c_str());
    }

    return n;
}

void StartGenerating(GeneratorCallback* callback, const path& mvPath,
                     const path& previewPath, int64 time, bool recursive)
{
    MvPreviewGenerator gen(callback);
    gen.StartGenerating(mvPath, previewPath, time, recursive);
}
}

void StartGeneratingFromDataBase(GeneratorCallback* callback,
                                 const path& previewPath, int64 time)
{
    CoInitialize(NULL);
    MvPreviewGenerator gen(callback);
    wstring dbServer = L"192.168.0.206\\mssql2008";
    wstring userName = L"sa";
    wstring password = L"ivwihicnck";
    gen.StartGenerating(dbServer, userName, password, previewPath, time);
    CoUninitialize();
}

class CAboutDlg : public CDialogEx
{
public:
    enum { IDD = IDD_ABOUTBOX };

    CAboutDlg();

protected:
    virtual void DoDataExchange(CDataExchange* dataExch); // DDX/DDV support
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg()
    : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* dataExch)
{
    CDialogEx::DoDataExchange(dataExch);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

//------------------------------------------------------------------------------
CMvPreviewGeneratorDialog::CMvPreviewGeneratorDialog(CWnd* parent /*=NULL*/)
    : CDialogEx(CMvPreviewGeneratorDialog::IDD, parent)
    , icon_(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
    , mvLocation_()
    , browseForMv_()
    , previewLocation_()
    , browseForPreview_()
    , previewAtMin_()
    , previewAtSec_()
    , recursive_()
    , tooltip_()
    , lastMin_()
    , lastSec_()
    , thread_("generating")
    , fromDbServer_()
{
}

void CMvPreviewGeneratorDialog::DoDataExchange(CDataExchange* dataExch)
{
    CDialogEx::DoDataExchange(dataExch);
    DDX_Control(dataExch, IDC_COMBO_MV_DIR, mvLocation_);
    DDX_Control(dataExch, IDC_BUTTON_BROWSE_MV_DIR, browseForMv_);
    DDX_Control(dataExch, IDC_COMBO_PREVIEW_DIR, previewLocation_);
    DDX_Control(dataExch, IDC_BUTTON_BROWSE_PREVIEW_DIR, browseForPreview_);
    DDX_Control(dataExch, IDC_EDIT_MIN, previewAtMin_);
    DDX_Control(dataExch, IDC_EDIT_SEC, previewAtSec_);
    DDX_Control(dataExch, IDC_CHECK_RECURSIVE, recursive_);
    DDX_Control(dataExch, IDC_CHECK_FROM_DB_SERVER, fromDbServer_);
}

BEGIN_MESSAGE_MAP(CMvPreviewGeneratorDialog, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_GEN,
                  &CMvPreviewGeneratorDialog::OnBnClickedButtonGen)
    ON_CBN_KILLFOCUS(IDC_COMBO_MV_DIR,
                     &CMvPreviewGeneratorDialog::OnCbnKillfocusComboMvDir)
    ON_CBN_KILLFOCUS(IDC_COMBO_PREVIEW_DIR,
                     &CMvPreviewGeneratorDialog::OnCbnKillfocusComboPreviewDir)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_MV_DIR,
                  &CMvPreviewGeneratorDialog::OnBnClickedButtonBrowseMvDir)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_PREVIEW_DIR,
                  &CMvPreviewGeneratorDialog::OnBnClickedButtonBrowsePreviewDir)
    ON_EN_UPDATE(IDC_EDIT_MIN,
                 &CMvPreviewGeneratorDialog::OnEnUpdateEditMin)
    ON_EN_KILLFOCUS(IDC_EDIT_MIN,
                    &CMvPreviewGeneratorDialog::OnEnKillfocusEditMin)
    ON_EN_UPDATE(IDC_EDIT_SEC,
                 &CMvPreviewGeneratorDialog::OnEnUpdateEditSec)
    ON_EN_KILLFOCUS(IDC_EDIT_SEC,
                    &CMvPreviewGeneratorDialog::OnEnKillfocusEditSec)
    ON_BN_CLICKED(IDC_CHECK_RECURSIVE,
                  &CMvPreviewGeneratorDialog::OnBnClickedCheckRecursive)
    ON_BN_CLICKED(IDC_CHECK_FROM_DB_SERVER,
                  &CMvPreviewGeneratorDialog::OnBnClickedCheckFromDbServer)
END_MESSAGE_MAP()

// CMvPreviewGeneratorDialog message handlers

BOOL CMvPreviewGeneratorDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* sysMenu = GetSystemMenu(FALSE);
    if (sysMenu) {
        BOOL nameValid;
        CString aboutMenu;
        nameValid = aboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(nameValid);
        if (!aboutMenu.IsEmpty()) {
            sysMenu->AppendMenu(MF_SEPARATOR);
            sysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, aboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    // when the application'l main window is not a dialog
    SetIcon(icon_, TRUE); // Set big icon
    SetIcon(icon_, FALSE); // Set small icon

    CMFCToolTipInfo toolTipInfo;
    toolTipInfo.m_bBalloonTooltip = TRUE;
    toolTipInfo.m_bBoldLabel = FALSE;
    toolTipInfo.m_bDrawDescription = TRUE;
    toolTipInfo.m_bDrawIcon = TRUE;
    toolTipInfo.m_bRoundedCorners = TRUE;
    toolTipInfo.m_bDrawSeparator = FALSE;

    tooltip_.reset(new CMFCToolTipCtrl(&toolTipInfo));
    tooltip_->Create(this);
    tooltip_->SetDescription(L"Invalid input");
    const wchar_t* intruction = L"Input a number between 0 to 59";
    tooltip_->AddTool(&previewAtMin_, intruction);
    tooltip_->AddTool(&previewAtSec_, intruction);
    tooltip_->SetDelayTime(0);
    tooltip_->Activate(TRUE);

    wstring l = Preference::get()->GetMvLocation();
    mvLocation_.SetWindowText(l.empty() ? L"c:\\" : l.c_str());

    l = Preference::get()->GetPreviewLocation();
    previewLocation_.SetWindowText(l.empty() ? L"c:\\" : l.c_str());

    lastMin_ = TranslatePreviewTime(Preference::get()->GetPreviewMin());
    previewAtMin_.SetWindowText(lastMin_.c_str());

    lastSec_ = TranslatePreviewTime(Preference::get()->GetPreviewSec());
    previewAtSec_.SetWindowText(lastSec_.c_str());

    bool r = Preference::get()->IsRecursive();
    recursive_.SetCheck(r ? 1 : 0);

    fromDbServer_.SetCheck(1);

    // Some controls are obsoleted.
    mvLocation_.EnableWindow(FALSE);
    browseForMv_.EnableWindow(FALSE);
    recursive_.EnableWindow(FALSE);
    return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CMvPreviewGeneratorDialog::PreTranslateMessage(MSG* message)
{
    if (tooltip_)
        tooltip_->RelayEvent(message);

    return CDialogEx::PreTranslateMessage(message);
}

void CMvPreviewGeneratorDialog::OnSysCommand(UINT id, LPARAM param)
{
    if ((id & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg about;
        about.DoModal();
    } else {
        CDialogEx::OnSysCommand(id, param);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CMvPreviewGeneratorDialog::OnPaint()
{
    if (IsIconic()) {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND,
                    reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, icon_);
    } else {
        CDialogEx::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMvPreviewGeneratorDialog::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(icon_);
}

void CMvPreviewGeneratorDialog::OnBnClickedButtonGen()
{
    CString text;
    mvLocation_.GetWindowText(text);
    path mvLocation(text.GetBuffer());

    if (mvLocation_.FindString(-1, text) < 0)
        mvLocation_.AddString(text);

    previewLocation_.GetWindowText(text);
    path previewLocation(text.GetBuffer());
    if (previewLocation_.FindString(-1, text) < 0)
        previewLocation_.AddString(text);

    wstringstream s1(lastMin_);
    wstringstream s2(lastSec_);
    int m;
    int s;
    s1 >> m;
    s2 >> s;
    s = s + m * 60;

    ProgressDialog d(this);
    thread_.Start();
    thread_.message_loop()->PostTask(
        FROM_HERE,
        !!fromDbServer_.GetCheck() ? 
            NewRunnableFunction(StartGeneratingFromDataBase, &d,
                                previewLocation, s * 1000I64) :
            NewRunnableFunction(StartGenerating, &d, mvLocation,
                                previewLocation, s * 1000I64,
                                !!recursive_.GetCheck()));
    d.DoModal();
    thread_.Stop();
}

void CMvPreviewGeneratorDialog::OnCbnKillfocusComboMvDir()
{
    CString curSetting;
    mvLocation_.GetWindowText(curSetting);
    Preference::get()->SetMvLocation(wstring(curSetting.GetBuffer()));
}

void CMvPreviewGeneratorDialog::OnCbnKillfocusComboPreviewDir()
{
    CString curSetting;
    previewLocation_.GetWindowText(curSetting);
    Preference::get()->SetMvLocation(wstring(curSetting.GetBuffer()));
}

void CMvPreviewGeneratorDialog::OnBnClickedButtonBrowseMvDir()
{
    CString curSetting;
    mvLocation_.GetWindowText(curSetting);

    wstring title;
    title.resize(MAX_PATH + 1);

    BROWSEINFO b = {0};
    b.hwndOwner = GetSafeHwnd();
    b.pszDisplayName = &title[0];
    b.lpszTitle = L"Select MV location";
    b.ulFlags = BIF_USENEWUI | BIF_BROWSEINCLUDEFILES;
    b.lpfn = BrowseCallbackProc;
    b.lParam = reinterpret_cast<LPARAM>(curSetting.GetBuffer());

    PIDLIST_ABSOLUTE p = SHBrowseForFolder(&b);
    unique_ptr<void, void (__stdcall*)(void*)> autoRelease(p, CoTaskMemFree);
    if (!p)
        return;

    wstring result;
    result.resize(MAX_PATH + 1);
    if (SHGetPathFromIDList(p, &result[0])) {
        trim_right(result);
        if (mvLocation_.FindString(-1, result.c_str()) < 0)
            mvLocation_.AddString(result.c_str());

        mvLocation_.SetWindowText(result.c_str());
        Preference::get()->SetMvLocation(result);
        mvLocation_.SetFocus();
    }
}

void CMvPreviewGeneratorDialog::OnBnClickedButtonBrowsePreviewDir()
{
    CString curSetting;
    previewLocation_.GetWindowText(curSetting);

    wstring title;
    title.resize(MAX_PATH + 1);

    BROWSEINFO b = {0};
    b.hwndOwner = GetSafeHwnd();
    b.pszDisplayName = &title[0];
    b.lpszTitle = L"Select preview location";
    b.ulFlags = BIF_USENEWUI | BIF_BROWSEINCLUDEFILES;
    b.lpfn = BrowseCallbackProc;
    b.lParam = reinterpret_cast<LPARAM>(curSetting.GetBuffer());

    PIDLIST_ABSOLUTE p = SHBrowseForFolder(&b);
    unique_ptr<void, void (__stdcall*)(void*)> autoRelease(p, CoTaskMemFree);
    if (!p)
        return;

    wstring result;
    result.resize(MAX_PATH + 1);
    if (SHGetPathFromIDList(p, &result[0])) {
        trim_right(result);
        if (previewLocation_.FindString(-1, result.c_str()) < 0)
            previewLocation_.AddString(result.c_str());

        previewLocation_.SetWindowText(result.c_str());
        Preference::get()->SetPreviewLocation(result);
        previewLocation_.SetFocus();
    }
}

void CMvPreviewGeneratorDialog::OnEnUpdateEditMin()
{
    OnEditUpdate(&previewAtMin_, &lastMin_);
}

void CMvPreviewGeneratorDialog::OnEnKillfocusEditMin()
{
    Preference::get()->SetPreviewMin(
        OnEditKillFocus(&previewAtMin_, &lastMin_));
}

void CMvPreviewGeneratorDialog::OnEnUpdateEditSec()
{
    OnEditUpdate(&previewAtSec_, &lastSec_);
}

void CMvPreviewGeneratorDialog::OnEnKillfocusEditSec()
{
    Preference::get()->SetPreviewSec(
        OnEditKillFocus(&previewAtSec_, &lastSec_));
}

void CMvPreviewGeneratorDialog::OnBnClickedCheckRecursive()
{
    Preference::get()->SetRecursive(!!recursive_.GetCheck());
}

void CMvPreviewGeneratorDialog::OnBnClickedCheckFromDbServer()
{
    int fromDbServer = fromDbServer_.GetCheck();
    mvLocation_.EnableWindow(!fromDbServer);
    browseForMv_.EnableWindow(!fromDbServer);
    recursive_.EnableWindow(!fromDbServer);
}
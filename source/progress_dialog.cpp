#include "progress_dialog.h"

#include <cassert>

#include "afxdialogex.h"

namespace {
const int kMessageInitializing = WM_USER + 100;
const int kMessageDone = WM_USER + 101;
}

IMPLEMENT_DYNAMIC(ProgressDialog, CDialog)

ProgressDialog::ProgressDialog(CWnd* parent)
    : CDialog(ProgressDialog::IDD, parent)
    , gen_(NULL)
    , total_(0)
    , finished_(0)
    , currentFile_()
    , progress_()
{
}

ProgressDialog::~ProgressDialog()
{
}

void ProgressDialog::Initializing(MvPreviewGenerator* gen, int totalFiles)
{
    PostMessage(kMessageInitializing, reinterpret_cast<WPARAM>(gen),
                totalFiles);
}

void ProgressDialog::Progress(const std::wstring& current)
{
    currentFile_.SetWindowText(current.c_str());
    finished_++;
    progress_.SetPos(finished_);
}

void ProgressDialog::Done()
{
    PostMessage(kMessageDone, 0, 0);
}

void ProgressDialog::DoDataExchange(CDataExchange* dataExch)
{
    CDialog::DoDataExchange(dataExch);
    DDX_Control(dataExch, IDC_STATIC_CURRENT_FILE, currentFile_);
    DDX_Control(dataExch, IDC_PROGRESS1, progress_);
}

BEGIN_MESSAGE_MAP(ProgressDialog, CDialog)
    ON_MESSAGE(kMessageInitializing, OnInitializing)
    ON_MESSAGE(kMessageDone, OnDone)
END_MESSAGE_MAP()

void ProgressDialog::OnCancel()
{
    if (gen_)
        gen_->Cancel();
}

BOOL ProgressDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetWindowText(L"Processing...");
    currentFile_.SetWindowText(L"");
    progress_.SetRange32(0, total_);
    return TRUE;
}

LRESULT ProgressDialog::OnInitializing(WPARAM w, LPARAM l)
{
    assert(!gen_);
    assert(gen);
    assert(totalFiles >= 0);

    gen_ = reinterpret_cast<MvPreviewGenerator*>(w);
    total_ = l;
    finished_ = 0;
    if (progress_.GetSafeHwnd())
        progress_.SetRange32(0, total_);

    return 0;
}

LRESULT ProgressDialog::OnDone(WPARAM w, LPARAM l)
{
    assert(gen_);
    gen_ = NULL;
    total_ = 0;
    finished_ = 0;
    EndDialog(IDOK);
    return 0;
}
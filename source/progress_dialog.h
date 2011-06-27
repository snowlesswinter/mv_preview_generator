#ifndef _PROGRESS_DIALOG_H_
#define _PROGRESS_DIALOG_H_

#include "mfc_predefine.h"
#include "afxwin.h"

#include "mv_preview_generator.h"
#include "resource/resource.h" // main symbols
#include "afxcmn.h"

class ProgressDialog : public CDialog, public GeneratorCallback
{
public:
    enum { IDD = IDD_DIALOG_PROGRESS };

    explicit ProgressDialog(CWnd* parent); // standard constructor
    virtual ~ProgressDialog();

    virtual void Initializing(MvPreviewGenerator* gen, int totalFiles);
    virtual void Progress(const std::wstring& current);
    virtual void Done();

protected:
    virtual void DoDataExchange(CDataExchange* dataExch); // DDX/DDV support
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    virtual LRESULT OnInitializing(WPARAM w, LPARAM l);
    virtual LRESULT OnDone(WPARAM w, LPARAM l);

    DECLARE_MESSAGE_MAP()

private:
    DECLARE_DYNAMIC(ProgressDialog)

    MvPreviewGenerator* gen_;
    int total_;
    int finished_;
    CStatic currentFile_;
    CProgressCtrl progress_;
};

#endif  // _PROGRESS_DIALOG_H_

#pragma once

#include <memory>
#include <string>

#include "mfc_predefine.h"
#include "afxwin.h"

#include "third_party/chromium/base/thread.h"
#include "resource/resource.h" // main symbols

// CMvPreviewGeneratorDialog dialog
class CMvPreviewGeneratorDialog : public CDialogEx
{
// Construction
public:
    enum { IDD = IDD_MV_PREVIEW_GENERATOR_DIALOG };

    CMvPreviewGeneratorDialog(CWnd* parent = NULL); // standard constructor

protected:
    virtual void DoDataExchange(CDataExchange* dataExch); // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* message);
    afx_msg void OnSysCommand(UINT id, LPARAM param);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedButtonGen();
    afx_msg void OnCbnKillfocusComboMvDir();
    afx_msg void OnCbnKillfocusComboPreviewDir();
    afx_msg void OnBnClickedButtonBrowseMvDir();
    afx_msg void OnBnClickedButtonBrowsePreviewDir();
    afx_msg void OnEnUpdateEditMin();
    afx_msg void OnEnKillfocusEditMin();
    afx_msg void OnEnUpdateEditSec();
    afx_msg void OnEnKillfocusEditSec();
    afx_msg void OnBnClickedCheckRecursive();
    DECLARE_MESSAGE_MAP()

private:
    HICON icon_;
    CComboBox mvLocation_;
    CButton browseForMv_;
    CComboBox previewLocation_;
    CButton browseForPreview_;
    CEdit previewAtMin_;
    CEdit previewAtSec_;
    CButton recursive_;
    std::unique_ptr<CMFCToolTipCtrl> tooltip_;
    std::wstring lastMin_;
    std::wstring lastSec_;
    base::Thread thread_;
};

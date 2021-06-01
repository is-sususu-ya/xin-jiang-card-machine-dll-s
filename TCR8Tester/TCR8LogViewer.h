#if !defined(AFX_TCR8LOGVIEWER_H__C19C00B0_E502_4FAA_A115_C10725367A72__INCLUDED_)
#define AFX_TCR8LOGVIEWER_H__C19C00B0_E502_4FAA_A115_C10725367A72__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TCR8LogViewer.h : header file
#include "../../common/utils_macro.h"

/////////////////////////////////////////////////////////////////////////////
// CTCR8LogViewer dialog

class CTCR8LogViewer : public CDialog
{
// Construction
public:
	BOOL m_bPauseRoll;
	void AppendTCR8Log(const char *txt, int len);
	CTCR8LogViewer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTCR8LogViewer)
	enum { IDD = IDD_DIALOG_TCR8LOGVIEWER };
	CListCtrl	m_listTCR8Log;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTCR8LogViewer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTCR8LogViewer)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckPauseRoll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TCR8LOGVIEWER_H__C19C00B0_E502_4FAA_A115_C10725367A72__INCLUDED_)

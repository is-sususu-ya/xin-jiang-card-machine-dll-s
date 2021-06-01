#if !defined(AFX_PROTOCOLVIEWER_H__433FDF76_AC1A_4119_9ABB_0F91631026DA__INCLUDED_)
#define AFX_PROTOCOLVIEWER_H__433FDF76_AC1A_4119_9ABB_0F91631026DA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProtocolViewer.h : header file
#include "../../common/utils_macro.h"
//

/////////////////////////////////////////////////////////////////////////////
// CProtocolViewer dialog

class CProtocolViewer : public CDialog
{
// Construction
public:
	BOOL m_bPauseRoll;
	void AppendProtocol(char *pPacket, BOOL bIsTx, int len);
	CProtocolViewer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CProtocolViewer)
	enum { IDD = IDD_DIALOG_PROTOCOLVIEW };
	CListCtrl	m_listProtocol;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProtocolViewer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProtocolViewer)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckPauseRoll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROTOCOLVIEWER_H__433FDF76_AC1A_4119_9ABB_0F91631026DA__INCLUDED_)

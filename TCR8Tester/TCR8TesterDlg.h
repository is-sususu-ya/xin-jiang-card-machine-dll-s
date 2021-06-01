// TCR8TesterDlg.h : header file
//

#if !defined(AFX_TCR8TESTERDLG_H__8877ECD2_209B_4EE4_A27B_868E94E2A5CB__INCLUDED_)
#define AFX_TCR8TESTERDLG_H__8877ECD2_209B_4EE4_A27B_868E94E2A5CB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../TCR8Lib/TCR8lib.h"
#include "../../common/utils_macro.h"
#include "TextProgressBar.h"
#include "SetBoxInfo.h"
#include "ProtocolViewer.h"
#include "TCR8LogViewer.h"

/////////////////////////////////////////////////////////////////////////////
// CTCR8TesterDlg dialog
typedef struct tagTCR8UserData {
	HWND	hWnd;
	UINT	Msg;
	char    TxData[64];
	char    RxData[32];
	char    TCR8Log[256];
}TCR8UserData;

class CTCR8TesterDlg : public CDialog
{
// Construction
public:
	long VersionCode( const char * cCode );
	CTCR8LogViewer m_dlgTCR8LogViewer;
	void AppendTCR8Log( LPCTSTR txt );
	void UpdateActiveChannel(int iUpper, int iLower);
	CProtocolViewer m_dlgProtocolViewer;
	void AppendPacket(char *pPacket, BOOL bTx);
	CSetBoxInfo m_dlgSetBoxInfo;
	int m_nOperateChannel;
	void OnBoxUnload(int iBoxNum);
	void ClearBoxSN(int iBoxNum);
	void SetChennalState(int nChennal, BOOL bNormal);
	void ShowCardOnExit(int nChennal, BOOL bShow);
	void ShowCardOnReader(int nChennal, BOOL bShow);
	void UpdateCardCount(int iBoxNum, int iCount);
	void UpdateBoxSN(int iBoxNum, DWORD dwSN);
	int m_nBaudRate;
	void AppendEventText(LPCTSTR txt);
	TCR8UserData m_TCR8UserData;
	TCR8HANDLE m_hTCR8;
	void EnableControls(BOOL bEnable);
	CTCR8TesterDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTCR8TesterDlg)
	enum { IDD = IDD_TCR8TESTER_DIALOG };
	CTextProgressBar	m_ProgressBox4;
	CTextProgressBar	m_ProgressBox3;
	CTextProgressBar	m_ProgressBox2;
	CTextProgressBar	m_ProgressBox1;
	CListCtrl	m_ListEvent;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTCR8TesterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CTCR8TesterDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonOpencom();
	afx_msg void OnButtonClosecom();
	afx_msg void OnButtonRun();
	afx_msg void OnButtonSuspend();
	afx_msg void OnDestroy();
	afx_msg void OnButtonEject();
	afx_msg void OnButtonRecycle();
	afx_msg void OnButtonCancelbutton();
	afx_msg void OnButtonSwitchChennal();
	afx_msg void OnButtonSwitchAnt();
	afx_msg void OnButtonSetboxinfo();
	afx_msg void OnButtonViewprotocol();
	afx_msg void OnButtonViewtcr8log();
	afx_msg void OnButtonGetfirmver();
	afx_msg void OnRadioDispenser();
	afx_msg void OnRadioCollector();
	afx_msg void OnButtonTriggerButton();
	afx_msg void OnButtonPullBack();
	//}}AFX_MSG
	afx_msg void OnButtonBaudSelect(UINT uID);
	afx_msg void OnButtonOperateChannelSelect(UINT uID);
	afx_msg void OnTCR8Event(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//afx_msg void OnButtonGerFirmVer();

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TCR8TESTERDLG_H__8877ECD2_209B_4EE4_A27B_868E94E2A5CB__INCLUDED_)

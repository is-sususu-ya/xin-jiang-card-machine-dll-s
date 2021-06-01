#if !defined(AFX_CONFIGDLG_H__62FB9254_122A_49BE_9FE7_05732778966C__INCLUDED_)
#define AFX_CONFIGDLG_H__62FB9254_122A_49BE_9FE7_05732778966C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog

class CConfigDlg : public CDialog
{
// Construction
public:
	CConfigDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigDlg)
	enum { IDD = IDD_DIALOG_CONFIG };
	CSpinButtonCtrl	m_spinMaskPeriod;
	CSpinButtonCtrl	m_spinTMINOutbox;
	CSpinButtonCtrl	m_spinTMAXOutbox;
	CSpinButtonCtrl	m_spinTMINEject;
	CSpinButtonCtrl	m_spinTMAXEject;
	UINT	m_nOddsEject;
	UINT	m_nOddsOutbox;
	UINT	m_tmaxEject;
	UINT	m_tminEject;
	UINT	m_tmaxOutbox;
	UINT	m_tminOutbox;
	UINT	m_nMaskPeriod;
	int		m_nSpeed;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	afx_msg void OnRadioSpeed(UINT uID);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGDLG_H__62FB9254_122A_49BE_9FE7_05732778966C__INCLUDED_)

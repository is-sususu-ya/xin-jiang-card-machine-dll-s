#if !defined(AFX_SETBOXINFO_H__67DF3035_AC5E_4C54_9FDE_A79BCF14E7CD__INCLUDED_)
#define AFX_SETBOXINFO_H__67DF3035_AC5E_4C54_9FDE_A79BCF14E7CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SetBoxInfo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSetBoxInfo dialog

class CSetBoxInfo : public CDialog
{
// Construction
public:
	/*
	short m_iBoxCapacity;
	short m_iCardCounter;
	DWORD m_dwBoxSN;
	*/
	CSetBoxInfo(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetBoxInfo)
	enum { IDD = IDD_DIALOG_SETBOXINFO };
	int		m_iBoxCapacity;
	int		m_iCardCounter;
	long	m_dwBoxSN;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetBoxInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSetBoxInfo)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETBOXINFO_H__67DF3035_AC5E_4C54_9FDE_A79BCF14E7CD__INCLUDED_)

#if !defined(AFX_TEXTPROGRESSBAR_H__4C841DFE_54BA_419E_A04B_62882CCC9C16__INCLUDED_)
#define AFX_TEXTPROGRESSBAR_H__4C841DFE_54BA_419E_A04B_62882CCC9C16__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TextProgressBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTextProgressBar window

class CTextProgressBar : public CProgressCtrl
{
// Construction
public:
	CTextProgressBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextProgressBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL m_bClearProgressBar;
	void ClearProgressBar();
	int m_nCurrentPosition;
	int m_nUpper;
	int m_nLower;
	int SetPos(int nPos);
	void SetRange(int nLower, int nUpper);
	virtual ~CTextProgressBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTextProgressBar)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXTPROGRESSBAR_H__4C841DFE_54BA_419E_A04B_62882CCC9C16__INCLUDED_)

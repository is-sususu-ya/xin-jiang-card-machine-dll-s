// TCR8Tester.h : main header file for the TCR8TESTER application
//

#if !defined(AFX_TCR8TESTER_H__56F546E8_0543_4BE3_8DCF_40A873681ABD__INCLUDED_)
#define AFX_TCR8TESTER_H__56F546E8_0543_4BE3_8DCF_40A873681ABD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTCR8TesterApp:
// See TCR8Tester.cpp for the implementation of this class
//

class CTCR8TesterApp : public CWinApp
{
public:
	CTCR8TesterApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTCR8TesterApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTCR8TesterApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TCR8TESTER_H__56F546E8_0543_4BE3_8DCF_40A873681ABD__INCLUDED_)

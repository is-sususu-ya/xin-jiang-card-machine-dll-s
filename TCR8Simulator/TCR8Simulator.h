// TCR8Simulator.h : main header file for the TCR8SIMULATOR application
//

#if !defined(AFX_TCR8SIMULATOR_H__D305288F_9F0D_4F11_A49C_C4AF5F551F32__INCLUDED_)
#define AFX_TCR8SIMULATOR_H__D305288F_9F0D_4F11_A49C_C4AF5F551F32__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTCR8SimulatorApp:
// See TCR8Simulator.cpp for the implementation of this class
//

class CTCR8SimulatorApp : public CWinApp
{
public:
	CTCR8SimulatorApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTCR8SimulatorApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTCR8SimulatorApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TCR8SIMULATOR_H__D305288F_9F0D_4F11_A49C_C4AF5F551F32__INCLUDED_)

// SetBoxInfo.cpp : implementation file
//

#include "stdafx.h"
#include "tcr8tester.h"
#include "SetBoxInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSetBoxInfo dialog


CSetBoxInfo::CSetBoxInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CSetBoxInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSetBoxInfo)
	m_iBoxCapacity = 0;
	m_iCardCounter = 0;
	m_dwBoxSN = 0;
	//}}AFX_DATA_INIT
}


void CSetBoxInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSetBoxInfo)
	DDX_Text(pDX, IDC_EDIT_BOXCAP, m_iBoxCapacity);
	DDV_MinMaxInt(pDX, m_iBoxCapacity, 500, 1000);
	DDX_Text(pDX, IDC_EDIT_CARDCNT, m_iCardCounter);
	DDV_MinMaxInt(pDX, m_iCardCounter, 0, 1000);
	DDX_Text(pDX, IDC_EDIT_BOXSN, m_dwBoxSN);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetBoxInfo, CDialog)
	//{{AFX_MSG_MAP(CSetBoxInfo)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetBoxInfo message handlers

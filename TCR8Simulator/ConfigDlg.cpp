// ConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TCR8Simulator.h"
#include "ConfigDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog


CConfigDlg::CConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigDlg)
	m_nOddsEject = 0;
	m_nOddsOutbox = 0;
	m_tmaxEject = 0;
	m_tminEject = 0;
	m_tmaxOutbox = 0;
	m_tminOutbox = 0;
	m_nMaskPeriod = 0;
	m_nSpeed = -1;
	//}}AFX_DATA_INIT
}


void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigDlg)
	DDX_Control(pDX, IDC_SPIN_MASKPERIOD, m_spinMaskPeriod);
	DDX_Control(pDX, IDC_SPIN_TISSUEMIN, m_spinTMINOutbox);
	DDX_Control(pDX, IDC_SPIN_TISSUEMAX, m_spinTMAXOutbox);
	DDX_Control(pDX, IDC_SPIN_TEJECTMIN, m_spinTMINEject);
	DDX_Control(pDX, IDC_SPIN_TEJECTMAX, m_spinTMAXEject);
	DDX_Text(pDX, IDC_EDIT_ODDSEJECT, m_nOddsEject);
	DDV_MinMaxUInt(pDX, m_nOddsEject, 1, 10000);
	DDX_Text(pDX, IDC_EDIT_ODDSOUTBOX, m_nOddsOutbox);
	DDV_MinMaxUInt(pDX, m_nOddsOutbox, 1, 10000);
	DDX_Text(pDX, IDC_EDIT_TEJECTMAX, m_tmaxEject);
	DDV_MinMaxUInt(pDX, m_tmaxEject, 0, 30);
	DDX_Text(pDX, IDC_EDIT_TEJECTMIN, m_tminEject);
	DDV_MinMaxUInt(pDX, m_tminEject, 0, 20);
	DDX_Text(pDX, IDC_EDIT_TISSUEMAX, m_tmaxOutbox);
	DDV_MinMaxUInt(pDX, m_tmaxOutbox, 0, 50);
	DDX_Text(pDX, IDC_EDIT_TISSUEMIN, m_tminOutbox);
	DDV_MinMaxUInt(pDX, m_tminOutbox, 0, 20);
	DDX_Text(pDX, IDC_EDIT_MASKPERIOD, m_nMaskPeriod);
	DDV_MinMaxUInt(pDX, m_nMaskPeriod, 0, 100);
	DDX_Radio(pDX, IDC_RADIO_9600, m_nSpeed);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigDlg)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED,IDC_RADIO_9600, IDC_RADIO_19200, OnRadioSpeed)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg message handlers

BOOL CConfigDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_spinTMINOutbox.SetRange(0,20);
	m_spinTMAXOutbox.SetRange(0,50);
	m_spinTMINEject.SetRange(0,20);
	m_spinTMAXEject.SetRange(0,20);
	m_spinMaskPeriod.SetRange(0,100);
	m_spinTMINOutbox.SetPos(m_tminOutbox);
	m_spinTMAXOutbox.SetPos(m_tmaxOutbox);
	m_spinTMINEject.SetPos(m_tminEject);
	m_spinTMAXEject.SetPos(m_tmaxEject);
	m_spinMaskPeriod.SetPos(m_nMaskPeriod);
	((CButton *)GetDlgItem(IDC_RADIO_9600)) ->SetCheck( m_nSpeed==0 ? BST_CHECKED : BST_UNCHECKED );
	((CButton *)GetDlgItem(IDC_RADIO_19200)) ->SetCheck( m_nSpeed==1 ? BST_CHECKED : BST_UNCHECKED );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData();
	CDialog::OnOK();
}

void CConfigDlg::OnRadioSpeed(UINT uID) 
{
	// TODO: Add your control notification handler code here
	m_nSpeed = uID==IDC_RADIO_9600 ? 9600 : 19200;
}

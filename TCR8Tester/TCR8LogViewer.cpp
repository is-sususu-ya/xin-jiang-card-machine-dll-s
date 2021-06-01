// TCR8LogViewer.cpp : implementation file
//

#include "stdafx.h"
#include "tcr8tester.h"
#include "TCR8LogViewer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTCR8LogViewer dialog


CTCR8LogViewer::CTCR8LogViewer(CWnd* pParent /*=NULL*/)
	: CDialog(CTCR8LogViewer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTCR8LogViewer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CTCR8LogViewer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTCR8LogViewer)
	DDX_Control(pDX, IDC_LIST_TCR8LOG, m_listTCR8Log);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTCR8LogViewer, CDialog)
	//{{AFX_MSG_MAP(CTCR8LogViewer)
	ON_BN_CLICKED(IDC_CHECK_PAUSEROLL, OnCheckPauseRoll)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTCR8LogViewer message handlers
BOOL CTCR8LogViewer::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	int nW;

	nW = m_listTCR8Log.GetStringWidth( "W" );

	m_listTCR8Log.InsertColumn( 0, "Time",	LVCFMT_CENTER,	 nW*7, -1 );
	m_listTCR8Log.InsertColumn( 1, "TCR8Log",LVCFMT_LEFT, nW*32, -1 );
	m_listTCR8Log.SetExtendedStyle( LVS_EX_FULLROWSELECT );

	m_bPauseRoll = FALSE;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTCR8LogViewer::AppendTCR8Log(const char *txt, int len)
{
	char event_time[32];
	SYSTEMTIME tsys;

	GetLocalTime( &tsys );
	sprintf( event_time, "%02d:%02d:%02d.%03d", tsys.wHour, tsys.wMinute, tsys.wSecond, tsys.wMilliseconds );
	if( m_listTCR8Log.GetItemCount() >= 10000 )
	{
       m_listTCR8Log.DeleteItem( 0 );
	}   

	int n = m_listTCR8Log.InsertItem( m_listTCR8Log.GetItemCount(), event_time );
	m_listTCR8Log.SetItemText( n, 1, txt );

	if( !m_bPauseRoll )
	{
		m_listTCR8Log.EnsureVisible( m_listTCR8Log.GetItemCount()-1, FALSE );
	}
}

void CTCR8LogViewer::OnCheckPauseRoll() 
{
	// TODO: Add your control notification handler code here
	m_bPauseRoll = IS_CHECKED( IDC_CHECK_PAUSEROLL );

	if( !m_bPauseRoll )
	{
		m_listTCR8Log.EnsureVisible( m_listTCR8Log.GetItemCount()-1, FALSE );
	}
}

// ProtocolViewer.cpp : implementation file
//

#include "stdafx.h"
#include "tcr8tester.h"
#include "ProtocolViewer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProtocolViewer dialog


CProtocolViewer::CProtocolViewer(CWnd* pParent /*=NULL*/)
	: CDialog(CProtocolViewer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProtocolViewer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CProtocolViewer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProtocolViewer)
	DDX_Control(pDX, IDC_LIST_PROTOCOL, m_listProtocol);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProtocolViewer, CDialog)
	//{{AFX_MSG_MAP(CProtocolViewer)
	ON_BN_CLICKED(IDC_CHECK_PAUSEROLL, OnCheckPauseRoll)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProtocolViewer message handlers

BOOL CProtocolViewer::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	int nW;

	nW = m_listProtocol.GetStringWidth( "W" );

	m_listProtocol.InsertColumn( 0, "Time",	LVCFMT_CENTER,	 nW*7, -1 );
	m_listProtocol.InsertColumn( 1, "Dir",LVCFMT_LEFT, nW*3, -1 );
	m_listProtocol.InsertColumn( 2, "DATA",LVCFMT_LEFT, nW*24, -1 );
	m_listProtocol.SetExtendedStyle( LVS_EX_FULLROWSELECT );

	m_bPauseRoll = FALSE;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CProtocolViewer::AppendProtocol(char *pPacket, BOOL bIsTx, int len)
{
	char event_time[32];
	SYSTEMTIME tsys;

	len = len;
	GetLocalTime( &tsys );
	sprintf( event_time, "%02d:%02d:%02d.%03d", tsys.wHour, tsys.wMinute, tsys.wSecond, tsys.wMilliseconds );
	if( m_listProtocol.GetItemCount() >= 10000 )
	{
       m_listProtocol.DeleteItem( 0 );
	}   

	int n = m_listProtocol.InsertItem( m_listProtocol.GetItemCount(), event_time );
	m_listProtocol.SetItemText( n, 1, bIsTx ? "Tx" : "Rx" );

	m_listProtocol.SetItemText( n, 2, pPacket );

	if( !m_bPauseRoll )
	{
		m_listProtocol.EnsureVisible( m_listProtocol.GetItemCount()-1, FALSE );
	}
}

void CProtocolViewer::OnCheckPauseRoll() 
{
	// TODO: Add your control notification handler code here
	m_bPauseRoll = IS_CHECKED( IDC_CHECK_PAUSEROLL );

	if( !m_bPauseRoll )
	{
		m_listProtocol.EnsureVisible( m_listProtocol.GetItemCount()-1, FALSE );
	}
}

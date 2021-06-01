// TCR8TesterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TCR8Tester.h"
#include "TCR8TesterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TCR8LOG_PATH	"D:/rwlog/RunwellTCR8Lib.log"

#define BOX_CAPCITY	800
/////////////////////////////////////////////////////////////////////////////
// CTCR8TesterDlg dialog
#define WM_TCR8EVENT	WM_USER + 1

int m_iTwoPacket = 0;

void TCR8_EventHandle( void *hMachine, int nEventId, int nParam )
{
	TCR8HANDLE hTCR8 = (TCR8HANDLE)hMachine;
	TCR8UserData *pUserData = (TCR8UserData *)hTCR8->m_pUsrData;

	if( nParam != NULL )
	{
		int len;

		switch(nEventId)
		{
		case EVID_TX:
			len = strlen( (char *)nParam );
			memcpy( pUserData->TxData, (char *)nParam, len+1 );
			break;

		case EVID_RX:
			len = strlen( (char *)nParam );
			memcpy( pUserData->RxData, (char *)nParam, len+1 );
			break;

		case EVID_KERNELLOG:
			len = strlen( (char *)nParam );
			memcpy( pUserData->TCR8Log, (char *)nParam, len+1 );
			break;

		}
	}

	PostMessage( pUserData->hWnd, pUserData->Msg, nEventId, nParam );
}

CTCR8TesterDlg::CTCR8TesterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTCR8TesterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTCR8TesterDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTCR8TesterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTCR8TesterDlg)
	DDX_Control(pDX, IDC_PROGRESS_BOX4, m_ProgressBox4);
	DDX_Control(pDX, IDC_PROGRESS_BOX3, m_ProgressBox3);
	DDX_Control(pDX, IDC_PROGRESS_BOX2, m_ProgressBox2);
	DDX_Control(pDX, IDC_PROGRESS_BOX1, m_ProgressBox1);
	DDX_Control(pDX, IDC_LIST_EVT, m_ListEvent);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTCR8TesterDlg, CDialog)
	//{{AFX_MSG_MAP(CTCR8TesterDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPENCOM, OnButtonOpencom)
	ON_BN_CLICKED(IDC_BUTTON_CLOSECOM, OnButtonClosecom)
	ON_BN_CLICKED(IDC_BUTTON_RUN, OnButtonRun)
	ON_BN_CLICKED(IDC_BUTTON_SUSPEND, OnButtonSuspend)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_EJECT, OnButtonEject)
	ON_BN_CLICKED(IDC_BUTTON_RECYCLE, OnButtonRecycle)
	ON_BN_CLICKED(IDC_BUTTON_CANCELBUTTON, OnButtonCancelbutton)
	ON_BN_CLICKED(IDC_BUTTON_SWITCH_CHENNAL, OnButtonSwitchChennal)
	ON_BN_CLICKED(IDC_BUTTON_SWITCH_ANT, OnButtonSwitchAnt)
	ON_BN_CLICKED(IDC_BUTTON_SETBOXINFO, OnButtonSetboxinfo)
	ON_BN_CLICKED(IDC_BUTTON_VIEWPROTOCOL, OnButtonViewprotocol)
	ON_BN_CLICKED(IDC_BUTTON_VIEWTCR8LOG, OnButtonViewtcr8log)
	ON_BN_CLICKED(IDC_BUTTON_GETFIRMVER, OnButtonGetfirmver)
	ON_BN_CLICKED(IDC_RADIO_DISPENSER, OnRadioDispenser)
	ON_BN_CLICKED(IDC_RADIO_COLLECTOR, OnRadioCollector)
	ON_BN_CLICKED(IDC_BUTTON_TRIGGERBUTTON, OnButtonTriggerButton)
	ON_BN_CLICKED(IDC_BUTTON_PULLBACK, OnButtonPullBack)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED,IDC_RADIO_9600, IDC_RADIO_19200, OnButtonBaudSelect)
	ON_CONTROL_RANGE(BN_CLICKED,IDC_RADIO_CH1, IDC_RADIO_CH4, OnButtonOperateChannelSelect)
	ON_MESSAGE( WM_TCR8EVENT, OnTCR8Event )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTCR8TesterDlg message handlers
//ON_BN_CLICKED(IDC_BUTTON_GETFIRMVER, OnButtonGerFirmVer)

BOOL CTCR8TesterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	m_nOperateChannel = -1;

	m_dlgProtocolViewer.Create( CProtocolViewer::IDD );
	m_dlgProtocolViewer.ShowWindow( SW_HIDE );
	m_dlgTCR8LogViewer.Create( CTCR8LogViewer::IDD );
	m_dlgTCR8LogViewer.ShowWindow( SW_HIDE );

	EnableControls(FALSE);

	int nW = m_ListEvent.GetStringWidth( "W" );
	m_ListEvent.InsertColumn( 0, "Time",	LVCFMT_CENTER,	 nW*14, -1 );
	m_ListEvent.InsertColumn( 1, "Event",LVCFMT_LEFT, nW*32, -1 );
	m_ListEvent.SetExtendedStyle( LVS_EX_FULLROWSELECT );

	m_ProgressBox1.SetRange( 0, BOX_CAPCITY );
	m_ProgressBox2.SetRange( 0, BOX_CAPCITY );
	m_ProgressBox3.SetRange( 0, BOX_CAPCITY );
	m_ProgressBox4.SetRange( 0, BOX_CAPCITY );

	for( nW = 1; nW < 5; nW++ )
	{
		OnBoxUnload( nW );
		ShowCardOnReader( nW, FALSE );
		ShowCardOnExit( nW, FALSE );
		SetChennalState( nW, FALSE );
	}

	m_nBaudRate = 19200;
	SET_CHECK( IDC_RADIO_19200, BST_CHECKED );
	SET_CHECK( IDC_RADIO_DISPENSER, BST_CHECKED );

	m_hTCR8 = TCR8_Create();

	if( m_hTCR8 == NULL )
	{
		AfxMessageBox( "无法创建卡机对象, 程序停止运行!" );
		PostMessage( WM_COMMAND, IDOK, 0 );
	}
	else
	{
		TCR8_EnableLog( m_hTCR8, TCR8LOG_PATH );
		TCR8_Log( m_hTCR8, "[Note] Create TCR8 Object Success.\n" );

		m_TCR8UserData.hWnd = m_hWnd;
		m_TCR8UserData.Msg  = WM_TCR8EVENT;
		_SetUserData( m_hTCR8, &m_TCR8UserData );
		_SetMachineType( m_hTCR8, 0 );

		TCR8_SetCallback( m_hTCR8, TCR8_EventHandle );
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTCR8TesterDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTCR8TesterDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTCR8TesterDlg::EnableControls(BOOL bEnable)
{
	ENABLE_CONTROL( CButton, IDC_BUTTON_CLOSECOM, bEnable );
	ENABLE_CONTROL( CButton, IDC_BUTTON_RUN, bEnable );
	ENABLE_CONTROL( CButton, IDC_BUTTON_SUSPEND, bEnable );
}

void CTCR8TesterDlg::OnTCR8Event(WPARAM wParam, LPARAM lParam)
{
	int nEventId = (int)wParam;
	CString strEvent;
	
//	if( nEventId != EVID_TX && nEventId != EVID_RX )
	{
		strEvent = TCR8_GetEventText( nEventId, lParam );
		AppendEventText( strEvent );
	}

	if( EVID_RX == nEventId && lParam != NULL )
	{
		if( *(m_TCR8UserData.RxData + 2) == 'E' )
		{
			m_iTwoPacket ++;
		}

		if( *(m_TCR8UserData.RxData + 2) == 'D' )
		{
			m_iTwoPacket = 0;
		}

		if( m_iTwoPacket == 2 )
		{
			m_iTwoPacket = 0;
		}
	}

	switch( nEventId )
	{
	case EVID_POWERON:     // 卡机上电
	case EVID_LINK:        // 卡机连线
	case EVID_OUTCARDFAIL: // 出卡失败
	case EVID_RECYCLEOK:   // 回收卡成功
	case EVID_RECYCLEFAIL: // 回收卡失败
	case EVID_PRESSPOLKEY: // 军警键按下
	case EVID_RWTIMEOUT:   // 读写超时
	case EVID_THREADEXIT:  // 卡机工作线程异常退出	
	case EVID_COLLECTOK:
	case EVID_COLLECTFAIL:
		break;

	case EVID_PRESSKEY:    // 司机按键
		UpdateActiveChannel( _GetActive(m_hTCR8,0), _GetActive(m_hTCR8,1) );
		break;

	case EVID_TX:          // 发送协议帧
		if( lParam != NULL )
		{
			AppendPacket( m_TCR8UserData.TxData, TRUE );
		}
		break;

	case EVID_RX:  // 收到协议帧
		if( lParam != NULL )
		{
			AppendPacket( m_TCR8UserData.RxData, FALSE );
		}
		break;

	case EVID_OUTCARDOK:   // 出卡成功
		ShowCardOnExit( lParam, TRUE );
		break;

	case EVID_TAKECARD:    // 卡取走
		ShowCardOnExit( lParam, FALSE );
		break;

	case EVID_STATECHANGE: // 卡机状态变化
		{
			SetChennalState( lParam, !(_IsChannelFail( m_hTCR8, lParam-1 ) || _IsChannelOffline( m_hTCR8, lParam-1 )) );
			ShowCardOnReader( lParam, _IsCardOnAntenna( m_hTCR8, lParam-1 ) );
			UpdateActiveChannel( _GetActive(m_hTCR8,0), _GetActive(m_hTCR8,1) );
		}
		break;

	case EVID_COUNTCHANGE: // 卡机卡数变化
		UpdateCardCount( lParam, _GetChannelCount( m_hTCR8, lParam-1 ) - (_IsCardOnAntenna( m_hTCR8, lParam-1 ) ? 1 : 0 ) );
		break;

	case EVID_BUTTONIGNRD: // 按键忽略
		break;

	case EVID_BOXLOAD:     // 卡盒装上
		UpdateBoxSN( lParam, _GetCartridgeSN( m_hTCR8, lParam-1 ) );
		break;

	case EVID_BOXUNLOAD:  // 卡盒卸下
		OnBoxUnload( lParam );
		break;

	case EVID_SNCHANGED:  // 卡盒SN变化
		UpdateBoxSN( lParam, _GetCartridgeSN( m_hTCR8, lParam-1 ) );
		break;

	case EVID_KERNELLOG:  // 收到核心板日志
		AppendTCR8Log( m_TCR8UserData.TCR8Log );
		break;
	}
}

void CTCR8TesterDlg::AppendEventText(LPCTSTR txt)
{
	char event_time[32];
	SYSTEMTIME tsys;

	GetLocalTime( &tsys );
	sprintf( event_time, "%02d:%02d:%02d.%03d", tsys.wHour, tsys.wMinute, tsys.wSecond, tsys.wMilliseconds );
	if( m_ListEvent.GetItemCount() >= 10000 )
	{
       m_ListEvent.DeleteItem( 0 );
	}   

	int n = m_ListEvent.InsertItem(m_ListEvent.GetItemCount(), event_time);
	m_ListEvent.SetItemText(n, 1, txt );
	m_ListEvent.EnsureVisible(m_ListEvent.GetItemCount()-1, FALSE);
}

void CTCR8TesterDlg::OnButtonOpencom() 
{
	// TODO: Add your control notification handler code here
	int nCOM = GET_CONTROL( CComboBox, IDC_COMBO_COMNUM ) ->GetCurSel();

	if( nCOM == -1 )
	{
		AfxMessageBox("请选择一个串口!");
		return;
	}

	TCR8_SetComPort( m_hTCR8, nCOM+1, m_nBaudRate );

	if( !TCR8_OpenDevice( m_hTCR8 ) )
	{
		AfxMessageBox("打开卡机失败!");
		return;
	}

	ENABLE_CONTROL( CButton, IDC_BUTTON_OPENCOM, FALSE );
	ENABLE_CONTROL( CComboBox, IDC_COMBO_COMNUM, FALSE );
	ENABLE_CONTROL( CButton, IDC_RADIO_9600, FALSE );
	ENABLE_CONTROL( CButton, IDC_RADIO_19200, FALSE );
	
	ENABLE_CONTROL( CButton, IDC_BUTTON_CLOSECOM, TRUE );
	ENABLE_CONTROL( CButton, IDC_BUTTON_RUN, TRUE );
//	ENABLE_CONTROL( CButton, IDC_BUTTON_SUSPEND, TRUE );
}

void CTCR8TesterDlg::OnButtonClosecom() 
{
	// TODO: Add your control notification handler code here
	if( !TCR8_CloseDevice( m_hTCR8 ) )
	{
		AfxMessageBox("关闭卡机失败!");
		return;
	}

	ENABLE_CONTROL( CButton, IDC_BUTTON_OPENCOM, TRUE );
	ENABLE_CONTROL( CComboBox, IDC_COMBO_COMNUM, TRUE );
	ENABLE_CONTROL( CButton, IDC_RADIO_9600, TRUE );
	ENABLE_CONTROL( CButton, IDC_RADIO_19200, TRUE );
	
	ENABLE_CONTROL( CButton, IDC_BUTTON_CLOSECOM, FALSE );
	ENABLE_CONTROL( CButton, IDC_BUTTON_RUN, FALSE );
	ENABLE_CONTROL( CButton, IDC_BUTTON_SUSPEND, FALSE );
}

void CTCR8TesterDlg::OnButtonBaudSelect(UINT uID)
{
	m_nBaudRate = uID == IDC_RADIO_19200 ? 19200 : 9600;
}

void CTCR8TesterDlg::OnButtonOperateChannelSelect(UINT uID)
{
	m_nOperateChannel = uID - IDC_RADIO_CH1 + 1;
}

void CTCR8TesterDlg::OnButtonRun() 
{
	// TODO: Add your control notification handler code here
	if( !TCR8_Run( m_hTCR8 ) )
	{
		AfxMessageBox("运行卡机线程失败!");
		return;
	}
	ENABLE_CONTROL( CButton, IDC_BUTTON_RUN, FALSE );
	ENABLE_CONTROL( CButton, IDC_BUTTON_SUSPEND, TRUE );
}

void CTCR8TesterDlg::OnButtonSuspend() 
{
	// TODO: Add your control notification handler code here
	if( !TCR8_Suspend( m_hTCR8 ) )
	{
		AfxMessageBox("挂起卡机线程失败!");
		return;
	}

	ENABLE_CONTROL( CButton, IDC_BUTTON_RUN, TRUE );
	ENABLE_CONTROL( CButton, IDC_BUTTON_SUSPEND, FALSE );
}

void CTCR8TesterDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	TCR8_CloseDevice( m_hTCR8 );
	TCR8_Destroy( m_hTCR8 );
}

void CTCR8TesterDlg::UpdateBoxSN(int iBoxNum, DWORD dwSN)
{
	if( 0 < iBoxNum && iBoxNum < 5 )
	{
		CString strSN;

		iBoxNum --;

		strSN.Format( "%d", dwSN );
		SET_WNDTEXT( IDC_EDIT_BOX1SN + iBoxNum, strSN );
	}
}

void CTCR8TesterDlg::UpdateCardCount(int iBoxNum, int iCount)
{
	if( 0 < iBoxNum && iBoxNum < 5 )
	{
		iBoxNum --;

		switch(iBoxNum)
		{
		case 0:
			m_ProgressBox1.SetPos( iCount );
			break;

		case 1:
			m_ProgressBox2.SetPos( iCount );
			break;

		case 2:
			m_ProgressBox3.SetPos( iCount );
			break;

		case 3:
			m_ProgressBox4.SetPos( iCount );
			break;

		default:
			break;
		}
	}
}

void CTCR8TesterDlg::ShowCardOnReader(int nChennal, BOOL bShow)
{
	CStatic *pStatic;
	HBITMAP hBitmap;
	
	if( 0 < nChennal && nChennal < 5 )
	{
		nChennal --;
		pStatic=(CStatic *)GetDlgItem(IDC_STATIC_READER1 + nChennal);
		pStatic->SetWindowText( "" );

		pStatic->ModifyStyle(0xF,SS_BITMAP|SS_CENTERIMAGE);
		hBitmap=::LoadBitmap(AfxGetApp()->m_hInstance, MAKEINTRESOURCE( bShow ? IDB_BITMAP_CARDONREADER : IDB_BITMAP_READER));
		pStatic->SetBitmap(hBitmap);
	}
}

void CTCR8TesterDlg::ShowCardOnExit(int nChennal, BOOL bShow)
{
	CStatic *pStatic;
	HBITMAP hBitmap;
	
	if( 0 < nChennal && nChennal < 5 )
	{
		nChennal --;
		pStatic=(CStatic *)GetDlgItem(IDC_STATIC_EXIT1 + nChennal);
		pStatic->SetWindowText( "" );

		pStatic->ModifyStyle(0xF,SS_BITMAP|SS_CENTERIMAGE);
		hBitmap=::LoadBitmap(AfxGetApp()->m_hInstance, MAKEINTRESOURCE( bShow ? IDB_BITMAP_CARDONEXIT : IDB_BITMAP_EXIT));
		pStatic->SetBitmap(hBitmap);
	}
}

void CTCR8TesterDlg::SetChennalState(int nChennal, BOOL bNormal)
{
	CStatic *pStatic;
	HBITMAP hBitmap;
	
	if( 0 < nChennal && nChennal < 5 )
	{
		nChennal --;
		pStatic=(CStatic *)GetDlgItem(IDC_STATIC_CH1 + nChennal);
		pStatic->SetWindowText( "" );

		pStatic->ModifyStyle(0xF,SS_BITMAP|SS_CENTERIMAGE);

		switch(nChennal)
		{
		case 0:
			hBitmap=::LoadBitmap(AfxGetApp()->m_hInstance, MAKEINTRESOURCE( bNormal ? IDB_BITMAP_OK1 : IDB_BITMAP_NG1 ) );
			break;

		case 1:
			hBitmap=::LoadBitmap(AfxGetApp()->m_hInstance, MAKEINTRESOURCE( bNormal ? IDB_BITMAP_OK2 : IDB_BITMAP_NG2 ) );
			break;

		case 2:
			hBitmap=::LoadBitmap(AfxGetApp()->m_hInstance, MAKEINTRESOURCE( bNormal ? IDB_BITMAP_OK3 : IDB_BITMAP_NG3 ) );
			break;

		case 3:
			hBitmap=::LoadBitmap(AfxGetApp()->m_hInstance, MAKEINTRESOURCE( bNormal ? IDB_BITMAP_OK4 : IDB_BITMAP_NG4 ) );
			break;
		}
		
		pStatic->SetBitmap(hBitmap);
	}
}

void CTCR8TesterDlg::ClearBoxSN(int iBoxNum)
{
	if( 0 < iBoxNum && iBoxNum < 5 )
	{
		iBoxNum --;

		SET_WNDTEXT( IDC_EDIT_BOX1SN + iBoxNum, "" );
	}
}

void CTCR8TesterDlg::OnBoxUnload(int iBoxNum)
{
	ClearBoxSN( iBoxNum );

	if( 0 < iBoxNum && iBoxNum < 5 )
	{
		iBoxNum --;
		
		switch( iBoxNum )
		{
		case 0:
			m_ProgressBox1.ClearProgressBar();
			break;

		case 1:
			m_ProgressBox2.ClearProgressBar();
			break;

		case 2:
			m_ProgressBox3.ClearProgressBar();
			break;

		case 3:
			m_ProgressBox4.ClearProgressBar();
			break;
		}
	}
}

void CTCR8TesterDlg::OnButtonEject() 
{
	// TODO: Add your control notification handler code here
	if( _IsOnline( m_hTCR8 ) )
	{
		if( _IsCollector( m_hTCR8 ) )
			TCR8_CollectCard( m_hTCR8, 0 ); // 默认通道收卡
		else
			TCR8_EjectCard( m_hTCR8, 0 ); // 默认通道发卡
	}
	else
	{
		AfxMessageBox( _IsCollector( m_hTCR8 ) ? "卡机离线，不允许收卡操作!" : "卡机离线，不允许发卡操作!");
	}
}

void CTCR8TesterDlg::OnButtonRecycle() 
{
	// TODO: Add your control notification handler code here
	if( _IsOnline( m_hTCR8 ) )
	{
		TCR8_RecycleCard( m_hTCR8 );
	}
	else
	{
		AfxMessageBox("卡机离线，不允许回收卡操作!");
	}
}

void CTCR8TesterDlg::OnButtonCancelbutton() 
{
	// TODO: Add your control notification handler code here
	if( _IsOnline( m_hTCR8 ) )
	{
		if( _IsCollector( m_hTCR8 ) )
			TCR8_ReturnCard( m_hTCR8, 0 );
		else
			TCR8_CancelButton( m_hTCR8 );
	}
	else
	{
		AfxMessageBox( _IsCollector( m_hTCR8 ) ? "卡机离线，不允许退卡操作!" : "卡机离线，不允许取消按键操作!");
	}
}

void CTCR8TesterDlg::OnButtonSwitchChennal() 
{
	// TODO: Add your control notification handler code here
	
	if( !_IsOnline( m_hTCR8 ) )
	{
		AfxMessageBox("卡机离线，不允许通道切换操作!");
	}
	else if( m_nOperateChannel == -1 )
	{
		AfxMessageBox("请先选择操作通道!");
	}
	else
	{
		TCR8_SwitchChannel( m_hTCR8, m_nOperateChannel );
		SET_CHECK( (IDC_RADIO_CH1 + m_nOperateChannel - 1), FALSE );
		m_nOperateChannel = -1;
	}
}

void CTCR8TesterDlg::OnButtonSwitchAnt() 
{
	// TODO: Add your control notification handler code here
	if( !_IsOnline( m_hTCR8 ) )
	{
		AfxMessageBox("卡机离线，不允许天线切换操作!");
	}
	else if( m_nOperateChannel == -1 )
	{
		AfxMessageBox("请先选择操作通道!");
	}
	else
	{
		TCR8_SwitchAntenna( m_hTCR8, m_nOperateChannel );
		SET_CHECK( IDC_RADIO_CH1 + m_nOperateChannel - 1, FALSE );
		m_nOperateChannel = -1;
	}
}

void CTCR8TesterDlg::OnButtonSetboxinfo() 
{
	// TODO: Add your control notification handler code here
	if( !_IsOnline( m_hTCR8 ) )
	{
		AfxMessageBox("卡机离线，不允许设置卡盒信息操作!");
	}
	else if( m_nOperateChannel == -1 )
	{
		AfxMessageBox("请先选择操作通道!");
	}
	else
	{
		m_dlgSetBoxInfo.m_dwBoxSN      = _GetCartridgeSN( m_hTCR8, m_nOperateChannel - 1 );
		m_dlgSetBoxInfo.m_iCardCounter = _GetChannelCount( m_hTCR8, m_nOperateChannel - 1 ) - (_IsCardOnAntenna( m_hTCR8, m_nOperateChannel-1 ) ? 1 : 0 );
		m_dlgSetBoxInfo.m_iBoxCapacity = 500;

		if( m_dlgSetBoxInfo.DoModal() == IDOK )
		{
			TCR8_SetCartridgeInfo( m_hTCR8, m_nOperateChannel, m_dlgSetBoxInfo.m_dwBoxSN, m_dlgSetBoxInfo.m_iCardCounter );
		}
		SET_CHECK( IDC_RADIO_CH1 + m_nOperateChannel - 1, FALSE );
		m_nOperateChannel = -1;
	}
}

void CTCR8TesterDlg::AppendPacket(char *pPacket, BOOL bTx)
{
	m_dlgProtocolViewer.AppendProtocol( pPacket, bTx, 0 );
}

void CTCR8TesterDlg::OnButtonViewprotocol() 
{
	// TODO: Add your control notification handler code here
	m_dlgProtocolViewer.ShowWindow( SW_SHOW );
}

void CTCR8TesterDlg::UpdateActiveChannel(int iUpper, int iLower)
{
	CStatic *pStatic;
	HBITMAP hBitmap;
	int i;
	
	hBitmap=::LoadBitmap( AfxGetApp()->m_hInstance, MAKEINTRESOURCE( IDB_BITMAP_ACTIVE ) );

	for( i = 0; i < 4; i++ )
	{
		pStatic=(CStatic *)GetDlgItem(IDC_STATIC_UPPER1 + i);
		pStatic->ModifyStyle( SS_BITMAP|SS_CENTERIMAGE, 0 );
		pStatic->SetWindowText( "" );
	}

	pStatic=(CStatic *)GetDlgItem(IDC_STATIC_UPPER1 + iUpper-1);
	pStatic->ModifyStyle( 0xF, SS_BITMAP|SS_CENTERIMAGE );
	pStatic->SetBitmap(hBitmap);

	pStatic=(CStatic *)GetDlgItem(IDC_STATIC_UPPER1 + iLower-1);
	pStatic->ModifyStyle( 0xF, SS_BITMAP|SS_CENTERIMAGE );
	pStatic->SetBitmap(hBitmap);
	
}

void CTCR8TesterDlg::AppendTCR8Log(LPCTSTR txt)
{
	m_dlgTCR8LogViewer.AppendTCR8Log( txt, 0 );
}

void CTCR8TesterDlg::OnButtonViewtcr8log() 
{
	// TODO: Add your control notification handler code here
	m_dlgTCR8LogViewer.ShowWindow( SW_SHOW );
}
/*
void CTCR8TesterDlg::OnButtonGerFirmVer()
{

}
*/
long CTCR8TesterDlg::VersionCode( const char * cCode )
{
	long	vCode = 0L;
	const char	*p = cCode;
	int		nByte;

	for( nByte = 0; nByte < 4; nByte ++ )
	{
		vCode <<= 8;
		vCode += (*p - '0') * 10 + *(p+1)-'0'; 
		p += 2;
		if ( *p == '.' ) 
			p++;
	}
	return vCode;
}

void CTCR8TesterDlg::OnButtonGetfirmver() 
{
	// TODO: Add your control notification handler code here
	DWORD ver;
	char *ptr = "00.02.01.03";
	char buf[16];

	ver = VersionCode( ptr );
//	ver = _GetKernelVersion();

	_snprintf( buf, 16, "%08x", ver );

	SET_WNDTEXT( IDC_EDIT_FIRM_VER, buf );
	
}

void CTCR8TesterDlg::OnRadioDispenser() 
{
	// TODO: Add your control notification handler code here
	CButton *pBtn;

	_SetMachineType( m_hTCR8, 0 );
	pBtn = (CButton *)GetDlgItem( IDC_BUTTON_EJECT );
	if( pBtn != NULL )
	{
		pBtn->SetWindowText( "发卡" );
	}

	pBtn = (CButton *)GetDlgItem( IDC_BUTTON_CANCELBUTTON );
	if( pBtn != NULL )
	{
		pBtn->SetWindowText( "取消按键" );
	}
}

void CTCR8TesterDlg::OnRadioCollector() 
{
	// TODO: Add your control notification handler code here
	CButton *pBtn;

	_SetMachineType( m_hTCR8, 1 );

	pBtn = (CButton *)GetDlgItem( IDC_BUTTON_EJECT );
	if( pBtn != NULL )
	{
		pBtn->SetWindowText( "收卡" );
	}

	pBtn = (CButton *)GetDlgItem( IDC_BUTTON_CANCELBUTTON );
	if( pBtn != NULL )
	{
		pBtn->SetWindowText( "退卡" );
	}
}

void CTCR8TesterDlg::OnButtonTriggerButton() 
{
	// TODO: Add your control notification handler code here
	if( !_IsOnline( m_hTCR8 ) )
	{
		AfxMessageBox("卡机离线，不允触发按键操作!");
	}
	else if( _IsCollector( m_hTCR8 ) )
	{
		AfxMessageBox("当前为收卡机，不允触发按键操作!");
	}
	else if( m_nOperateChannel == -1 )
	{
		AfxMessageBox("请先选择操作通道!");
	}
	else
	{
		TCR8_TriggerButton( m_hTCR8, m_nOperateChannel );
		SET_CHECK( (IDC_RADIO_CH1 + m_nOperateChannel - 1), FALSE );
		m_nOperateChannel = -1;
	}
}

void CTCR8TesterDlg::OnButtonPullBack() 
{
	// TODO: Add your control notification handler code here
	if( !_IsOnline( m_hTCR8 ) )
	{
		AfxMessageBox("卡机离线，不允拉回卡口卡操作!");
	}
	else if( _IsCollector( m_hTCR8 ) )
	{
		AfxMessageBox("当前为收卡机，不允拉回卡口卡操作!");
	}
	else
	{
		if( m_nOperateChannel == -1 )
		{
			TCR8_PullBackToAnt( m_hTCR8, 0 );
		}
		else
		{
			TCR8_PullBackToAnt( m_hTCR8, m_nOperateChannel );
			SET_CHECK( (IDC_RADIO_CH1 + m_nOperateChannel - 1), FALSE );
			m_nOperateChannel = -1;
		}
	}
}

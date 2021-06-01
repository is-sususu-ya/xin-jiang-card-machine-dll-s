// TCR8SimulatorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TCR8Simulator.h"
#include "TCR8SimulatorDlg.h"
#include "ConfigDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Timer ID and Time constant
#define TID_LOOPING			1
#define TMS_LOOPING			200
#define TID_UNMASKBUTTON	2
#define TMS_UNMASKBUTTON	3000
#define TID_STATUSPACKET	3
#define TMS_STATUSPACKET	3000
#define TID_CSCRWLIMIT		4
#define TMS_CSCRWLIMIT		10000

// channel dependant. Each ID shall be 4'th multiple
#define TID_MIN				100
#define TID_MAX				120
#define TID_OUTBOXOK		100	
#define TID_OUTBOXFAIL		104
#define TMIN_OUTBOX			1000
#define TMAX_OUTBOX			2000
#define TID_RECYCLE			108
#define TMIN_RECYCLE		500
#define TMAX_RECYCLE		1000
#define TID_EJECTOK			112
#define TID_EJECTFAIL		116
#define TMIN_EJECT			1000
#define TMAX_EJECT			2000

#define EXPECT_NONE	0		// use random result
#define EXPECT_FAIL	1		// always fail
#define EXPECT_OK	2		// always OK

#define RWFAIL_LIMIT	2

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTCR8SimulatorDlg dialog

CTCR8SimulatorDlg::CTCR8SimulatorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTCR8SimulatorDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTCR8SimulatorDlg)
	m_bAutoRecycle = FALSE;
	m_nCH2OP = -1;
	m_nActiveLower = -1;
	m_nActiveUpper = -1;
	m_bAutoEject = -1;
	m_bAutoPrepare = -1;
	m_bRadomFail = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_bAutoAck = TRUE;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTCR8SimulatorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTCR8SimulatorDlg)
	DDX_Control(pDX, IDC_LIST_PROTOCOL, m_ctrlList);
	DDX_Check(pDX, IDC_CHECK_AUTORECYCLE, m_bAutoRecycle);
	DDX_Radio(pDX, IDC_RADIO_CH1, m_nCH2OP);
	DDX_Radio(pDX, IDC_RADIO_WORKDOWN1, m_nActiveLower);
	DDX_Radio(pDX, IDC_RADIO_WORKUP1, m_nActiveUpper);
	DDX_Radio(pDX, IDC_RADIO_AUTOEJECT0, m_bAutoEject);
	DDX_Radio(pDX, IDC_RADIO_AUTOPREPARE0, m_bAutoPrepare);
	DDX_Check(pDX, IDC_CHECK_RANDOMFAIL, m_bRadomFail);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTCR8SimulatorDlg, CDialog)
	//{{AFX_MSG_MAP(CTCR8SimulatorDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, OnButtonOpen)
	ON_BN_CLICKED(IDC_CHECK_POWER, OnCheckPower)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoadBox)
	ON_BN_CLICKED(IDC_BUTTON_UNLOAD, OnButtonUnloadBox)
	ON_BN_CLICKED(IDC_BUTTON_PREPARE, OnButtonPrepareCard)
	ON_BN_CLICKED(IDC_BUTTON_OUTBOXFAIL, OnButtonOutboxFailed)
	ON_BN_CLICKED(IDC_BUTTON_RECYCLE, OnButtonRecycleCard)
	ON_BN_CLICKED(IDC_BUTTON_EJECT, OnButtonEjectCard)
	ON_BN_CLICKED(IDC_BUTTON_EJECTFAIL, OnButtonEjectFail)
	ON_BN_CLICKED(IDC_BUTTON_OFFLINE, OnButtonOnOffLine)
	ON_BN_CLICKED(IDC_BUTTON_TAKEN, OnButtonTakeCard)
	ON_BN_CLICKED(IDC_BUTTON_SENDLOG, OnButtonSendLog)
	ON_BN_CLICKED(IDC_BUTTON_SENDRAW, OnButtonSendRaw)
	ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_COMBO_COM, OnSelchangeComboCOM)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, OnButtonClose)
	ON_BN_CLICKED(IDC_CHECK_PAUSE, OnCheckPause)
	ON_BN_CLICKED(IDC_BUTTON_RESETFAULT, OnButtonResetFault)
	ON_BN_CLICKED(IDC_CHECK_STOPAUTOACK, OnButtonAutoCheck)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_CONFIG, OnButtonConfig)		
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED, IDC_BUTTON_PRESS1, IDC_BUTTON_PRESS2, OnPressTakeCardButton)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_BUTTON_MP1, IDC_BUTTON_MP2, OnButtonMP)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO_CH1, IDC_RADIO_CH4, OnRadioChannel)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO_AUTOPREPARE0, IDC_RADIO_WORKDOWN2, OnClickOptions )
	ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_EDIT_CNT1, IDC_EDIT_CNT4, OnKillfocusEditCount )
	ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_EDIT_SN1, IDC_EDIT_SN4, OnKillfocusEditSN )
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECK_HASBOX1, IDC_CHECK_OFFLINE4, OnCheckChannelState)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTCR8SimulatorDlg system message handlers

BOOL CTCR8SimulatorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	int nW = m_ctrlList.GetStringWidth( "W" );
	m_ctrlList.InsertColumn( 0, "时间",		LVCFMT_CENTER,	nW*16, -1 );
	m_ctrlList.InsertColumn( 1, "核心板-->车道",	LVCFMT_LEFT,	nW*36, -1 );
	m_ctrlList.InsertColumn( 2, "车道-->核心板",	LVCFMT_LEFT,	nW*36, -1 );
	m_bExtendedMode = FALSE;
	m_bPaused = FALSE;
	m_nPort = 0;
	m_nSpeed = 19200;
	m_tty = -1;
	m_FSM = tsIdle;
	for( int i=0; i<4; i++ )
	{
		m_Count[i] = 0;
		m_SN[i] = 0;
		m_Cap[i] = 0;
		m_State[i] = 0;
	}
	m_uLoop = 0;
	m_tidStatusPacket = 0;
	m_bAutoRecycle = TRUE;
	m_nCH2OP = 0;
	m_bAutoEject = TRUE;
	m_bAutoPrepare = TRUE;
	m_nTransChannel = 0;
	m_nAntenna = 1;
	m_bMaskButton = FALSE;
	m_bPowerOn = FALSE;
	m_bWaitInit = TRUE;
	m_nOddsPrepareFail = 100;		// Default is 1/100
	m_nOddsEjectFail = 1000;		// Default is 1/1000
	m_tminOutBox = TMIN_OUTBOX;
	m_tmaxOutBox = TMAX_OUTBOX;
	m_tminRecycle = TMIN_RECYCLE;
	m_tmaxRecycle = TMAX_RECYCLE;
	m_tminEject = TMIN_EJECT;
	m_tmaxEject = TMAX_EJECT;
	m_tMaskPeriod = TMS_UNMASKBUTTON;
	m_nActiveUpper = 0;
	m_nActiveLower = 0;
	m_nSpeed = 19200;
	UpdateData(FALSE);
	UpdateControls();
	UpdateFSM();

	ENABLE_CWND( IDC_BUTTON_OPEN, FALSE );
	ENABLE_CWND( IDC_BUTTON_CLOSE, FALSE );
	ENABLE_CWND( IDC_CHECK_POWER, FALSE );
	
	// Create working thread
	if ( (m_pWorkThread = new CWorkThread( this )) != NULL )
	{
		if (!m_pWorkThread->CreateThread(CREATE_SUSPENDED))
		{
			CString strErr;
			m_pWorkThread ->Delete();
			m_pWorkThread = NULL;
			
			strErr.Format("Failed to create working thread. Error Code = %d\n", GetLastError() );
			AfxMessageBox( strErr, MB_OK|MB_ICONERROR );
			PostMessage( WM_CLOSE );
		}
	}

	srand( (int)time(NULL) );

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTCR8SimulatorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTCR8SimulatorDlg::OnPaint() 
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
HCURSOR CTCR8SimulatorDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTCR8SimulatorDlg::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
	if ( m_pWorkThread != NULL )
	{
		m_pWorkThread->Terminate();
		m_pWorkThread->ResumeThread();
		Sleep( 100 );
	}
	CDialog::OnClose();
}

void CTCR8SimulatorDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	int		i, n;					// 0-based channel number
	UINT	uTID = nIDEvent;

	if ( TID_MIN <= uTID && uTID < TID_MAX )
	{
		n = (uTID - 100) % 4;
		uTID -= n;
	}
	switch ( uTID )
	{
	case TID_LOOPING:
		{
			m_uLoop++;
			UpdateLoopCount();
			if ( m_FSM != tsIdle )
			{
				UpdateAntenna();
				UpdateTransactionChannel();
				UpdateFSM();
			}
			for( i=0; i<4; i++ )
			{
				if ( m_bAutoPrepare && CanPrepareCSC(i) )
					StartPrepareCard(i, m_bRadomFail ? EXPECT_NONE : EXPECT_OK );
				if ( m_bAutoRecycle && CanRecycleCSC(i) )
					StartRecycleCard(i);
				UpdateChannelState(i);
			}
		}
		break;
	case TID_UNMASKBUTTON:
		KillTimer(nIDEvent);
		m_bMaskButton = FALSE;
		break;
	case TID_STATUSPACKET:
		if ( m_FSM != tsWaitRW )
			SendStatusPacket();
		break;
	case TID_CSCRWLIMIT:
		if ( m_FSM==tsWaitRW )
		{
			SendComment(0, "通行卡读写超时,中断流程!");
			SendEjectCSCPacket( m_nTransChannel, 4 );
			TransactionReset();
		}
		KillTimer(TID_CSCRWLIMIT);
		break;
	case TID_OUTBOXOK:
		KillTimer(nIDEvent);
		SetOnAnt(n);
		ClrPreparing(n);
		m_Count[n]--;
		UpdateCount(n);
		UpdateChannelState(n);
		UpdateControls();
		SendComment(n+1,"备卡到天线位");
		if ( m_FSM==tsWaitCSC && m_nTransChannel==n+1 )
			OnPressTakeCardButton( IDC_BUTTON_PRESS1+(m_nTransChannel > 2 ? 1 : 0) );
		SendStatusPacket();
		break;
	case TID_OUTBOXFAIL:
		{
			CString str;
			KillTimer(nIDEvent);
			SetFail(n);
			ClrPreparing(n);
			UpdateChannelState(n);
			UpdateControls();
			str.Format("备卡失败 (序号:%d,卡数:%d)", m_SN[n], m_Count[n] );
			SendStatusPacket();
			SendComment(n+1, str);
			if ( m_FSM==tsWaitCSC && m_nTransChannel==(n+1) )
				TransactionReset();
		}
		break;
	case TID_RECYCLE:
		KillTimer(nIDEvent);
		ClrRecycling(n);
		ClrBadCSC(n);
		ClrOnAnt(n);
		UpdateChannelState(n);
		UpdateControls();
		SendRecyclePacket(n+1, TRUE);
		SendStatusPacket();
		SendComment(n+1, "回收坏卡" );
		break;
	case TID_EJECTOK:
		KillTimer(nIDEvent);
		ClrEjecting(n);
		ClrOnAnt(n);
		SetOnExit(n);
		UpdateControls();
		UpdateChannelState(n);
		UpdateControls();
		SendComment(n+1, "卡推出卡机口" );
		if ( (n+1) == m_nTransChannel && m_FSM == tsRWDone )
		{
			SendEjectCSCPacket(n+1, (m_nTransChannel < 3) ? 1 : 2 );
			m_FSM = tsOnExit;
		}
		SendStatusPacket();
		break;
	case TID_EJECTFAIL:
		KillTimer(nIDEvent);
		ClrEjecting(n);
		SetFail(n);
		UpdateChannelState(n);
		UpdateControls();
		SendComment(n+1, "卡推卡机口失败" );
		if ((n+1)==m_nTransChannel && m_FSM==tsRWDone )
		{
			SendEjectCSCPacket(n+1,3);
			TransactionReset();
		}
		SendStatusPacket();
		break;
	default:
		CDialog::OnTimer(nIDEvent);
	}
}

////////////////////////////////////////////////////////////////////
// Reflections
void CTCR8SimulatorDlg::UpdateControls()
{
	if ( m_bAutoPrepare )
	{
		ENABLE_CWND( IDC_BUTTON_PREPARE, FALSE );
		ENABLE_CWND( IDC_BUTTON_OUTBOXFAIL, FALSE );
	}
	else
	{
		BOOL bEnable = CanPrepareCSC(m_nCH2OP) && m_bPowerOn;
		ENABLE_CWND( IDC_BUTTON_PREPARE, bEnable );
		ENABLE_CWND( IDC_BUTTON_OUTBOXFAIL, bEnable );
	}
	if ( m_bAutoRecycle )
	{
		ENABLE_CWND( IDC_BUTTON_RECYCLE, FALSE );
	}
	else
	{
		ENABLE_CWND( IDC_BUTTON_RECYCLE, CanRecycleCSC(m_nCH2OP) && m_bPowerOn );
	}
	if ( m_bAutoEject )
	{
		ENABLE_CWND( IDC_BUTTON_EJECT, FALSE );
		ENABLE_CWND( IDC_BUTTON_EJECTFAIL, FALSE );
	}
	else
	{
		BOOL bEnable = CanEjectCSC(m_nCH2OP) && m_bPowerOn;
		ENABLE_CWND( IDC_BUTTON_EJECT, bEnable );
		ENABLE_CWND( IDC_BUTTON_EJECTFAIL, bEnable );
	}
	ENABLE_CWND( IDC_BUTTON_LOAD, HasBox(m_nCH2OP) ? FALSE : TRUE );
	ENABLE_CWND( IDC_BUTTON_UNLOAD, HasBox(m_nCH2OP) ? TRUE : FALSE );
	ENABLE_CWND( IDC_BUTTON_RESETFAULT, (IsFail(m_nCH2OP) && m_bPowerOn) ? TRUE : FALSE );
	ENABLE_CWND( IDC_BUTTON_SENDLOG, (m_tty != -1 && m_bPowerOn) );
	ENABLE_CWND( IDC_BUTTON_SENDRAW, (m_tty != -1 && m_bPowerOn) );
	ENABLE_CWND( IDC_CHECK_RANDOMFAIL, m_bAutoPrepare );
	SET_WNDTEXT( IDC_BUTTON_OFFLINE, IsOffline(m_nCH2OP) ? "连线" : "离线" );
}

void CTCR8SimulatorDlg::UpdateChannelState(int n)
{
	UINT	uID = IDC_CHECK_HASBOX1 + n;

	for( int i=0; i<6; i++, uID+=4 )
	{
		GET_CONTROL( CButton, uID ) ->SetCheck( (m_State[n] & (1 << i)) ? BST_CHECKED : BST_UNCHECKED );
	}
	if ( IsPreparing(n) )
		SET_WNDTEXT( IDC_STATIC_CHACT1+n, "备卡中..." );
	else if ( IsRecycling(n) )
		SET_WNDTEXT( IDC_STATIC_CHACT1+n, "回收中..." );
	else if ( IsEjecting(n) )
		SET_WNDTEXT( IDC_STATIC_CHACT1+n, "出卡中..." );
	else if ( IsFail(n) )
		SET_WNDTEXT( IDC_STATIC_CHACT1+n, "有故障" );
	else if ( HasBadCSC(n) )
		SET_WNDTEXT( IDC_STATIC_CHACT1+n, "有坏卡" );
	else if ( IsOffline(n) )
		SET_WNDTEXT( IDC_STATIC_CHACT1+n, "卡机离线" );
	else if ( IsOnAnt(n) )
		SET_WNDTEXT( IDC_STATIC_CHACT1+n, "备妥" );
	else if ( IsOnExit(n) )
		SET_WNDTEXT( IDC_STATIC_CHACT1+n, "到卡口" );
	else
		SET_WNDTEXT( IDC_STATIC_CHACT1+n, "" );
}

////////////////////////////////////////////////////////////////////
// Controls

void CTCR8SimulatorDlg::OnSelchangeComboCOM() 
{
	// TODO: Add your control notification handler code here
	m_nPort = GET_CONTROL( CComboBox, IDC_COMBO_COM ) ->GetCurSel() + 1;
	if ( m_nPort > 0 && m_pWorkThread != NULL )
		ENABLE_CWND( IDC_BUTTON_OPEN, TRUE );
}

void CTCR8SimulatorDlg::OnButtonOpen() 
{
	// TODO: Add your control notification handler code here

	if ( (m_tty = tty_openPort( m_nPort )) != -1 )
	{
		tty_raw( m_tty, m_nSpeed, 8, PRTY_NONE );
		tty_setReadTimeouts( m_tty, 200, 1, 100 );
		ENABLE_CWND( IDC_BUTTON_CLOSE, TRUE );
		ENABLE_CWND( IDC_BUTTON_OPEN, FALSE );
		ENABLE_CWND( IDC_CHECK_POWER, TRUE );
		ENABLE_CWND( IDC_COMBO_COM, FALSE );
		// Resume the polling thread
		m_pWorkThread->SetTTY( m_tty );
		if ( m_bPowerOn )
			m_pWorkThread->ResumeThread();
	}
	else
	{
		CString strErr;
		strErr.Format("串口设备COM%d打开失败, 错误码%d!", m_nPort, GetLastError() );
		AfxMessageBox(strErr, MB_OK|MB_ICONERROR );
	}
}

void CTCR8SimulatorDlg::OnButtonClose() 
{
	// TODO: Add your control notification handler code here
	if ( m_tty != -1 )
	{
		m_pWorkThread->SuspendThread();
		tty_close( m_tty );
		m_tty = -1;
		ENABLE_CWND( IDC_BUTTON_CLOSE, FALSE );
		ENABLE_CWND( IDC_BUTTON_OPEN, TRUE );
		ENABLE_CWND( IDC_COMBO_COM, TRUE );
	}
}

void CTCR8SimulatorDlg::OnCheckPower() 
{
	// TODO: Add your control notification handler code here
	m_bPowerOn = GET_CONTROL( CButton, IDC_CHECK_POWER )->GetCheck() == BST_CHECKED;
	SET_WNDTEXT( IDC_CHECK_POWER, m_bPowerOn ? "断电" : "上电" );
	if ( m_bPowerOn )
	{
		SetTimer( TID_LOOPING, TMS_LOOPING, NULL );
		SendComment(0, "核心板上电" );
		SendPowerOnPacket();
		m_bWaitInit = TRUE;
		m_uLoop = 0;
		m_nTransChannel = m_nAntenna = 0;
		m_FSM = tsIdle;
		m_nCH2OP = 0;
		UpdateOperateChannel();
		UpdateTransactionChannel();
		UpdateFSM();
		UpdateAntenna();
		m_pWorkThread ->ResumeThread();
	}
	else
	{
		KillTimer( TID_LOOPING );
		if ( m_tidStatusPacket )
		{
			KillTimer( m_tidStatusPacket );
			m_tidStatusPacket = 0;
		}
		m_pWorkThread ->CleanQueue();
		m_pWorkThread ->SuspendThread();
	}
	UpdateControls();
}

void CTCR8SimulatorDlg::OnButtonConfig() 
{
	// TODO: Add your control notification handler code here
	CConfigDlg *pDlg = new CConfigDlg;

	pDlg->m_nOddsEject = m_nOddsEjectFail;
	pDlg->m_nOddsOutbox = m_nOddsPrepareFail;
	pDlg->m_tmaxEject = m_tmaxEject / 100;
	pDlg->m_tminEject = m_tminEject / 100;
	pDlg->m_tmaxOutbox = m_tmaxOutBox / 100;
	pDlg->m_tminOutbox = m_tminOutBox / 100;
	pDlg->m_nMaskPeriod = m_tMaskPeriod / 100;
	pDlg->m_nSpeed = m_nSpeed==19200;
	if ( pDlg->DoModal() == IDOK )
	{
		m_nOddsEjectFail = pDlg->m_nOddsEject;
		m_nOddsPrepareFail = pDlg->m_nOddsOutbox;
		m_tmaxEject = pDlg->m_tmaxEject * 100;
		m_tminEject  = pDlg->m_tminEject * 100;
		m_tmaxOutBox = pDlg->m_tmaxOutbox * 100;
		m_tminOutBox  = pDlg->m_tminOutbox * 100;
		m_tMaskPeriod = pDlg->m_nMaskPeriod * 100;
		// let recysle time same parameters same as eject time parameters
		m_tminRecycle = m_tminEject;
		m_tmaxRecycle = m_tmaxEject;
		m_nSpeed = pDlg->m_nSpeed==0 ? 9600 : 19200;
	}
}

void CTCR8SimulatorDlg::OnRadioChannel( UINT uID ) 
{
	// TODO: Add your control notification handler code here
	m_nCH2OP = uID - IDC_RADIO_CH1;
	UpdateControls();
}

void CTCR8SimulatorDlg::OnButtonLoadBox() 
{
	// TODO: Add your control notification handler code here
	if ( !HasBox( m_nCH2OP ) )
	{
		CString str;

		m_SN[m_nCH2OP] = 900000 + rand() % 10000;
		m_Count[m_nCH2OP] = DEF_CAP;
		m_Cap[m_nCH2OP] = DEF_CAP;
		SetHasBox(m_nCH2OP);
		UpdateSN(m_nCH2OP);
		UpdateCount(m_nCH2OP);
		UpdateChannelState(m_nCH2OP);
		UpdateControls();
		str.Format("卡盒装入(序号:%d,容量:%d,卡数:%d)", m_SN[m_nCH2OP], m_Cap[m_nCH2OP], m_Count[m_nCH2OP] );
		SendComment(m_nCH2OP+1, str );
		if ( !IsOffline(m_nCH2OP) )
			SendCartridgePacket(m_nCH2OP+1);
	}
}

void CTCR8SimulatorDlg::OnButtonUnloadBox() 
{
	// TODO: Add your control notification handler code here
	if ( HasBox( m_nCH2OP ) )
	{
		CString str;
		str.Format("卡盒卸下(序号:%d,卡数:%d)", m_SN[m_nCH2OP], m_Count[m_nCH2OP] );
		SendComment(m_nCH2OP+1, str );
		m_SN[m_nCH2OP] = 0;
		m_Count[m_nCH2OP] = 0;
		m_Cap[m_nCH2OP] = 0;
		ClrHasBox(m_nCH2OP);
		UpdateSN(m_nCH2OP);
		UpdateCount(m_nCH2OP);
		UpdateChannelState(m_nCH2OP);
		UpdateControls();
		if ( !IsOffline(m_nCH2OP) )
			SendCartridgePacket(m_nCH2OP+1);
	}
}

void CTCR8SimulatorDlg::OnButtonPrepareCard() 
{
	// TODO: Add your control notification handler code here
//	if ( CanPrepareCSC(m_nCH2OP) )
		StartPrepareCard( m_nCH2OP, EXPECT_OK );
}

void CTCR8SimulatorDlg::OnButtonOutboxFailed() 
{
	// TODO: Add your control notification handler code here
//	if ( CanPrepareCSC(m_nCH2OP) )
		StartPrepareCard( m_nCH2OP, EXPECT_FAIL );
}

void CTCR8SimulatorDlg::OnButtonRecycleCard() 
{
	// TODO: Add your control notification handler code here
	if ( HasBadCSC(m_nCH2OP) )
		StartRecycleCard( m_nCH2OP );
}

void CTCR8SimulatorDlg::OnButtonEjectCard() 
{
	// TODO: Add your control notification handler code here
//	if ( IsOnAnt(m_nCH2OP) && (m_nCH2OP==m_nTransChannel-1) && m_FSM==tsRWDone )
		StartEjectCard( m_nCH2OP, EXPECT_OK );
}

void CTCR8SimulatorDlg::OnButtonEjectFail() 
{
	// TODO: Add your control notification handler code here
//	if ( IsOnAnt(m_nCH2OP) && (m_nCH2OP==m_nTransChannel-1) && m_FSM==tsRWDone )
		StartEjectCard( m_nCH2OP, EXPECT_FAIL );
}

void CTCR8SimulatorDlg::OnButtonOnOffLine() 
{
	// TODO: Add your control notification handler code here
	if ( IsOffline( m_nCH2OP ) )
	{
		// Now set as online
		ClrOffline(m_nCH2OP);
		UpdateChannelState(m_nCH2OP);
		if ( m_SN0[m_nCH2OP] != m_SN[m_nCH2OP] )
			SendCartridgePacket(m_nCH2OP+1);
	}
	else
	{
		// Set as Offline
		SetOffline(m_nCH2OP);
		UpdateChannelState(m_nCH2OP);
		m_SN0[m_nCH2OP] = m_SN[m_nCH2OP];
	}
	UpdateControls();
	SendStatusPacket();
}

void CTCR8SimulatorDlg::OnButtonResetFault() 
{
	// TODO: Add your control notification handler code here
	ClrFail(m_nCH2OP);
	UpdateChannelState(m_nCH2OP);
	UpdateControls();
}

int CTCR8SimulatorDlg::SelectChannel( int nDeck, int *Code )
{
	int	n, n1;				// 0 based channel number
	int rc=0;

	// nDeck 1 means upper, 2 means lower
	n = (nDeck==1) ? m_nActiveUpper : m_nActiveLower + 2;		// 0 based channel number
	if ( IsOnAnt(n) && !HasBadCSC(n) && !IsFail(n) && !IsOffline(n) )
	{
		// select active channel of given deck
		rc = n + 1;			// selected channel is 1-based
		m_nAntenna = m_nTransChannel = rc;
	}
	else
	{
		// switch to siberling channel
		if ( nDeck==1 ) n1 = 1 - n;
		else			n1 = 5 - n;
		if ( IsOnAnt(n1) && !HasBadCSC(n1) && !IsFail(n1) && !IsOffline(n1) )
		{
			// select active channel of given deck
			rc = n1 + 1;
			m_nAntenna = m_nTransChannel = rc;
			if ( nDeck==2 ) m_nActiveLower = 1 - m_nActiveLower;
			else			m_nActiveUpper = 1 - m_nActiveUpper;
			UpdateActiveChannel();
		}
	}
	if ( rc == 0 )
	{
		if ( IsRecycling(n) || IsPreparing(n) )
		{
			*Code = IBT_DELAY;
			m_nAntenna = m_nTransChannel = n + 1;
		}
		else if ( IsRecycling(n1) || IsPreparing(n1) )
		{
			*Code = IBT_DELAY;
			m_nAntenna = m_nTransChannel = n1 + 1;
		}
		else if ( m_bAutoRecycle && HasBadCSC(n) && !IsFail(n) && m_Count[n]>0 )
		{
			*Code = IBT_DELAY;
			m_nAntenna = m_nTransChannel = n + 1;
			StartRecycleCard(n);
		}
		else if ( m_bAutoRecycle && HasBadCSC(n1) && !IsFail(n1) && m_Count[n1]>0 )
		{
			*Code = IBT_DELAY;
			m_nAntenna = m_nTransChannel = n1 + 1;
			if ( nDeck==2 ) m_nActiveLower = 1 - m_nActiveLower;
			else			m_nActiveUpper = 1 - m_nActiveUpper;
			UpdateActiveChannel();
			StartRecycleCard(n1);
		}
		else if ( IsFail(n) && IsFail(n1) )
			*Code = IBT_FAILED;
		else if ( m_Count[n]==0 && m_Count[n1]==0 && !IsOnAnt(n) && !IsOnAnt(n1) )
			*Code = IBT_NOCARD;
		else
			*Code = IBT_NOTREADY;
	}
	return rc;
}

void CTCR8SimulatorDlg::OnPressTakeCardButton(UINT uID) 
{
	// TODO: Add your control notification handler code here
	int nDeck = uID - IDC_BUTTON_PRESS1 + 1;
	int rc;

	if ( m_tty == -1 || !m_bPowerOn )
		return;
	
	if ( m_FSM==tsIdle )
	{
		SendComment(0, nDeck==1 ? "上工位按键" : "下工位按键" );
		m_nRWFailCount = 0;
	}
	else if ( m_FSM >= tsRWDone )
		return;

	if ( m_bMaskButton )
	{
		SendComment(0, "-->按键屏蔽期间,忽略");
		SendButtonIgnoredPacket( nDeck, IBT_MASKPERIOD );
		return;
	}

	if ( SelectChannel(nDeck, &rc) > 0 )
	{
		m_FSM = tsWaitRW;
		m_nCH2OP = m_nTransChannel - 1;
		UpdateOperateChannel();
		UpdateControls();
		SendComment( m_nTransChannel, "选择此通道发卡" );
		SendButtonPressedPacket();
		SetTimer( TID_CSCRWLIMIT, TMS_CSCRWLIMIT, NULL );
	}
	else
	{
		switch ( rc )
		{
		case IBT_DELAY:
			m_FSM = tsWaitCSC;
			SendComment(0, "--> 卡机忙,等卡到位");
			break;
		case IBT_NOCARD:
			SendComment(0, "--> 无卡可发" );
			break;
		case IBT_FAILED:
			SendComment(0, "--> 通道故障" );
			break;
		case IBT_NOTREADY:
			SendComment(0, "--> 通道未备妥" );
			break;
		}
		SendButtonIgnoredPacket( nDeck, rc );
	}
}

void CTCR8SimulatorDlg::OnButtonMP( UINT uID ) 
{
	// TODO: Add your control notification handler code here
	int nButton = uID - IDC_BUTTON_MP1;		// 0 for upper military/policy button

	SendComment( 0, nButton ? "下工位军警按键" : "上工位军警按键" );
	if ( m_bMaskButton )
	{
		SendComment(0, "-->按键屏蔽期间,忽略");
		return;
	}
	SendMPButtonPressedPacket( nButton + 1 );
}


void CTCR8SimulatorDlg::OnButtonTakeCard() 
{
	// TODO: Add your control notification handler code here
	if ( IsOnExit(m_nCH2OP) )
	{
		ClrOnExit(m_nCH2OP);
		ClrBadCSC(m_nCH2OP);
		SET_WNDTEXT( IDC_STATIC_CHACT1+m_nCH2OP, "卡取走" );
		if ( m_nCH2OP == m_nTransChannel-1 )
		{
			SendCSCTakenPacket(m_nTransChannel);
			TransactionReset();
			if ( m_tMaskPeriod > 0 )
			{
				SetTimer( TID_UNMASKBUTTON, m_tMaskPeriod, NULL );
				m_bMaskButton = TRUE;
			}
		}
		SendComment( m_nCH2OP+1, "卡取走" );
		SendStatusPacket();
		UpdateControls();
	}
}

void CTCR8SimulatorDlg::OnButtonSendLog() 
{
	// TODO: Add your control notification handler code here
	CString strLog;

	GET_WNDSTRING( IDC_EDIT_LOGTEXT, strLog );
	SendComment( 0, strLog );
}

void CTCR8SimulatorDlg::OnButtonSendRaw() 
{
	// TODO: Add your control notification handler code here
	char rawText[256], cookText[256];
	int	 i, Byte, len;
	char *ptr;

	GET_WNDTEXT( IDC_EDIT_LOGTEXT, rawText, 256 );
	ptr = cookText;
	len = strlen(rawText);
	for( i=0; i<len; )
	{
		if ( rawText[i] == '\\' )
		{
			if ( isxdigit(rawText[i+1]) && isxdigit(rawText[i+2]) )
			{
				if ( isdigit(rawText[i+1]) )
					Byte = rawText[i+1] - '0';
				else
					Byte = tolower(rawText[i+1]) - 'a' + 10;
				Byte <<= 4;
				if ( isdigit(rawText[i+2]) )
					Byte += rawText[i+2] - '0';
				else
					Byte += tolower(rawText[i+2]) - 'a' + 10;
				i += 3;
				*(ptr++) = Byte;
				continue;
			}
		}
		*(ptr++) = rawText[i++];
	}
	*ptr = '\0';
	len = ptr - cookText;
	if ( cookText[0] == '<' && isdigit(cookText[1]) )
		SendPacketEx( cookText, len, cookText[2]=='B' ? 0 : 1 );
	else
		SendPacketEx( cookText, len, 2 );
}

void CTCR8SimulatorDlg::OnButtonAutoCheck()
{
	m_bAutoAck = GET_CONTROL( CButton, IDC_CHECK_STOPAUTOACK ) ->GetCheck() == BST_UNCHECKED();
}

void CTCR8SimulatorDlg::OnClickOptions(UINT uID) 
{
	// TODO: Add your control notification handler code here
	UpdateData( TRUE );
	UpdateControls();
}

void CTCR8SimulatorDlg::OnKillfocusEditCount( UINT uID ) 
{
	// TODO: Add your control notification handler code here
	int n = uID - IDC_EDIT_CNT1;
	CString str;
	int		num;

	GET_WNDSTRING( uID, str );
	num = atoi( str );
	if ( 0 <= num && num <= DEF_CAP )
	{
		str.Format("%d", m_Count[n] );
		SET_WNDTEXT( uID, str );
	}
	else
	{
		CString strErr;
		strErr.Format("卡数必须在 0 ~ %d 之间", DEF_CAP );
		AfxMessageBox(strErr, MB_OK|MB_ICONSTOP );
		m_Count[n] = num;
	}
}

void CTCR8SimulatorDlg::OnKillfocusEditSN( UINT uID )
{
	// TODO: Add your control notification handler code here
	int n = uID - IDC_EDIT_SN1;
	CString str;
	int		num;

	GET_WNDSTRING( uID, str );
	num = atoi( str );
	if ( 0 < num && num < 100000000 )
	{
		str.Format("%d", m_SN[n] );
		SET_WNDTEXT( uID, str );
	}
	else
	{
		AfxMessageBox("序号必须在 1~99999999 之间!", MB_OK|MB_ICONSTOP );
		m_SN[n] = num;
	}
}

void CTCR8SimulatorDlg::OnCheckChannelState( UINT uID ) 
{
	// All channel state indication check box are read-only. Once checked, I have to refresh according to 
	// current state
	int		n = (uID - IDC_CHECK_HASBOX1) % 4;
	UpdateChannelState( n );
}

void CTCR8SimulatorDlg::OnCheckPause() 
{
	// TODO: Add your control notification handler code here
	m_bPaused = GET_CONTROL( CButton, IDC_CHECK_PAUSE )->GetCheck() == BST_CHECKED;	
	if ( !m_bPaused )
		m_ctrlList.EnsureVisible(m_ctrlList.GetItemCount()-1, FALSE);
}
//////////////////////////////////////////////////////////////////////////
// Transaction and Action
void CTCR8SimulatorDlg::TransactionReset()
{
	m_FSM = tsIdle;
	m_nAntenna = 0;
	m_nTransChannel = 0;
	UpdateFSM();
	UpdateTransactionChannel();
	UpdateAntenna();
}

void CTCR8SimulatorDlg::StartPrepareCard(int nChannel, int Expect )
{
	UINT tid, tms;

	SetPreparing(nChannel);
	switch ( Expect )
	{
	case EXPECT_OK:
		tid = TID_OUTBOXOK;
		break;
	case EXPECT_FAIL:
		tid = TID_OUTBOXFAIL;
		break;
	default:
		tid = (rand() % m_nOddsPrepareFail) == 0 ? TID_OUTBOXFAIL : TID_OUTBOXOK;
	}
	if ( tid == EXPECT_OK )
		tms = m_tminOutBox + (m_tmaxOutBox - m_tminOutBox) / ((rand() % 5) + 1);
	else
		tms = m_tmaxOutBox;
	if ( tms == 0 ) tms = 1;
	SendComment(nChannel+1, "开始备卡..." );
	SetTimer( tid+nChannel, tms, NULL );
}

void CTCR8SimulatorDlg::StartRecycleCard(int nChannel)
{
	UINT  tms;
	SetRecycling(nChannel);
	tms = m_tminRecycle + (m_tmaxRecycle - m_tminRecycle) / ((rand() % 5) + 1);
	SendComment( nChannel+1, "开始回收卡..." );
	if ( tms == 0 ) tms = 1;
	SetTimer( TID_RECYCLE+nChannel, tms, NULL );
}

void CTCR8SimulatorDlg::StartEjectCard(int nChannel, int Expect )
{
	UINT tid, tms;

	SetEjecting(nChannel);
	switch ( Expect )
	{
	case EXPECT_OK:
		tid = TID_EJECTOK;
		break;
	case EXPECT_FAIL:
		tid = TID_EJECTFAIL;
		break;
	default:
		tid = (rand() % m_nOddsEjectFail) == 0 ? TID_EJECTFAIL : TID_EJECTOK;
	}
	if ( tid == EXPECT_OK )
		tms = m_tminEject + (m_tmaxEject - m_tminEject) / ((rand() % 5) + 1);
	else
		tms = m_tmaxEject;
	if ( tms == 0 ) tms = 1;
	SendComment( nChannel+1, "开始将卡推出卡口..." );
	SetTimer( tid+nChannel, tms, NULL );
}

//////////////////////////////////////////////////////////////////////////
// Protocols

void CTCR8SimulatorDlg::ProcessReceivedPacket( const char *packet )
{
	char text[32];
	int	 i;

	switch ( packet[2] )
	{
	case 'a':		// initial packet
		{
			CString strMsg;

			strMsg = "收到初始化信息帧";
			m_bExtendedMode = memcmp( packet+3, "901", 3 ) == 0;
			if ( m_bExtendedMode )
				strMsg += "(使用扩展协议)";
			else
				strMsg += "(使用基本协议)";
			SendComment(0, strMsg );
			m_bWaitInit = FALSE;
			// clear the message queue so that all pending message (most likely, only power on packet) will be removed.
			m_pWorkThread ->CleanQueue();
			if ( m_bExtendedMode )
				SendVersionPacket();
#ifdef MSG_USESINGLEBOXINFO
			for( i=1; i<5; i++ )
				SendCartridgePacket(i);
#else
			SendCartridgePacket(0);
#endif
			SendStatusPacket();
			m_tidStatusPacket = SetTimer( TID_STATUSPACKET, TMS_STATUSPACKET, NULL );
		}
		break;
	case 'b':		// validation done
		i = packet[3] - '0';
		if ( m_FSM == tsWaitRW )
		{
			KillTimer( TID_CSCRWLIMIT );
			if ( i==0 || (i==m_nAntenna) ||
				 (i==5 && (m_nAntenna==1 || m_nAntenna==2)) ||
				 (i==6 && (m_nAntenna==3 || m_nAntenna==4)) )
			{
				m_FSM = tsRWDone;
				SendComment(m_nTransChannel, "通行卡读写成功" );
				if ( m_bAutoEject )
					StartEjectCard( m_nTransChannel-1, EXPECT_OK );
				else 
					UpdateControls();
			}
			else 
			{
				CString strErr;
				if ( i != m_nTransChannel )
					strErr.Format("车道指示发卡通道(%d)和核心板锁定通道(%d)不一致，交易中止", i, m_nTransChannel );
				else
					strErr.Format("车道指示发卡通道(%d)和天线位置(%d)不一致，交易中止", i, m_nAntenna );
				AppendErrorLog(strErr);
				SendComment(m_nTransChannel,strErr);
				SendEjectCSCPacket( m_nTransChannel, 3 );
				TransactionReset();
			}
		}
		break;
	case 'c':		// validation failed
		i = packet[3] - '0';
		if ( m_FSM == tsWaitRW )
		{
			KillTimer( TID_CSCRWLIMIT );
			if ( i==0 || (i==m_nTransChannel && m_nTransChannel==m_nAntenna) )
			{
				SetBadCSC(m_nTransChannel-1);
				SendStatusPacket();
				SendComment(m_nTransChannel,"读写失败,置坏卡");
				if ( ++m_nRWFailCount >= RWFAIL_LIMIT )
				{
					SendComment(0, "--> 连续读写失败, 中断流程" );
					SendButtonIgnoredPacket(m_nTransChannel < 3 ? 1 : 2, IBT_ERRLIMIT );
				}
				else
					OnPressTakeCardButton( IDC_BUTTON_PRESS1 + (m_nAntenna < 3 ? 0 : 1) );
			}
			else 
			{
				CString strErr;
				if ( i != m_nTransChannel )
					strErr.Format("车道指示坏卡(%d)和核心板锁定通道(%d)不一致，交易中止", i, m_nTransChannel );
				else
					strErr.Format("车道指示坏卡(%d)和天线位置(%d)不一致，交易中止", i, m_nAntenna );
				AppendErrorLog(strErr);
				SendComment(m_nTransChannel,strErr);
				TransactionReset();
			}
		}
		break;
	case 'd':		// play audio
		SendComment(0, "收到已经作废的播放语音帧!" );
		break;
	case 'e':		// Query status
		SendStatusPacket();
		break;
	case 'f':
#ifdef MSG_USESINGLEBOXINFO
		i = packet[3] - '0';
		if ( i == 0 )
		{
			for( i=1; i<5; i++ )
				SendCartridgePacket(i);
		}
		else if ( 0 < i && i < 5 )
			SendCartridgePacket(i);
#else
		SendCartridgePacket(0);
#endif
		break;
	case 'g':
		if ( m_FSM == tsWaitRW )
		{
			SendComment(0, "车道取消按键" );
			KillTimer( TID_CSCRWLIMIT );
			TransactionReset();
		}
		break;
	case 'h':		// switch antenna
		if ( m_FSM != tsWaitCSC && m_FSM != tsWaitRW )
		{
			int nPosit = packet[3] - '0';
			if ( 1 <= nPosit && nPosit <= 8 )
			{
				m_nAntenna = nPosit;
				UpdateAntenna();
				// If antenna is at one of cartridge tag CSC positions, hold take card button for at least one second
				// to ensure that antenna won't be switched away while AP is reading cartridge information from tag CSC
				if ( nPosit > 4 )			
				{							
					m_bMaskButton = TRUE;
					SetTimer( TID_UNMASKBUTTON, 1000, NULL );			// Mask take CSC button operation for 1 sec.
				}
				SendComment(nPosit, "计算机切换天线到此位置" );
			}
		}
		else
		{
			CString strErr;
			strErr.Format("流程状态为(%s),此时不可以切换天线", nmFSM[(int)m_FSM] ); 
			SendComment(0, strErr );
		}
		break;
	case 'i':		// select channel
		if ( m_FSM == tsIdle )
		{
			int	n = packet[3] - '0';
			if ( n==1 || n==2 )
				m_nActiveUpper = n - 1;
			else if ( n==3 || n==4 )
				m_nActiveLower = n - 3;
			else
			{
				sprintf( text, "指定了错误的通道位置\'%c\'", packet[3] );
				AppendErrorLog(text);
			}
			UpdateActiveChannel();
			SendComment(n, "上位机选择此通道");
		}
		else
		{
			CString strErr;
			strErr.Format("流程状态为(%s),此时不可以切换通道", nmFSM[(int)m_FSM] ); 
			SendComment(0, strErr );
		}
		break;
	case 'j':		// set cartridge data
		{
			int	SN, Count, Cap, oldSN;
			int n = packet[3] - '1';		// convert to 0-based index
			sscanf(packet+4, "%08d%03d%03d", &SN, &Cap, &Count );
			if ( SN > 0 )
			{
				oldSN = m_SN[n];
				m_SN[n] = SN;
				sprintf( text, "%d", SN );
				SET_WNDTEXT( IDC_EDIT_SN1+n, text );
			}
			if ( 0 < Count && Count <= DEF_CAP )
			{
				m_Count[n] = Count;
				sprintf( text, "%d", Count );
				SET_WNDTEXT( IDC_EDIT_CNT1+n, text );
			}
			if ( Cap >= Count && Cap >= MIN_CAP && Cap <= MAX_CAP )
			{
				m_Cap[n] = Cap;
			}
			CString str;
			str.Format("设置卡盒讯息(序号:%d,容量:%d,卡数:%d)", SN, Cap, Count );
			SendComment(n, str);
			if ( oldSN != m_SN[n] )
				SendCartridgePacket(n+1);
		}
		break;
	case 'k':		// 查询卡机属性 - 福建协议
		break;

	case 'r': 		// 使能指定卡机的自动备卡功能 - 福建协议
		break;
	case 'l':			// 触发按键动作
		break;
	case 'm':			// 无条件推出天线上的卡
		break;
	case 'n':			 // 无条件回收天线上的卡
		break;
	case 't':		// time sync.
		// Do nothing
		SendComment(0,"计算机发送对时帧");
		break;
	case 'u':			// enable program log output
		break;
	}

}

void CTCR8SimulatorDlg::AppendPacketOnList( const char *packet, const char *comment, BOOL bReceived )
{
	char		event_time[ 32 ];
	SYSTEMTIME	tsys;
	CString		strPacket;
	int			n;

	GetLocalTime( &tsys );
	sprintf( event_time, "%02d:%02d:%02d.%03d", tsys.wHour, tsys.wMinute, tsys.wSecond, tsys.wMilliseconds );
	if( m_ctrlList.GetItemCount() >= 10000 )
	{
       m_ctrlList.DeleteItem( 0 );
	}   

	n = m_ctrlList.InsertItem(m_ctrlList.GetItemCount(), event_time);
	strPacket = packet;
	if ( comment != NULL )
	{
		strPacket += " (";
		strPacket += comment;
		strPacket += ")";
	}
	m_ctrlList.SetItemText(n, bReceived ? 2 : 1, (LPCTSTR)strPacket );
	if ( !m_bPaused )
		m_ctrlList.EnsureVisible(m_ctrlList.GetItemCount()-1, FALSE);

}

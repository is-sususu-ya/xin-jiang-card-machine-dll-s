// TCR8SimulatorDlg.h : header file
//

#if !defined(AFX_TCR8SIMULATORDLG_H__61062E55_9E21_4FBF_8AF9_A655AF3BCA37__INCLUDED_)
#define AFX_TCR8SIMULATORDLG_H__61062E55_9E21_4FBF_8AF9_A655AF3BCA37__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CWorkThread.h"
#include "../common/utils_macro.h"

#define TCR8_VERSION	"01.00.00.02"
#define DEF_CAP			500
#define MIN_CAP			400
#define MAX_CAP			600

#define IBT_DELAY			0
#define IBT_NOCARD			1
#define IBT_FAILED			2
#define IBT_NOTREADY		3
#define IBT_DOWNMODE		4
#define IBT_MASKPERIOD		5
#define IBT_ERRLIMIT		6

//#define MSG_USESINGLEBOXINFO

static const char *nmFSM[] = {
		"等待按键", "等待工作通道备卡", "等待读写", "读写成功出卡...", "等待取卡" };

/////////////////////////////////////////////////////////////////////////////
// CTCR8SimulatorDlg dialog

class CTCR8SimulatorDlg : public CDialog
{
// Construction
public:
	CTCR8SimulatorDlg(CWnd* pParent = NULL);	// standard constructor
	void ProcessReceivedPacket( const char *packet );
	void AppendPacketOnList( const char *packet, const char *comment, BOOL bReceived );
#define AppendRxPacket(p,c)	AppendPacketOnList(p,c,TRUE)
#define AppendTxPacket(p,c)	AppendPacketOnList(p,c,FALSE)
	void AppendErrorLog( const char *log )		{ /* TODO */ }		

// Dialog Data
	//{{AFX_DATA(CTCR8SimulatorDlg)
	enum { IDD = IDD_TCR8SIMULATOR_DIALOG };
	CListCtrl	m_ctrlList;
	BOOL	m_bAutoRecycle;
	int		m_nCH2OP;			
	int		m_nActiveLower;
	int		m_nActiveUpper;
	int		m_bAutoEject;
	int		m_bAutoPrepare;
	BOOL	m_bRadomFail;
	//}}AFX_DATA

	int		m_tty;				// tty OSF file descriptor
	int		m_nPort;			// COM port
	int		m_nSpeed;

	CWorkThread *m_pWorkThread;
	BOOL	m_bAutoAck;


	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTCR8SimulatorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

enum TRState {
	tsIdle, tsWaitCSC, tsWaitRW, tsRWDone, tsOnExit };

	TRState	m_FSM; 
	int		m_Count[4];
	int		m_SN[4];
	int		m_SN0[4];			// SN when channel is offline
	int		m_Cap[4];
	int		m_State[4];
	int		m_bExtendedMode;	// extended level 0 for basic set, 1 for extend 1, etc.
	BOOL	m_bPaused;
	int		m_nTransChannel;	// 1 based. 0 when transaction is idle
	int		m_nAntenna;			// 1 based
	BOOL	m_bMaskButton;
	BOOL	m_bPowerOn;
	BOOL	m_bWaitInit;		// Just send power on packet and wait for initial packet
	UINT	m_uLoop;
	UINT	m_tidStatusPacket;
	int		m_nRWFailCount;

	// parameters
	int		m_nOddsPrepareFail;		// Default is 1/100
	int		m_nOddsEjectFail;		// Default is 1/1000
	int		m_tminOutBox;
	int		m_tmaxOutBox;
	int		m_tminRecycle;
	int		m_tmaxRecycle;
	int		m_tminEject;
	int		m_tmaxEject;
	int		m_tMaskPeriod;			// Time period (in msec) to mask out the driver's push button action after previous card is taken

#define CSB_HASBOX		0x01
#define CSB_ONANT		0x02
#define CSB_BADCSC		0x04
#define CSB_FAIL		0x08
#define CSB_ONEXIT		0x10
#define CSB_OFFLINE		0x20
#define CSB_PREPARING	0x100
#define CSB_RECYCLING	0x200
#define CSB_EJECTING	0x400

#define HasBox(n)		(m_State[n] & CSB_HASBOX)
#define IsOnAnt(n)		(m_State[n] & CSB_ONANT)
#define HasBadCSC(n)	(m_State[n] & CSB_BADCSC)
#define IsFail(n)		(m_State[n] & CSB_FAIL)
#define IsOnExit(n)		(m_State[n] & CSB_ONEXIT)
#define IsOffline(n)	(m_State[n] & CSB_OFFLINE)
#define SetHasBox(n)	(m_State[n] |= CSB_HASBOX)
#define SetOnAnt(n)		(m_State[n] |= CSB_ONANT)
#define SetBadCSC(n)	(m_State[n] |= CSB_BADCSC)
#define SetFail(n)		(m_State[n] |= CSB_FAIL)
#define SetOnExit(n)	(m_State[n] |= CSB_ONEXIT)
#define SetOffline(n)	(m_State[n] |= CSB_OFFLINE)
#define ClrHasBox(n)	(m_State[n] &= ~CSB_HASBOX)
#define ClrOnAnt(n)		(m_State[n] &= ~CSB_ONANT)
#define ClrBadCSC(n)	(m_State[n] &= ~CSB_BADCSC)
#define ClrFail(n)		(m_State[n] &= ~CSB_FAIL)
#define ClrOnExit(n)	(m_State[n] &= ~CSB_ONEXIT)
#define ClrOffline(n)	(m_State[n] &= ~CSB_OFFLINE)
#define IsPreparing(n)	(m_State[n] & CSB_PREPARING)
#define SetPreparing(n)	(m_State[n] |= CSB_PREPARING)
#define ClrPreparing(n)	(m_State[n] &= ~CSB_PREPARING)
#define IsRecycling(n)	(m_State[n] & CSB_RECYCLING)
#define SetRecycling(n)	(m_State[n] |= CSB_RECYCLING)
#define ClrRecycling(n)	(m_State[n] &= ~CSB_RECYCLING)
#define IsEjecting(n)	(m_State[n] & CSB_EJECTING)
#define SetEjecting(n)	(m_State[n] |= CSB_EJECTING)
#define ClrEjecting(n)	(m_State[n] &= ~CSB_EJECTING)
#define IsActing(n)		(m_State[n] & 0x700 )
#define IsAble2Act(n)	( (m_State[n] & 0x728)==0 )		// Not Failed, Not offline, Not acting (recycle, eject, prepare)
#define IsTrackFree(n)	( (m_State[n] & 0x12)==0 )		// Not OnAnt and not on Exit

#define CanPrepareCSC(n) (m_Count[n]>0 && IsTrackFree(n) && IsAble2Act(n) )
#define CanRecycleCSC(n) (HasBadCSC(n) && IsOnAnt(n) /*&& (m_FSM==tsIdle||m_FSM==tsWaitCSC)*/ && IsAble2Act(n) )
#define CanEjectCSC(n)	( IsOnAnt(n) && (m_FSM==tsIdle ? 1 : (m_FSM==tsRWDone && n==m_nTransChannel-1)) && IsAble2Act(n) )

	// Generated message map functions
	//{{AFX_MSG(CTCR8SimulatorDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonOpen();
	afx_msg void OnCheckPower();
	afx_msg void OnButtonLoadBox();
	afx_msg void OnButtonUnloadBox();
	afx_msg void OnButtonPrepareCard();
	afx_msg void OnButtonOutboxFailed();
	afx_msg void OnButtonRecycleCard();
	afx_msg void OnButtonEjectCard();
	afx_msg void OnButtonEjectFail();
	afx_msg void OnButtonOnOffLine();
	afx_msg void OnButtonTakeCard();
	afx_msg void OnButtonSendLog();
	afx_msg void OnButtonSendRaw();
	afx_msg void OnClose();
	afx_msg void OnSelchangeComboCOM();
	afx_msg void OnButtonClose();
	afx_msg void OnCheckPause();
	afx_msg void OnButtonAutoCheck();
	afx_msg void OnButtonResetFault();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnButtonConfig();
	//}}AFX_MSG
	void OnRadioChannel( UINT uID );
	void OnClickOptions( UINT uID );
	void OnKillfocusEditCount( UINT uID );
	void OnKillfocusEditSN( UINT uID );
	void OnCheckChannelState( UINT uID );
	void OnPressTakeCardButton( UINT uID );
	void OnButtonMP( UINT uID );
	DECLARE_MESSAGE_MAP()

	// protected methods
	////////////////////////////////////////////////////////////////////
	// GUI Update
	void UpdateChannelState(int n);
	void UpdateControls();
	void UpdateFSM()
	{
		SET_WNDTEXT( IDC_STATIC_FSM, nmFSM[(int)m_FSM] );
	}
	void UpdateLoopCount()
	{
		char buf[12];
		sprintf(buf, "%d", ++m_uLoop );
		SET_WNDTEXT( IDC_STATIC_LOOPCNT, buf );
	}
	void UpdateTransactionChannel()
	{
		char buf[2] = "#";
		buf[0] = m_nTransChannel + '0';
		SET_WNDTEXT( IDC_STATIC_WORKCHANNEL, buf );
	}
	void UpdateActiveChannel()
	{
		GET_CONTROL( CButton, IDC_RADIO_WORKUP1 ) ->SetCheck( m_nActiveUpper ? BST_UNCHECKED : BST_CHECKED );
		GET_CONTROL( CButton, IDC_RADIO_WORKUP2 ) ->SetCheck( m_nActiveUpper ? BST_CHECKED : BST_UNCHECKED );
		GET_CONTROL( CButton, IDC_RADIO_WORKDOWN1 ) ->SetCheck( m_nActiveLower ? BST_UNCHECKED : BST_CHECKED );
		GET_CONTROL( CButton, IDC_RADIO_WORKDOWN2 ) ->SetCheck( m_nActiveLower ? BST_CHECKED : BST_UNCHECKED );
	}
	void UpdateOperateChannel()
	{
		for (int i=0; i<4; i++ )
			GET_CONTROL( CButton, IDC_RADIO_CH1+i ) ->SetCheck( m_nCH2OP==i ? BST_CHECKED : BST_UNCHECKED );
	}
	void UpdateAntenna()
	{
		char buf[2] = "#";
		buf[0] = m_nAntenna + '0';
		SET_WNDTEXT( IDC_STATIC_ANTENNA, buf );
	}
	void UpdateSN(int n)
	{
		char buf[12];
		if ( m_SN[n] == 0 )
			buf[0] = '\0';
		else
			sprintf( buf, "%d", m_SN[n] );
		SET_WNDTEXT( IDC_EDIT_SN1+n, buf );
	}
	void UpdateCount(int n)
	{
		char buf[8];
		if ( m_SN[n] == 0 )
			buf[0] = '\0';
		else
			sprintf( buf, "%d", m_Count[n] );
		SET_WNDTEXT( IDC_EDIT_CNT1+n, buf );
	}
	///////////////////////////////////////////////////////////////////////
	// Protocol packets
	void SendPacket(const char *packet, int mode )
	{
		if ( m_pWorkThread != NULL && m_tty != -1 && m_bPowerOn )
			m_pWorkThread->SendPacket(packet, mode);
	}
	void SendPacketEx(const char *packet, int len, int mode )
	{
		if ( m_pWorkThread != NULL && m_tty != -1 && m_bPowerOn )
			m_pWorkThread->SendPacketEx(packet, len, mode);
	}
	void SendPowerOnPacket()
	{
		char buf[] = "<#A>";
		SendPacket( buf, 1 );
	}
	void SendVersionPacket()
	{
		char	msg_body[ MSGBUF_SIZE ];
		char	*ptr = msg_body;

		/* prepare message to lane computer */
		if ( m_bExtendedMode )
		{
			*(ptr++) = '<';
			*(ptr++) = '#';
			*(ptr++) = 0x56;
			strcpy( ptr, TCR8_VERSION );		ptr += 11;
			*(ptr++) = '>';
			*ptr = '\0';
			SendPacket(msg_body, 1 );
		}
	}
	void SendStatusPacket()
	{
		char buf[ MSGBUF_SIZE ];
		char *ptr = buf;
		int	 nOnAnt;

		if ( !m_bWaitInit )
		{
			*(ptr++) = '<';
			*(ptr++) = '#';
			*(ptr++) = 'B';
			*(ptr++) = m_nActiveUpper + '1';
			*(ptr++) = m_nActiveLower + '3';
			for( int i=0; i<4; i++ )
			{
				char ch;
				if ( IsFail(i) || IsOffline(i) )
					ch = '1';
				else if ( HasBadCSC(i) )
					ch = '2';
				else if ( m_bExtendedMode && IsOffline(i) )
					ch = '3';
				else
					ch = '0';
				*(ptr++) = ch;
				*(ptr++) = HasBox(i) ? '0' : '1';
				nOnAnt = IsOnAnt(i) ? 1 : 0;
				sprintf( ptr, "%03d", m_Count[i] + nOnAnt);	ptr += 3;
				if ( m_bExtendedMode && IsOnExit(i) )
					*(ptr++) = '2';
				else
					*(ptr++) = nOnAnt + '0';
			}
			*(ptr++) = '>';
			*ptr = '\0';
			SendPacket( buf, 0 );
		}
	}
	void SendCartridgePacket( int nChannel )		// 1-based channel
	{
		char	msg_body[ MSGBUF_SIZE ];
		if ( m_bExtendedMode )
		{
#ifdef MSG_USESINGLEBOXINFO
			int	n = nChannel - 1;
			sprintf(msg_body, "<#F%01d%08d%03d%03d>", nChannel, m_SN[n], m_Cap[n], m_Count[n] );
#else
			sprintf(msg_body, "<#F%08d%08d%08d%08d>", m_SN[0], m_SN[1], m_SN[2], m_SN[3] );
#endif
		}
		else
		{
			int SN[4];
			
			// in case SN is too long. basic mode protocol only carry 6 digit.
			for( int i=0; i<4; i++ )
				SN[i] = m_SN[i] % 1000000;

			sprintf(msg_body, "<#F%06d%03d%06d%03d%06d%03d%06d%03d>", 
				SN[0], m_Count[0], SN[1], m_Count[1], SN[2], m_Count[2], SN[3], m_Count[3] );
		}
		SendPacket(msg_body, 1 );
	}
	void SendButtonPressedPacket()
	{
		char buf[8];
		sprintf(buf, "<#D%c%c>", m_nTransChannel < 3 ? '1' : '2', m_nTransChannel + '0' );
		SendPacket( buf, 1 );
	}
	void SendMPButtonPressedPacket( int nDeck )
	{
		char buf[8];
		sprintf(buf, "<#D%c%c>", nDeck+'0', nDeck+'4' );
		SendPacket( buf, 1 );
	}
	void SendButtonIgnoredPacket( int nDeck, int nWhy )
	{
		if ( m_bExtendedMode )
		{
			char buf[8];
			sprintf(buf, "<#H%c%c>", nDeck+'0', nWhy+'0' );
			SendPacket( buf, 1 );
		}
	}
	void SendEjectCSCPacket( int Channel, char Code )
	{
		char buf[] = "<#CnX>";
		buf[3] = Code + '0';		// '1' upper deck, '2' lower deck, '3' failed, '4' timed out
		buf[4] = Channel + '0';
		SendPacket( buf, 1 );
	}
	void SendCSCTakenPacket( int nChannel )		// this channel is 1 based
	{
		char buf[] = "<#Edc>";
		buf[3] = nChannel < 3 ? '1' : '2';
		buf[4] = nChannel + '0';
		SendPacket( buf, 1 );
	}
	void SendRecyclePacket(int nChannel, BOOL bOK)	// this is 1 based channel
	{
		char buf[] = "<#Gdc>";
		buf[3] = bOK ? ( nChannel < 3 ? '1' : '2') : '3';
		buf[4] = nChannel + '0';
		SendPacket( buf, 1 );
	}
	void SendComment( int nChannel, const char *text )
	{
		if ( m_bExtendedMode )
		{
			CString strComment;
			if ( nChannel > 0 )
			{
				strComment.Format("{%d#: %s}", nChannel, text );
			}
			else if ( text[0] != '{' )
			{
				strComment.Format("{%s}", text );
			}
			else
			{
				strComment = text;
			}
			SendPacket( strComment, 2 );
		}
	}

private:
	int  SelectChannel( int nDeck, int *Code );
	void TransactionReset();
	void StartPrepareCard(int nChannel, int Expect );
	void StartRecycleCard(int nChannel);
	void StartEjectCard(int nChannel, int Expect );
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TCR8SIMULATORDLG_H__61062E55_9E21_4FBF_8AF9_A655AF3BCA37__INCLUDED_)

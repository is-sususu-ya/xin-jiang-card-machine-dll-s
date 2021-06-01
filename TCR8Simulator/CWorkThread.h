#if !defined(AFX_WORKTHREAD_H__F5587125_64AB_4889_8DD8_F3C27419DC4C__INCLUDED_)
#define AFX_WORKTHREAD_H__F5587125_64AB_4889_8DD8_F3C27419DC4C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WorkThread.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CWorkThread thread
#include "../common/longtime.h"
#include "../common/wintty.h"

class CTCR8SimulatorDlg;				// forward reference

#define MSGBUF_SIZE		128
#define Mutex_Lock()		WaitForSingleObject( m_hMutex, INFINITE )
#define Mutex_Unlock()		ReleaseMutex( m_hMutex )

typedef struct lane_msg {
	int			msg_state;					/* 0 -- free slot, 1 -- wait for ack from lane computer */
	int			msg_len;
	char		msg_body[ MSGBUF_SIZE ];	/* last message body -- null terminated (for resending) */
	_longtime	t_resend;					/* next time to resend if host is not responsed */
	time_t		t_queue;					/* queue time in sec */
	int			msg_naked;					/* number of NAKed times */
} LANEMSG;

class CWorkThread : public CWinThread
{
public:
	DECLARE_DYNCREATE(CWorkThread)
	CWorkThread(CTCR8SimulatorDlg* pOCXCtrl);

protected:
	CWorkThread();           // protected constructor used by dynamic creation

//  attributes
	CTCR8SimulatorDlg	*m_pDlg;				// pointer of class instance who owns me.
	HANDLE		m_hMutex;						// Spin-lock to access the Protocol Packet ring buffer.
	LANEMSG		_laneMsg[10];					// Ring buffer of protocol message send to lane computer
	int			m_cSeq;							// Sequence of protocol packet to send ('0' ~ '9')
	char		m_LastPacket[ MSGBUF_SIZE ];
	int			m_nRepeatError;
	int			m_nQuit;						// Quit the loop
	int			m_tty;

// Operations
public:
	/* nMode=0: do not need ack. Send only when queue is empty, otherwise drop it
			 1: need ack. Send if queue is empty, otherwise queued
			 2: comment, send immediately
	*/
	void	SendPacket( const char *packet, int nMode );			// Send a packet to Lane
	void	SendPacketEx( const char *packet, int len, int nMode );
	void	Terminate()		{ m_nQuit = 1; m_tty = -1; }
	void	SetTTY(int tty)	{ m_tty = tty; }
	void	CleanQueue()
	{
		Mutex_Lock();
		memset( _laneMsg, 0, sizeof(_laneMsg) );
		m_cSeq = '9';
		m_LastPacket[0] = '\0';
		m_nRepeatError = 0;
		Mutex_Unlock();
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMXThread)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWorkThread();

	// operation of high-level transaction protocol
	bool	IsAckPacket( const char *buf )		{ return buf[2] == '0'; }
	bool	IsNakPacket( const char *buf )		{ return buf[2] == '1'; }
	bool	IsInitPacket( const char *buf )		{ return buf[2] == 'a'; }
	void	SendAck( char cseq );
	void	SendNak( char cseq );
	void	AckedPacket( int nSeq );					// A packet sequence # nSeq is Acked by MOXA
	void	NakedPacket( int nSeq );					// A packet sequence # nSeq is Naked by MOXA
	void	RemovePowerOnPacket();
	bool	IsValidPacket( const char *packet );		// Is this a valid packet
	bool	IsEmptyQueue( void );						// Is packet sending queue empty
	int		GetTime2Resend( void );
	void	CheckAndResendPacket( void );
	bool	TooManyRepeatError( const char* packet )
	{
		if ( strcmp( packet, m_LastPacket ) == 0 && ++m_nRepeatError > 3 )
		{
			m_nRepeatError = 0;
			return true;
		}
		m_nRepeatError = 0;
		strcpy( m_LastPacket, packet );
		return false;
	}

	// Generated message map functions
	//{{AFX_MSG(CMXThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WORKTHREAD_H__F5587125_64AB_4889_8DD8_F3C27419DC4C__INCLUDED_)

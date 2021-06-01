// WorkThread.cpp : implementation file
//

#include "stdafx.h"
#include "TCR8Simulator.h"
#include "TCR8SimulatorDlg.h"
#include "CWorkThread.h"
#define __DEBUG 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWorkThread

IMPLEMENT_DYNCREATE(CWorkThread, CWinThread)

CWorkThread::CWorkThread( CTCR8SimulatorDlg *pDlg )
{
	memset( _laneMsg, 0, sizeof(_laneMsg) );
	memset( m_LastPacket, 0, sizeof( m_LastPacket ) );
	m_cSeq = '9';
	m_nRepeatError = 0;

	m_pDlg = pDlg;
	m_tty = -1;

	// set time zone
	_tzset();
	
	// create mutex handle
	m_hMutex = CreateMutex(
						NULL,				// default security attributes
						FALSE,				// initially not owned
						NULL);				// mutex name (NULL for unname mutex)

	ASSERT (m_hMutex != NULL);
}

CWorkThread::CWorkThread()
{
}

CWorkThread::~CWorkThread()
{
}

BOOL CWorkThread::InitInstance()
{
	int			tout;
	// never ending loop
	m_nQuit = 0;

	for( ;m_nQuit==0; )
	{
		// check for any protocol message in ring buffer that might have to be resend in 'msec'
		if ( (tout = GetTime2Resend()) == 0 )
			tout = 100;
		if ( tty_ready( m_tty, tout )==1 )
		{
			int		rlen;
			char	buf[ 128 ];

			rlen = tty_gets( m_tty, buf, sizeof(buf), '>' );
			if ( m_nQuit )	break;
			if ( rlen > 0 )
			{
				if ( IsValidPacket( buf ) )
				{
					m_pDlg->AppendRxPacket( buf, NULL );
					if ( IsAckPacket( buf ) )
						AckedPacket( buf[1] - '0' );
					else if ( IsNakPacket( buf ) )
						NakedPacket( buf[1] - '0' );
					else
					{
//						if ( IsInitPacket( buf ) )		Initial packet will cause the CleanQueue anyway
//							RemovePowerOnPacket();
						if ( m_pDlg->m_bAutoAck )
							SendAck( buf[1] );		// Ack it
						m_pDlg->ProcessReceivedPacket( buf );
					}
				}
				else
				{
					strcat( buf, " [INVALID]" );
					m_pDlg->AppendRxPacket( buf, NULL );
					if ( m_pDlg->m_bAutoAck && TooManyRepeatError( buf ) && buf[0]=='<' && buf[1] >= '0' && buf[1] <= '9' )
						SendAck( buf[1] );
					else if ( m_pDlg->m_bAutoAck && buf[0] == '<' && buf[1] >=0 && buf[1] <= '9' )
						SendNak( buf[1] );		// NAK it
				}
			}

		}	// end if ( tty_ready )
	
	}	// end for(;;)
	m_nQuit = 2;

	// avoid entering standard message loop by returning FALSE
	return FALSE;
}


BEGIN_MESSAGE_MAP(CWorkThread, CWinThread)
	//{{AFX_MSG_MAP(CWorkThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////////
// Method for Protocol with TCR8 control board


#define MSG_STX		'<'
#define MSG_ETX		'>'
#define MSG30_LEN	4	/* '0' ACK */
#define MSG31_LEN	4	/* '1' NAK */
#define MSG61_LEN	21	/* initial */
#define MSG62_LEN	5	/* RW Sucess */
#define MSG63_LEN	5	/* RW Failed */
#define MSG64_LEN	6	/* pay audio -- not used */
#define MSG65_LEN	5	/* query status */
#define MSG66_LEN	5	/* query cartridge */
#define MSG67_LEN	4	/* Cancel Button */
#define MSG68_LEN	5	/* Antenna switch */
#define MSG69_LEN	5	/* Channel switch */
#define MSG6A_LEN	19	/* Set cartridge information */
#define MSG6B_LEN	4	/* query cartridge */
#define MSG6C_LEN	5   /* trigger pushbutton sequence */
#define MSG6D_LEN	5   /* force eject */
#define MSG6E_LEN   5   /* force recycle */
#define MSG72_LEN	5 	/* enable auto prepare */
#define MSG74_LEN	18	/* time sync */
#define MSG75_LEN	5	/* enable/disable program log output */

#define MST_IDLE		0
#define	MST_WAITSEND	1			/* send donot wait for ACK */
#define MST_WAITACK		2			/* already sent, and wait for ACK now -- only queue head can be this state */
#define MST_TIMEOUT		1000		/* Time out to wait for ACKed (in # of msec.) */
#define MST_NAKLIMIT	3			/* Same message NAK by host over 'MST_NAKLIMIT' times will be treated 
 									 * as ACKed. This can prevent message that not supported by controller will
									 * block following message exchange as it will be always NAKed by contoller.
									 */
void CWorkThread::SendPacket( const char *i_packet, int nMode )
{
	SendPacketEx( i_packet, strlen(i_packet), nMode );
}

void CWorkThread::SendPacketEx( const char *i_packet, int i_size, int nMode )
{
	int		nseq;
	int		len = i_size;
	char	packet[ MSGBUF_SIZE ];
	bool	bIsEmpty;

	memcpy( packet, i_packet, i_size+1 );
	if ( nMode == 2 )
	{
		tty_write( m_tty, packet, len );
		m_pDlg->AppendTxPacket( packet, NULL );
		return;
	}
	
	Mutex_Lock();
	nseq = m_cSeq - '0';
	if ( ++nseq > 9 ) nseq = 0;

	if ( nMode==1 && _laneMsg[ nseq ].msg_state == MST_WAITACK )
	{
		// Queue full. Drop this one.
		CString strErr;

		strErr.Format("Message Queue Full, message %s ignored", packet );
		m_pDlg->AppendErrorLog( strErr );
		Mutex_Unlock();
		return;
	}

	m_cSeq = nseq + '0';
	packet[1] = m_cSeq;

	bIsEmpty = IsEmptyQueue();
	if ( nMode != 0 )
	{
		if ( bIsEmpty )
		{
			_laneMsg[ nseq ].msg_state = MST_WAITACK;
			_laneMsg[ nseq ].t_resend  = timeGetLongTime() + MST_TIMEOUT;
			_laneMsg[ nseq ].t_queue = time(NULL);
			_laneMsg[ nseq ].msg_naked = 0;
		}
		else
		{
			_laneMsg[ nseq ].msg_state = MST_WAITSEND;
		}			
		memcpy( _laneMsg[ nseq ].msg_body, packet, len + 1 );	/* 'len' does not include the NULL terminator. We need it */
		_laneMsg[ nseq ].msg_len = len;
	}
	if ( bIsEmpty || nMode==0 )
	{
		tty_write( m_tty, packet, len );
		m_pDlg->AppendTxPacket( packet, NULL );
	}
	Mutex_Unlock();
}

void CWorkThread::SendAck( char cseq )
{
	static char cAck[8] = "<x0>";

	if ( '0' <= cseq && cseq <= '9' )
	{
		cAck[1] = cseq;
		tty_write( m_tty, cAck, 4 );
		m_pDlg->AppendTxPacket( cAck, NULL );
	}
}

void CWorkThread::SendNak( char cseq )
{
	static char cNak[8] = "<x1>";

	if ( '0' <= cseq && cseq <= '9' )
	{
		cNak[1] = cseq;
		tty_write( m_tty, cNak, 4 );
		m_pDlg->AppendTxPacket( cNak, NULL );
	}
}

void CWorkThread::AckedPacket( int nSeq )
{
	Mutex_Lock();
	if ( 0 <= nSeq && nSeq < 10 && _laneMsg[ nSeq ].msg_state == MST_WAITACK )
	{
		_laneMsg[ nSeq ].msg_state = MST_IDLE;		/* free this slot */
		
		/* check for pending message queue to be sent */
		if ( ++nSeq > 9 )
			nSeq = 0;
		if ( _laneMsg[ nSeq ].msg_state == MST_WAITSEND )
		{
			_laneMsg[ nSeq ].msg_state = MST_WAITACK;
			_laneMsg[ nSeq ].t_resend = timeGetLongTime() + MST_TIMEOUT;		// Next second to resend if not acked
			_laneMsg[ nSeq ].t_queue = time(NULL);
			_laneMsg[ nSeq ].msg_naked = 0;
			
			tty_write( m_tty, _laneMsg[ nSeq ].msg_body, _laneMsg[ nSeq ].msg_len );
			m_pDlg->AppendTxPacket( _laneMsg[ nSeq ].msg_body, NULL );
		}
	}
	else
	{
		CString strErr;

		strErr.Format("Get ACK %d, but we do not expect it", nSeq );
		m_pDlg->AppendErrorLog( strErr );
	}
	Mutex_Unlock();
}

void CWorkThread::NakedPacket( int nSeq )
{
	Mutex_Lock();
	if ( 0 <= nSeq && nSeq < 10 && _laneMsg[ nSeq ].msg_state == MST_WAITACK )
	{
		if ( ++_laneMsg[ nSeq ].msg_naked > MST_NAKLIMIT )
		{
			CString strErr;

			strErr.Format("Packet %s NAKed by peer for %d times. Give-up", _laneMsg[ nSeq ].msg_body, MST_NAKLIMIT );
			m_pDlg->AppendErrorLog( strErr );
			// NAKed for too many times, assume acked and removed from queue
			AckedPacket( nSeq );
		}
		else
		{
			_laneMsg[ nSeq ].t_resend = timeGetLongTime() + MST_TIMEOUT;
			tty_write( m_tty, _laneMsg[ nSeq ].msg_body, _laneMsg[ nSeq ].msg_len );
			m_pDlg->AppendTxPacket( _laneMsg[ nSeq ].msg_body, NULL );
		}
	}
	else
	{
		CString strErr;

		strErr.Format("Get NAK %d while this message does not expect a ACK", nSeq );
		m_pDlg->AppendErrorLog( strErr );
	}
	Mutex_Unlock();
}

void CWorkThread::RemovePowerOnPacket()
{
	Mutex_Lock();
	for( int i=0; i<10; i++ )
	{
		if ( _laneMsg[i].msg_body[2] == 'A' )
		{
			_laneMsg[i].msg_state = MST_IDLE;
			break;
		}
	}
	Mutex_Unlock();
}

bool CWorkThread::IsValidPacket( const char *packet )
{
	static struct msg_codelen {
		int	code;
		int	len;
	} MsgCodeLen[] = {
		{ 0x30, MSG30_LEN },
		{ 0x31, MSG31_LEN },
		{ 0x61, MSG61_LEN },		/* 'a' - initial packet */
		{ 0x62, MSG62_LEN },		/* 'b' - validation done */
		{ 0x63, MSG63_LEN },		/* 'c' - validation failed */
		{ 0x64, MSG64_LEN },		/* 'd' - select channel */ 
		{ 0x65, MSG65_LEN },		/* 'e' - query status */
		{ 0x66, MSG66_LEN },		/* 'f' - query cartridge */
		{ 0x67, MSG67_LEN },		/* 'g' - cancel button */
		{ 0x68, MSG68_LEN },		/* 'h' - antenna switch */
		{ 0x69, MSG69_LEN },		/* 'i' - channel switch */
		{ 0x6a, MSG6A_LEN },		/* 'j' - set cartridge data */
#ifdef ENABLE_FUJIAN_PROTOCOL
		{ 0x6b, MSG6B_LEN },		/* 'k' - query cartridge attribute - fujian only */
		{ 0x72, MSG72_LEN },		/* 'r' - enable automatic prepare card for specified cartridge */
#endif
		{ 0x6c, MSG6C_LEN },		/* 'l' - remote trigger button */
		{ 0x6d, MSG6D_LEN },		/* 'm' - force eject */
		{ 0x6e, MSG6E_LEN },		/* 'n' - force recycle */
		{ 0x74, MSG74_LEN },		/* 't' - time sync */
		{ 0x75, MSG75_LEN },		/* 'u' - program log output */
	};
	int	i, len = strlen( packet );

	if ( packet[0] == MSG_STX && packet[len-1] == MSG_ETX && packet[1] >= '0' && packet[1] <= '9' )
	{
		for( i=0; i<sizeof(MsgCodeLen) / sizeof(struct msg_codelen); i++ )
		{
			if ( MsgCodeLen[i].code == packet[2] && MsgCodeLen[i].len == len )
				return true;
		}
	}
	return false;
}

// Note - Caller shall hold the lock of mutex when use this function
bool CWorkThread::IsEmptyQueue( void )
{
	int	i;

	for( i=0; i<10; i++ )
	{
		if ( _laneMsg[i].msg_state != MST_IDLE )
			return false;
	}
	return true;
}

int	 CWorkThread::GetTime2Resend( void )
{
	int		i,	t2resend=0;
	_longtime	tmsNow;
	
	Mutex_Lock();
	tmsNow = timeGetLongTime();
	for( i=0; i<10; i++ )
	{
		if ( _laneMsg[i].msg_state == MST_WAITACK )
		{
			if ( tmsNow >= _laneMsg[i].t_resend )
			{
				tty_write( m_tty, _laneMsg[ i ].msg_body, _laneMsg[ i ].msg_len );
				_laneMsg[ i ].t_resend = tmsNow + MST_TIMEOUT;
				m_pDlg->AppendTxPacket( _laneMsg[ i ].msg_body, NULL );
			}
			t2resend = (int)(_laneMsg[i].t_resend - tmsNow);
			break;
		}
	}
	Mutex_Unlock();
	return t2resend;
}

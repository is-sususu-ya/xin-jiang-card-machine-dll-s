#ifndef _H_UTILS_MACRO_INCL_
#define _H_UTILS_MACRO_INCL_

#define ENABLE_CONTROL( Class, IDC, flag )	( (##Class *)GetDlgItem( IDC ) )->EnableWindow( flag )
#define GET_CONTROL( Class, IDC )			( (##Class *)GetDlgItem( IDC ) )
#define SET_WNDTEXT( IDC, Text )			( GetDlgItem( IDC ) ) ->SetWindowText( Text )
#define GET_WNDTEXT( IDC, Text, Size )		( GetDlgItem( IDC ) ) ->GetWindowText( Text, Size )

#define GET_WNDSTRING( IDC, String )		( GetDlgItem( IDC ) ) ->GetWindowText( String )
#define ENABLE_CWND( IDC, flag )			( GetDlgItem( IDC ) )->EnableWindow( flag )
#define SET_FOCUS( IDC )					( GetDlgItem( IDC ) )->SetFocus()
#define SET_CAPTURE( IDC )					( GetDlgItem( IDC ) )->SetCapture()
#define GET_HWND( IDC )						( GetDlgItem( IDC ) )->m_hWnd
#define IS_CHECKED( IDC )					(((CButton *)GetDlgItem(IDC)) ->GetCheck() == BST_CHECKED )
#define SET_CHECK( IDC, bCheck )			((CButton *)GetDlgItem(IDC)) ->SetCheck( bCheck ? BST_CHECKED : BST_UNCHECKED )

#define CURSOR_WAIT()						SetCursor( LoadCursor( NULL, IDC_WAIT ) )
#define CURSOR_ARROW()						SetCursor( LoadCursor( NULL, IDC_ARROW ) )
#define CURSOR_HAND()						SetCursor( LoadCursor( NULL, IDC_HAND ) )
#define CURSOR_CROSS()						SetCursor( LoadCursor( NULL, IDC_CROSSHAIR ) )

#define IS_CHINESE()						(GetUserDefaultLCID()==2052)

#define GET_WNDINT(IDC, val) \
	do { \
		char buf[24]; \
		GetDlgItem(IDC)->GetWindowText(buf, 24); \
		val = atoi(buf); \
	} while (0)

#define SET_WNDINT(IDC, val) \
	do { \
	char buf[16]; \
	sprintf(buf, "%d", val); \
	GetDlgItem(IDC)->SetWindowText(buf); \
	} while (0)

#define GET_WNDFLOAT( IDC, value )		\
	do{\
		char buf[24] = {0};\
		GetDlgItem(IDC)->GetWindowText(buf, 24 ); \
		value = atof(buf);\
	} while (0)



#define SET_WNDFLOAT(IDC, val, nfrace) \
	do { \
		char buf[24]; \
		sprintf(buf, "%.*f", nfrace, val ); \
		GetDlgItem( IDC ) ->SetWindowText( buf ); \
	} while (0)
	

#define CLAMP_VAL( val, minv, maxv ) \
	do { \
		if ( val < minv ) \
			val = minv; \
		else if ( val > maxv ) \
			val = maxv; \
	} while (0) 

// better psuedo random number - more uniform distributed at lower bits range
// brand(n) result will be any number between 0 ~ n-1 (inclusive)
#define  brand(n)	(int)(rand() / (((double)RAND_MAX + 1)/ (n)))

#endif
# Microsoft Developer Studio Project File - Name="TCR8Tester" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=TCR8Tester - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TCR8Tester.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TCR8Tester.mak" CFG="TCR8Tester - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TCR8Tester - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "TCR8Tester - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TCR8Tester - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ../TCR8Lib/Release/TCR8Lib.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/TCR8LibTester.exe"

!ELSEIF  "$(CFG)" == "TCR8Tester - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../TCR8Lib/Debug/TCR8Lib.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/TCR8LibTester.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TCR8Tester - Win32 Release"
# Name "TCR8Tester - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ProtocolViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\SetBoxInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TCR8LogViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\TCR8Tester.cpp
# End Source File
# Begin Source File

SOURCE=.\TCR8Tester.rc
# End Source File
# Begin Source File

SOURCE=.\TCR8TesterDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\TextProgressBar.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ProtocolViewer.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SetBoxInfo.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TCR8LogViewer.h
# End Source File
# Begin Source File

SOURCE=.\TCR8Tester.h
# End Source File
# Begin Source File

SOURCE=.\TCR8TesterDlg.h
# End Source File
# Begin Source File

SOURCE=.\TextProgressBar.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\1_G_54X50.bmp
# End Source File
# Begin Source File

SOURCE=.\res\1_R_54X50.bmp
# End Source File
# Begin Source File

SOURCE=.\res\2_G_54X50.bmp
# End Source File
# Begin Source File

SOURCE=.\res\2_R_54X50.bmp
# End Source File
# Begin Source File

SOURCE=.\res\3_G_54X50.bmp
# End Source File
# Begin Source File

SOURCE=.\res\3_R_54X50.bmp
# End Source File
# Begin Source File

SOURCE=.\res\4_G_54X50.bmp
# End Source File
# Begin Source File

SOURCE=.\res\4_R_54X50.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap_a.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CardOnReader_64X48.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CSCExit1_64X38.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CSCOnExit1_64X38.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Reader_64X48.bmp
# End Source File
# Begin Source File

SOURCE=.\res\TCR8Tester.ico
# End Source File
# Begin Source File

SOURCE=.\res\TCR8Tester.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project

# Microsoft Developer Studio Project File - Name="TecKitJni" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TecKitJni - Win32 TestDebug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TecKitJni.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TecKitJni.mak" CFG="TecKitJni - Win32 TestDebug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TecKitJni - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TecKitJni - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TecKitJni - Win32 TestDebug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TecKitJni - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TECKITJNI_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TECKITJNI_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "TecKitJni - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TECKITJNI_EXPORTS" /Yu"stdafx.h" /FD /GZ  /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\\" /I "..\..\zlib\src" /I "C:\Program Files\j2sdk_nb\j2sdk1.4.2\include" /I "C:\Program Files\j2sdk_nb\j2sdk1.4.2\include\win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TECKITJNI_EXPORTS" /FD /GZ  /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "TecKitJni - Win32 TestDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TestDebug"
# PROP BASE Intermediate_Dir "TestDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "TestDebug"
# PROP Intermediate_Dir "TestDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\\" /I "..\..\zlib\src" /I "C:\Program Files\j2sdk_nb\j2sdk1.4.2\include" /I "C:\Program Files\j2sdk_nb\j2sdk1.4.2\include\win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TECKITJNI_EXPORTS" /FD /GZ  /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\\" /I "..\..\zlib\src" /I "C:\Program Files\j2sdk_nb\j2sdk1.4.2\include" /I "C:\Program Files\j2sdk_nb\j2sdk1.4.2\include\win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FD /GZ  /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "TecKitJni - Win32 Release"
# Name "TecKitJni - Win32 Debug"
# Name "TecKitJni - Win32 TestDebug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\zlib\src\adler32.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\compress.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\crc32.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\deflate.c
# End Source File
# Begin Source File

SOURCE=..\src\Engine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\gzio.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\infblock.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\infcodes.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\inffast.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\inflate.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\infutil.c
# End Source File
# Begin Source File

SOURCE=..\src\teckitjni.cpp
# End Source File
# Begin Source File

SOURCE=..\src\teckitjniTest.cpp

!IF  "$(CFG)" == "TecKitJni - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "TecKitJni - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "TecKitJni - Win32 TestDebug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\trees.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\zutil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\zlib\src\deflate.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\infblock.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\infcodes.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\inffast.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\infutil.h
# End Source File
# Begin Source File

SOURCE=.\TecKitJni.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\trees.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\zconf.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\zlib.h
# End Source File
# Begin Source File

SOURCE=..\..\zlib\src\zutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project

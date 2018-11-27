rem
rem Copyright (c) 2017-$Date: 2017-06-16 16:54:24 -0500 (Fri, 16 Jun 2017) $ TIBCO Software Inc. 
rem ALL RIGHTS RESERVED
rem
rem $Id: register.bat 94086 2017-06-16 21:54:24Z vinasing $
rem
rem Batch file for registering the TIBCO EMS C# client, UFO and admin libraries into GAC
rem

rem the TIBCO EMS C# client, UFO and admin libraries
set CSLIBCLIENT=..\..\..\bin\TIBCO.EMS.dll
set CSLIBUFOCLIENT=..\..\..\bin\TIBCO.EMS.UFO.dll
set CSLIBADMIN=..\..\..\bin\TIBCO.EMS.ADMIN.dll

rem the path where gacutil.exe is located (to be adjusted to your specific environment)
set GACPATH="C:\Program Files\Microsoft Visual Studio .NET\FrameworkSDK\bin"

%GACPATH%\gacutil.exe /i %CSLIBCLIENT%
%GACPATH%\gacutil.exe /i %CSLIBUFOCLIENT%
%GACPATH%\gacutil.exe /i %CSLIBADMIN%

set CSLIBCLIENT=
set CSLIBUFOCLIENT=
set CSLIBADMIN=
set GACPATH=

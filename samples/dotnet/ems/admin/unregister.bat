rem
rem Copyright (c) 2017-$Date: 2017-06-16 16:54:24 -0500 (Fri, 16 Jun 2017) $ TIBCO Software Inc. 
rem ALL RIGHTS RESERVED
rem
rem $Id: unregister.bat 94086 2017-06-16 21:54:24Z vinasing $
rem
rem Batch file for unregistering the TIBCO EMS C# client, UFO and admin libraries from GAC
rem

rem the TIBCO EMS C# client, UFO and admin libraries
set CSLIBCLIENT=TIBCO.EMS
set CSLIBUFOCLIENT=TIBCO.EMS.UFO
set CSLIBADMIN=TIBCO.EMS.ADMIN

rem the path where gacutil.exe is located (to be adjusted to your specific environment)
set GACPATH="C:\Program Files\Microsoft Visual Studio .NET\FrameworkSDK\bin"

%GACPATH%\gacutil.exe /u %CSLIBCLIENT%
%GACPATH%\gacutil.exe /u %CSLIBUFOCLIENT%
%GACPATH%\gacutil.exe /u %CSLIBADMIN%

set CSLIBCLIENT=
set CSLIBUFOCLIENT=
set CSLIBADMIN=
set GACPATH=

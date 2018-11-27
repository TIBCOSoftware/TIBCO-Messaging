rem
rem Copyright (c) 2001-$Date: 2017-06-16 16:54:24 -0500 (Fri, 16 Jun 2017) $ by TIBCO Software Inc.
rem ALL RIGHTS RESERVED
rem
rem $Id: register.bat 94086 2017-06-16 21:54:24Z vinasing $
rem
rem Batch file for registering the TIBCO EMS C# client and UFO libraries into GAC
rem

rem the TIBCO EMS C# client and UFO libraries
set CSLIB=..\..\bin\TIBCO.EMS.dll
set CSUFOLIB=..\..\bin\TIBCO.EMS.UFO.dll

rem the path where gacutil.exe is located (to be adjusted to your specific environment)
set GACPATH="C:\Program Files\Microsoft Visual Studio .NET\FrameworkSDK\bin"

%GACPATH%\gacutil.exe /i %CSLIB% 
%GACPATH%\gacutil.exe /i %CSUFOLIB%

set CSLIB=
set CSUFOLIB=
set GACPATH=

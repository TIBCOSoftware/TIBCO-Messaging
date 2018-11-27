@echo off
rem ## 
rem ##
rem ## Copyright (c) 2001-$Date: 2018-07-20 16:23:19 -0500 (Fri, 20 Jul 2018) $ by TIBCO Software Inc.
rem ## ALL RIGHTS RESERVED
rem ##
rem ## $Id: setup.bat 102524 2018-07-20 21:23:19Z olivierh $
rem ##
rem ## This setup file can be used to setup the environment for 
rem ## compiling and running EMS samples with Java
rem ## 

rem ## 
rem ## Set TIBEMS_ROOT to the root of your installation of
rem ## TIBCO Enterprise Message Service software
rem ## 

IF NOT "%TIBEMS_ROOT%"=="" goto cont

set TIBEMS_ROOT=..\..

:cont

rem ## 
rem ## You should not need to change the text below
rem ## 

set TIBEMS_JAVA=%TIBEMS_ROOT%\lib


if NOT EXIST %TIBEMS_JAVA%\jms-2.0.jar goto badenv
if NOT EXIST %TIBEMS_JAVA%\tibjms.jar goto badenv
if NOT EXIST %TIBEMS_JAVA%\tibjmsadmin.jar goto badenv

set CLASSPATH=%TIBEMS_JAVA%\jms-2.0.jar;%CLASSPATH%
set CLASSPATH=.;%TIBEMS_JAVA%\tibjms.jar;%TIBEMS_JAVA%\tibjmsadmin.jar;%CLASSPATH%
set CLASSPATH=%TIBEMS_JAVA%\tibjmsufo.jar;%CLASSPATH%

goto end

:badenv
echo .
echo Error: TIBEMS_ROOT variable is not set or does not correctly specify
echo the root directory of the TIBCO Enterprise Message Service software. 
echo Please correct the TIBEMS_ROOT variable at the beginning of this script.
echo .

:end


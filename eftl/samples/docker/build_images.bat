@ECHO OFF
::
:: Copyright (c) 2016-2022 TIBCO Software Inc.
:: All Rights Reserved. Confidential & Proprietary.


SET BASE_IMAGE=redhat/ubi8
set BASE=%~dp0

:getoptstart
IF "%1"=="-h" GOTO :help
IF "%1"=="--help" GOTO :help
IF "%1"=="--base" GOTO :baseimg
IF "%1"=="--ftl" GOTO :ftlzip
IF "%1"=="--eftl" GOTO :eftlzip
GOTO :getoptend
:help
ECHO "build-images.bat [-h|--help] [--base <base image:tag>] --ftl <path to linux FTL install package> --eftl <path to linux eFTL install package>"
exit /b 0
:baseimg
SHIFT
SET BASE_IMAGE=%1
GOTO :nextarg
:ftlzip
SHIFT
SET FTL_PKG=%1
GOTO :nextarg
:eftlzip
SHIFT
SET EFTL_PKG=%1
GOTO :nextarg
:nextarg
SHIFT
GOTO :getoptstart
:getoptend

IF "%FTL_PKG%"=="" (
    echo "FTL zip file not provided"
    exit /b 1
)
IF NOT EXIST "%FTL_PKG%" (
    echo "FTL zip file not found"
    exit /b 1
)
IF "%EFTL_PKG%"=="" (
    echo "EFTL zip file not provided"
    exit /b 1
)
IF NOT EXIST "%EFTL_PKG%" (
    echo "EFTL zip file not found"
    exit /b 1
)

REM unzip the pkg
call :unzip_pkg %FTL_PKG% %EFTL_PKG%

REM ============================
REM build images
REM ============================

:build_images
REM get package version
FOR /F "usebackq tokens=*" %%G in (`docker run --rm ftleftlpkg /bin/sh -c "cd /tmp/ftl/TIB_ftl_*; echo tar/TIB_ftl*_linux_x86_64-runtime.tar.gz | grep -o -E '\d+\.\d+\.\d+'"`) DO (SET FTL_PKG_VERSION=%%G)
FOR /F "usebackq tokens=*" %%G in (`docker run --rm ftleftlpkg /bin/sh -c "cd /tmp/ftl/TIB_ftl_*; echo tar/TIB_ftl*_linux_x86_64-runtime.tar.gz | grep -o -E '\d+\.\d+'"`) DO (SET FTL_PKG_VERSION_SHORT=%%G)

FOR /F "usebackq tokens=*" %%G in (`docker run --rm ftleftlpkg /bin/sh -c "cd /tmp/eftl/TIB_eftl_*; echo tar/TIB_eftl*_linux_x86_64-runtime.tar.gz | grep -o -E '\d+\.\d+\.\d+'"`) DO (SET EFTL_PKG_VERSION=%%G)
FOR /F "usebackq tokens=*" %%G in (`docker run --rm ftleftlpkg /bin/sh -c "cd /tmp/eftl/TIB_eftl_*; echo tar/TIB_eftl*_linux_x86_64-runtime.tar.gz | grep -o -E '\d+\.\d+'"`) DO (SET EFTL_PKG_VERSION_SHORT=%%G)

REM build tibftlserver image
docker build -t eftl-tibftlserver:%FTL_PKG_VERSION% --build-arg BASE_IMAGE --build-arg FTL_PKG_VERSION --build-arg FTL_PKG_VERSION_SHORT --build-arg EFTL_PKG_VERSION --build-arg EFTL_PKG_VERSION_SHORT --target eftl-tibftlserver - < "%BASE%/Dockerfile.eftl"

REM remove docker images
docker rmi ftleftlpkg
goto :eof

REM ======================================
REM function to uzip the package
REM ======================================
:unzip_pkg

set FTL_PKG=%~1
set EFTL_PKG=%~2
setlocal
for /F "tokens=*" %%a in ('docker create alpine sh -c "unzip -d /tmp/ftl /tmp/pkg-ftl.zip; unzip -d /tmp/eftl /tmp/pkg-eftl.zip"') do set cid=%%a

docker cp %FTL_PKG% %cid%:/tmp/pkg-ftl.zip
docker cp %EFTL_PKG% %cid%:/tmp/pkg-eftl.zip

docker start %cid%
for /F "tokens=*" %%a in ('docker wait %cid%') do set result=%%a
if %result% NEQ 0 (
    ECHO "Failed to unzip FTL and eFTL packages"
    docker rm %cid%
    EXIT /b 1
)
docker commit %cid% ftleftlpkg
docker rm %cid%
goto :build_images

endlocal
:eof
exit /b %ERRORLEVEL%

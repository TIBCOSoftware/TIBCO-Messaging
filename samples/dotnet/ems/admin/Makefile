#
# Copyright (c) 2017-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
# ALL RIGHTS RESERVED
#
# $Id: Makefile 92619 2017-03-27 21:01:48Z olivierh $
# 
# Makefile for building C# admin samples
#

# Set TIBEMS_ROOT to the root of your installation of
# TIBCO Enterprise Message Service software
TIBEMS_ROOT=..\..\..

REF=$(TIBEMS_ROOT)\bin\TIBCO.EMS.dll
ADMINREF=$(TIBEMS_ROOT)\bin\TIBCO.EMS.ADMIN.dll

all:    prepare build

prepare:
	copy $(REF) TIBCO.EMS.dll
	copy $(ADMINREF) TIBCO.EMS.ADMIN.dll

build:
	csc  /r:$(REF) /r:$(ADMINREF)  /t:exe /out:csACL.exe                csACL.cs 
	csc  /r:$(REF) /r:$(ADMINREF)  /t:exe /out:csConnectionFactory.exe  csConnectionFactory.cs 
	csc  /r:$(REF) /r:$(ADMINREF)  /t:exe /out:csRoute.exe              csRoute.cs 
	csc  /r:$(REF) /r:$(ADMINREF)  /t:exe /out:csServer.exe             csServer.cs 
	csc  /r:$(REF) /r:$(ADMINREF)  /t:exe /out:csUsersGroups.exe        csUsersGroups.cs
	
clean:
	del /Q *.exe

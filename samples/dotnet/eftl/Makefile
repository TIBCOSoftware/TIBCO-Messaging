######################################################################
# Copyright (c) 2009-$Date: 2018-09-04 17:57:51 -0500 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
# ALL RIGHTS RESERVED
# For more information, please contact:
# TIBCO Software Inc., Palo Alto, California, USA
#
# TIBCO EFTL
# Sample Windows Makefile for C#
######################################################################

REFS = $(TIBEFTL_DLL)\TIBCO.EFTL.dll

all:    build

build:
	copy $(REFS) .
	csc /r:$(REFS) /o+ /t:exe /out:Publisher.exe  Publisher.cs 
	csc /r:$(REFS) /o+ /t:exe /out:Subscriber.exe  Subscriber.cs
	csc /r:$(REFS) /o+ /t:exe /out:SharedDurable.exe  SharedDurable.cs
	csc /r:$(REFS) /o+ /t:exe /out:LastValueDurable.exe  LastValueDurable.cs
	csc /r:$(REFS) /o+ /t:exe /out:KVSet.exe  KVSet.cs
	csc /r:$(REFS) /o+ /t:exe /out:KVGet.exe  KVGet.cs
	csc /r:$(REFS) /o+ /t:exe /out:KVRemove.exe  KVRemove.cs

clean:
	del /Q *.exe


#
# Copyright (c) 2001-$Date: 2013-08-21 15:28:23 -0500 (Wed, 21 Aug 2013) $ by TIBCO Software Inc.
# ALL RIGHTS RESERVED
#
# $Id: Makefile 68846 2013-08-21 20:28:23Z rkutter $
# 
# Makefile for building C# client samples
#

# Set TIBEMS_ROOT to the root of your installation of
# TIBCO Enterprise Message Service software
TIBEMS_ROOT=..\..

REF=$(TIBEMS_ROOT)\bin\TIBCO.EMS.dll
UFOREF=$(TIBEMS_ROOT)\bin\TIBCO.EMS.UFO.dll

DEPENDENT_FILES = tibemsUtilities.cs

all:    prepare build

prepare:
	copy $(REF) TIBCO.EMS.dll
	copy $(UFOREF) TIBCO.EMS.UFO.dll

build:
	csc  /r:$(REF) /t:exe /out:csAsyncMsgConsumer.exe  csAsyncMsgConsumer.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:csAsyncMsgConsumerUsingDelegate.exe  csAsyncMsgConsumerUsingDelegate.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:csBrowser.exe           csBrowser.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:csDurable.exe           csDurable.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:csExtensions.exe        csExtensions.cs
	csc  /r:$(REF) /t:exe /out:csLookup.exe            csLookup.cs
	csc  /r:$(REF) /t:exe /out:csMsgConsumer.exe       csMsgConsumer.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:csMsgProducer.exe       csMsgProducer.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:csMsgProducerPerf.exe   csMsgProducerPerf.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:csMsgConsumerPerf.exe   csMsgConsumerPerf.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:csSelector.exe          csSelector.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /r:$(UFOREF) /t:exe /out:csUFOLookup.exe    csUFOLookup.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /r:$(UFOREF) /t:exe /out:csUFOMsgProducer.exe    csUFOMsgProducer.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /r:$(UFOREF) /t:exe /out:csUFOMsgConsumer.exe    csUFOMsgConsumer.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /r:$(UFOREF) /t:exe /out:csUFOAsyncMsgConsumer.exe   csUFOAsyncMsgConsumer.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:emsSSLGlobal.exe         emsSSLGlobal.cs
	csc  /r:$(REF) /t:exe /out:emsSSLSampleClient.exe   emsSSLSampleClient.cs
	csc  /r:$(REF) /t:exe /out:tibemsSSL.exe            tibemsSSL.cs
	csc  /r:$(REF) /t:exe /out:csLookupSSL.exe          csLookupSSL.cs
	csc  /r:$(REF) /t:exe /out:csLookupLDAP.exe         csLookupLDAP.cs
	csc  /r:$(REF) /t:exe /out:csMSDTCProducer.exe      csMSDTCProducer.cs $(DEPENDENT_FILES)
	csc  /r:$(REF) /t:exe /out:csMSDTCConsumer.exe      csMSDTCConsumer.cs $(DEPENDENT_FILES)


clean:
	del /Q *.exe


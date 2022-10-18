#
# Makefile for C samples
#
# $RCSfile: GNUmakefile,v $
# $Revision: 1.31 $
# $Date: 2018-07-20 16:23:19 -0500 (Fri, 20 Jul 2018) $
#

#################################################################
#
# list of utility files needed by all tests
#
#################################################################

UTILS = tibemsUtilities$(O)

#################################################################
#
# list of all samples, modify this when add/remove samples
#
#################################################################

ifndef JMSOS_VMS
ifndef JMSOS_WIN
 LOOKUP_SAMPLES = \
        tibemsLookup$(EXE) \
	tibemsLookup-static$(EXE)
endif
endif

ifdef JMSOS_WIN
 LOOKUP_SAMPLES = \
        tibemsLookup$(EXE) 
endif

ifdef JMSOS_VMS
 SHR_SAMPLES = \
	tibemsMsgProducerShr$(EXE) \
	tibemsMsgConsumerShr$(EXE)
endif

SAMPLES = \
	tibemsDurable$(EXE) \
	tibemsMsgProducer$(EXE) \
	tibemsMsgConsumer$(EXE) \
	tibemsUFOMsgConsumer$(EXE) \
	tibemsAsyncMsgConsumer$(EXE) \
	tibemsBrowser$(EXE) \
	tibemsMsg$(EXE) \
	tibemsMsgProducerPerf$(EXE) \
	tibemsMsgConsumerPerf$(EXE) \
	tibemsAdmin$(EXE) \
        $(LOOKUP_SAMPLES) $(SHR_SAMPLES)


# VMS uses the samples for testing and the producer and consumer
# samples for installation verification.
ifdef JMSOS_VMS
 EXPORT_INC =
 EXPORT_LIB =
 EXPORT_BIN = $(SAMPLES)
 SHIP_INC   =
 SHIP_LIB   =
 SHIP_BIN   = $(SHR_SAMPLES)
endif

ifndef JMSOS_VMS
 BASEDIR = $(JMS_SRC)$(rdir)$(PS)
 SMPFILES = $(wildcard $(BASEDIR)*.*)
 SAMPLES = $(subst $(BASEDIR),,$(SMPFILES))
 EXPORT_CE_SAMPLES := Makefile.linux Makefile.macosx Makefile.w32
 EXPORT_SAMPLES := $(filter-out $(EXPORT_CE_SAMPLES), $(SAMPLES))
endif

#################################################################
# normally no need to modify below
#################################################################

DEFAULT = $(SAMPLES)

include $(JMS_MINCLUDE)

tibemsDurable$(EXE): tibemsDurable$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsMsgProducer$(EXE): tibemsMsgProducer$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsMsgProducerShr$(EXE): tibemsMsgProducer$(O) $(UTILS)
	$(link) $(SHARED_LIBJMS) $(SHARED_LIBOPENSSL) $(SHARED_LIBZ)

tibemsMsgConsumer$(EXE): tibemsMsgConsumer$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsUFOMsgConsumer$(EXE): tibemsUFOMsgConsumer$(O) $(UTILS) \
	$(LIBJMS) $(LIBUFO) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsMsgConsumerShr$(EXE): tibemsMsgConsumer$(O) $(UTILS)
	$(link) $(SHARED_LIBJMS) $(SHARED_LIBOPENSSL) $(SHARED_LIBZ)

tibemsMsg$(EXE): tibemsMsg$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsLookup$(EXE): tibemsLookup$(O) $(UTILS)
	$(link) $(SHARED_LIBLOOKUP) $(SHARED_LIBJMS) $(SHARED_LIBLDAP) $(SHARED_LIBOPENSSL) $(SHARED_LIBXML2) $(SHARED_LIBZ)

ifndef JMSOS_WIN
tibemsLookup-static$(EXE): tibemsLookup$(O) $(UTILS) \
	$(LIBLOOKUP) $(LIBJMS) $(LIBLDAP) $(LIBOPENSSL) $(LIBXML2) $(LIBZ)
	$(link)
endif

tibemsAsyncMsgConsumer$(EXE): tibemsAsyncMsgConsumer$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsBrowser$(EXE): tibemsBrowser$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsMsgProducerPerf$(EXE): tibemsMsgProducerPerf$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsMsgConsumerPerf$(EXE): tibemsMsgConsumerPerf$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsXAMsgProducer$(EXE): tibemsXAMsgProducer$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsXAMsgConsumer$(EXE): tibemsXAMsgConsumer$(O) $(UTILS) \
	$(LIBJMS) $(LIBOPENSSL) $(LIBZ)
	$(link)

tibemsAdmin$(EXE): tibemsAdmin$(O) $(UTILS) \
	$(LIBJMS) $(LIBADMIN) $(LIBOPENSSL) $(LIBZ)
	$(link)

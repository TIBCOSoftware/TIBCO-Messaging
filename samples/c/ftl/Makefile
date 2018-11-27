######################################################################
# Copyright (c) 2009-2018 TIBCO Software Inc.
# Licensed under a BSD-style license. Refer to [LICENSE]
# For more information, please contact:
# TIBCO Software Inc., Palo Alto, California, USA
#
# TIBCO FTL
# Sample Darwin Makefile
######################################################################

TIBFTL_DIR=../../..

CC = clang
COMP_FLAGS = -std=gnu11 -m64 -O2 -Wall -Wshadow
LDFLAGS=-L$(TIBFTL_DIR)/lib -Wl,-rpath,${TIBFTL_ROOT}
INCFILES=-I$(TIBFTL_DIR)/include
CFLAGS=$(COMP_FLAGS) $(INCFILES)

TIBFTL_LIB=-ltib -ltibgroup -ltibutil
LIBS= $(TIBFTL_LIB)

PROGS = tibsendex tibrecvex tibrequest tibreply tiblatsend tiblatrecv tibthrusend tibthrusendd tibthrurecv tibthrurecvd tibgrp tibmapset tibmapget directlatsend directlatrecv tibsend tibrecv tibmonsub tiblogsub
all: $(PROGS)

clean: 	
	rm -f *.o core*
	rm -f $(PROGS)

tibsendex: tibsendex.o tibaux.o
	$(CC) $(LDFLAGS) tibsendex.o tibaux.o $(LIBS) -o $@

tibrecvex: tibrecvex.o tibaux.o
	$(CC) $(LDFLAGS) tibrecvex.o tibaux.o $(LIBS) -o $@

tibrequest: tibrequest.o tibaux.o
	$(CC) $(LDFLAGS) tibrequest.o tibaux.o $(LIBS) -o $@

tibreply: tibreply.o tibaux.o
	$(CC) $(LDFLAGS) tibreply.o tibaux.o $(LIBS) -o $@

tiblatsend: tiblatsend.o tibaux.o
	$(CC) $(LDFLAGS) tiblatsend.o tibaux.o $(LIBS) -o $@

tiblatrecv: tiblatrecv.o tibaux.o
	$(CC) $(LDFLAGS) tiblatrecv.o tibaux.o $(LIBS) -o $@

tibthrusend: tibthrusend.o tibaux.o
	$(CC) $(LDFLAGS) tibthrusend.o tibaux.o $(LIBS) -o $@

tibthrusendd: tibthrusendd.o tibaux.o
	$(CC) $(LDFLAGS) tibthrusendd.o tibaux.o $(LIBS) -o $@

tibthrurecv: tibthrurecv.o tibaux.o
	$(CC) $(LDFLAGS) tibthrurecv.o tibaux.o $(LIBS) -o $@

tibthrurecvd: tibthrurecvd.o tibaux.o
	$(CC) $(LDFLAGS) tibthrurecvd.o tibaux.o $(LIBS) -o $@

tibgrp: tibgrp.o tibaux.o
	$(CC) $(LDFLAGS) tibgrp.o tibaux.o $(LIBS) -o $@

tibmapset: tibmapset.o tibaux.o
	$(CC) $(LDFLAGS) tibmapset.o tibaux.o $(LIBS) -o $@

tibmapget: tibmapget.o tibaux.o
	$(CC) $(LDFLAGS) tibmapget.o tibaux.o $(LIBS) -o $@

directlatsend: directlatsend.o tibaux.o
	$(CC) $(LDFLAGS) directlatsend.o tibaux.o $(LIBS) -o $@

directlatrecv: directlatrecv.o tibaux.o
	$(CC) $(LDFLAGS) directlatrecv.o tibaux.o $(LIBS) -o $@

tibsend: tibsend.o
	$(CC) $(LDFLAGS) tibsend.o $(LIBS) -o $@

tibrecv: tibrecv.o
	$(CC) $(LDFLAGS) tibrecv.o $(LIBS) -o $@

tibmonsub: tibmonsub.o tibaux.o
	$(CC) $(LDFLAGS) tibmonsub.o tibaux.o $(LIBS) -o $@

tiblogsub: tiblogsub.o tibaux.o
	$(CC) $(LDFLAGS) tiblogsub.o tibaux.o $(LIBS) -o $@

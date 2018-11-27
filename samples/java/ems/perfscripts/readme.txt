 =================================================================
 Copyright (c) 2001-$Date: 2013-08-21 15:28:23 -0500 (Wed, 21 Aug 2013) $ by TIBCO Software Inc.
 ALL RIGHTS RESERVED

 $Id: readme.txt 68846 2013-08-21 20:28:23Z rkutter $
 =================================================================


This directory contains sample scripts which can be used to evaluate the
capacity of the EMS server on a particular hardware platform.

Shell Scripts
-------------

msgmodes.sh

	This is a sample script which initializes a set of environment
	variables to vary the modes of producing and consuming messages
	in EMS.  It invokes the pcthreads.sh script for each given mode
	to differ the number of threads.  The output of this script is
	a file called msgmodes.txt.

pcthreads.sh

	This is a simple script that for a given mode of operation runs
	from 1 producer/consumer thread up to 100 producer/consumer
	threads.


Perl scripts
------------

msgmodes.prl

	This is a simple perl script that extracts the relevant lines
	from the final output from msgmodes.sh (via stdin) and outputs
	them (via stdout).  The output of this can be quickly pulled
	into excel and used to create excel graphs.  Use the excel chart
	wizard, select a line type graph.  In the "series" tab, copy the
	"Values" line from the "Threads" series into the x axis labels.
	Then remove the "Threads" series.

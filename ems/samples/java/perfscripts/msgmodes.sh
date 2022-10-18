#!/bin/sh

#
# Copyright (c) 2001-$Date: 2013-08-21 15:28:23 -0500 (Wed, 21 Aug 2013) $ by TIBCO Software Inc.
# ALL RIGHTS RESERVED
#
# $Id: msgmodes.sh 68846 2013-08-21 20:28:23Z rkutter $
#

if [ -z "${TIBEMS_SERVER}" ]
then
	echo Please set TIBEMS_SERVER variable.
	exit
fi

rm tmp.txt
export TIBEMS_OUTPUT=tmp.txt

# fastest, least reliable TIBCO extension
export TIBEMS_DEST="-topic topic -uniquedests"
export TIBEMS_MODE="-delivery RELIABLE -ackmode NO"
pcthreads.sh
mv tmp.txt msgmodes.txt

# fastest, least reliable JMS compliant
export TIBEMS_DEST="-topic topic -uniquedests"
export TIBEMS_MODE="-delivery NON_PERSISTENT -ackmode DUPS_OK"
pcthreads.sh
echo " " >> msgmodes.txt
cat tmp.txt >> msgmodes.txt
rm tmp.txt

# persistent, non-failsafe
export TIBEMS_DEST="-topic topic -uniquedests"
export TIBEMS_MODE="-delivery PERSISTENT -ackmode DUPS_OK"
pcthreads.sh
echo " " >> msgmodes.txt
cat tmp.txt >> msgmodes.txt
rm tmp.txt

# persistent, failsafe
rm persistfailsafe.txt
export TIBEMS_DEST="-topic topic -uniquedests"
export TIBEMS_MODE="-delivery PERSISTENT -ackmode DUPS_OK -storeName \$sys.failsafe -durable sub"
pcthreads.sh
echo " " >> msgmodes.txt
cat tmp.txt >> msgmodes.txt
rm tmp.txt

# transactional
rm persistfailsafe.txt
export TIBEMS_DEST="-topic topic -uniquedests"
export TIBEMS_MODE="-delivery PERSISTENT -storeName \$sys.failsafe -durable sub -constxnsize 10 -prodtxnsize 10"
pcthreads.sh
echo " " >> msgmodes.txt
cat tmp.txt >> msgmodes.txt
rm tmp.txt

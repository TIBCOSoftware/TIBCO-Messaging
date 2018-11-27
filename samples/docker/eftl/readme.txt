/*
 * Copyright (c) 2009-$Date: 2017-10-09 15:26:34 -0500 (Mon, 09 Oct 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

Make sure the FTL_EXTRACT, EFTL_EXTRACT enviroment variables are set to the 
location where FTL and EFTL were extracted and EFTL_INSTALL points to the 
location where EFTL is installed and FTL_INSTALL points to where FTL is installed.

% mkdir /tmp/build
% cd /tmp/build
% mkdir ftl
% mkdir eftl
% cp ${FTL_EXTRACT}/deb/*.deb ftl/
% cp ${EFTL_EXTRACT}/deb/*.deb eftl/
% cp -r ${EFTL_INSTALL}/samples eftl/

# build the tibeftlserver docker image
% docker build -f eftl/samples/docker/Dockerfile.tibeftlserver -t ftl-tibeftlserver .

# load the tibrealmserver.json from the {EFTL_INSTALL}/samples via the tibrealmadmin command from FTL

% ${FTL_INSTALL}/bin/tibrealmadmin -ur ${EFTL_INSTALL}/samples/tibrealmserver.json -rs <realm_server_url>

# Running tibeftlserver within a docker container.

# NOTE: Make sure the tibrealmserver and tibagent from ftl installation are running.
# For instructions on how to start tibrealmserver and tibagent please see the
# ${FTL_INSTALL}/samples/docker/README.txt. Also it's expected that there are no
# prior configurations loaded up in the tibrealmserver before loading the
# eftl tibrealserver sample json.

% docker run --rm -p 9191:9191 ftl-tibeftlserver -rs discover:// -l ws://*:9191

# Now you are ready to connect clients to the tibeftlserver

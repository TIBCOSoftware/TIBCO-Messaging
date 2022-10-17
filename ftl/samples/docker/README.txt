/*
 * Copyright (c) 2009-2018 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

Please refer to the README_docker.txt file in the docker-images directory for
information on how to load run the prebuilt docker images.


Build Client Images
===================================

Set the environment variable FTL_INSTALL to the path where you
installed FTL.  Also set the FTL_EXTRACT variable to the path where
you have extracted FTL. Verify that the samples subdirectory is
present in the FTL_INSTALL directory.

% mkdir /tmp/build
% cd /tmp/build
% mkdir ftl

# copy the ftl debian packages.
% cp ${FTL_EXTRACT}/deb/*.deb ftl/
% cp -r ${FTL_INSTALL}/samples ftl/
% docker build -f ftl/samples/docker/Dockerfile.client -t ftl-client .

Run sample clients
==================

This document assumes that the TibAgent is running on all hosts.

Load the sample realm configuration into the running realm server:

% docker run -it --rm ftl-client tibrealmadmin -rs <realmserver_host_name>:13131 -ur /opt/tibco/ftl/5.4/samples/scripts/tibrealmserver.json

Start tibrecvex:

% docker run -it --rm ftl-client tibrecvex -id tcp discover:// -c 600

Start tibsendex in a separate shell from tibrecvex:

% docker run -it --rm ftl-client tibsendex -id tcp discover:// -d 1000 -c 600

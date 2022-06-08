/*
 * @COPYRIGHT_BANNER@ 
 */

Linux
=============
Run the build_images.sh script by specifying the FTL and eFTL package zip files along
with the base os image. 

e.g.
./build_images.sh --ftl /tmp/TIB_ftl_@CPACK_PACKAGE_VERSION@_linux_x86_64.zip --eftl /tmp/TIB_eftl_@CPACK_PACKAGE_VERSION@_linux_x86_64.zip --base redhat/ubi8


Run eFTL sample clients
==================

Start the FTLserver that starts the eFTL service 
================================================================

% docker run -d -p 8585:8585 eftl-tibftlserver:@CPACK_PACKAGE_VERSION@

The FTLServer URL is http://<host_name>:8585

Running samples (see eFTL samples directory for instructions)
================================================================

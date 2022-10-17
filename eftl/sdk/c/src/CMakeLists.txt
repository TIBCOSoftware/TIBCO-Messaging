#
# Copyright (c) 2014-$Date: 2017-08-24 14:37:19 -0700 (Thu, 24 Aug 2017) $ TIBCO Software Inc.
# All rights reserved.
# For more information, please contact:
# TIBCO Software Inc., Palo Alto, California, USA
# 
# $Id: eftl.c 95684 2017-08-24 21:37:19Z bpeterse $
#
cmake_minimum_required(VERSION 2.8)

project(tibeftl)

# Collect source files
file(GLOB SOURCES *.c)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

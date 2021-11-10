#
# Copyright (c) 2014-2021 TIBCO Software Inc.
# Licensed under a BSD-style license. Refer to [LICENSE]
# For more information, please contact:
# TIBCO Software Inc., Palo Alto, California, USA
# 
# $Id: eftl.c 95684 2017-08-24 21:37:19Z $
#
cmake_minimum_required(VERSION 2.8)

project(tibeftl)

# Collect source files
file(GLOB SOURCES *.c)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

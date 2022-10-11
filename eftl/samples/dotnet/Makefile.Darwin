######################################################################
# Copyright (c) 2009-$Date: 2019-06-03 08:32:33 -0700 (Mon, 03 Jun 2019) $ TIBCO Software Inc.
# ALL RIGHTS RESERVED
# For more information, please contact:
# TIBCO Software Inc., Palo Alto, California, USA
#
# TIBCO EFTL
# Sample Makefile for C#
######################################################################

all:    build

build:
	dotnet build --configuration Release -f netcoreapp6.0 Publisher.csproj 
	dotnet build --configuration Release -f netcoreapp6.0 Subscriber.csproj 
	dotnet build --configuration Release -f netcoreapp6.0 SharedDurable.csproj 
	dotnet build --configuration Release -f netcoreapp6.0 LastValueDurable.csproj 
	dotnet build --configuration Release -f netcoreapp6.0 Request.csproj 
	dotnet build --configuration Release -f netcoreapp6.0 Reply.csproj 
	dotnet build --configuration Release -f netcoreapp6.0 KVSet.csproj 
	dotnet build --configuration Release -f netcoreapp6.0 KVGet.csproj 
	dotnet build --configuration Release -f netcoreapp6.0 KVRemove.csproj 

clean:
	rm bin/x64/Release/netcoreapp6.0/*.dll


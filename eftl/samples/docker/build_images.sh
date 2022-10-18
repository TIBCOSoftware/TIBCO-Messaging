#!/bin/bash

#
# Copyright (c) 2016-2022 TIBCO Software Inc.
# All Rights Reserved. Confidential & Proprietary.
# For more information, please contact:
# TIBCO Software Inc., Palo Alto, California, USA
#

#
# Variables to set to affect script behavior:
#
export BASE_IMAGE=redhat/ubi8

base="$(cd "${0%/*}" 2>/dev/null; echo "$PWD")"
usage="$cmd [options] -- echo arguments
REQUIRED
  --ftl  <ftl_pkg_name>   : FTL package that was downloaded from TIBCO's delivery site. (e.g. path to the linux FTL package e.g. /var/tmp/TIB_ftl_<version>_linux_x86_64.zip)
  --eftl <eftl_pkg_name>  : eFTL package that was downloaded from TIBCO's delivery site. (e.g. path to the linux FTL package e.g. /var/tmp/TIB_eftl_<version>_linux_x86_64.zip)
OPTIONAL
  --base <base_os_image>  : Base image to use for building docker images.
"

unzip_pkg() {
    ftlpkg=$1
    eftlpkg=$2

    # uzip the package
    cid=$(docker create --privileged --entrypoint=sh alpine -x /tmp/unzip_pkg.sh)

    docker cp ${ftlpkg} $cid:/tmp/pkg-ftl.zip
    docker cp ${eftlpkg} $cid:/tmp/pkg-eftl.zip
    docker cp /tmp/unzip_pkg.sh $cid:/tmp/unzip_pkg.sh
    docker cp /tmp/get_ftl_version.sh $cid:/tmp/get_ftl_version.sh
    docker cp /tmp/get_ftl_version_short.sh $cid:/tmp/get_ftl_version_short.sh
    docker cp /tmp/get_eftl_version.sh $cid:/tmp/get_eftl_version.sh
    docker cp /tmp/get_eftl_version_short.sh $cid:/tmp/get_eftl_version_short.sh

    docker start $cid
    result=$(docker wait $cid)
    if [ $result != 0 ]; then
        echo "Failed to unzip ${ftlpkg} and ${eftlpkg}"
        docker rm $cid
        exit 1
    fi
    docker commit $cid ftleftlpkg
    docker rm $cid
}

#
# Get Options
#
[ $# -eq 0 ] && echo "$usage" && exit 1
while [ $# -gt 0 ] ; do arg="$1"
  case "$arg" in
    --help|-help)     echo "$usage" ; exit 1 ;;
    --ftl )           ftlpkg="$2" ; shift ;;
    --eftl )          eftlpkg="$2" ; shift ;;
    --base )          BASE_IMAGE="$2" ; shift ;;
    '--' )            shift ; break ;;
    -debug|--debug )  set -x ;;
    -* )              echo "Unrecognized option: $arg" ; echo "$usage" ; exit 1 ;;
    * )               posargs+=("$arg") ;;
  esac ; shift
done

if [[ ! -f "${ftlpkg}" ]]; then
  echo "FTL package does not exit"
  exit 1
fi

if [[ ! -f "${eftlpkg}" ]]; then
  echo "eFTL package does not exit"
  exit 1
fi

# scripts that run with docker
cat > /tmp/unzip_pkg.sh << END
    set -e
    unzip -d /tmp/ftl /tmp/pkg-ftl.zip
    unzip -d /tmp/eftl /tmp/pkg-eftl.zip
END

cat > /tmp/get_ftl_version.sh << END
    set -e
    (cd /tmp/ftl/TIB_ftl_*; echo tar/TIB_ftl_*_linux_x86_64-runtime.tar.gz | grep -o -E '\d+\.\d+\.\d+')
END

cat > /tmp/get_ftl_version_short.sh << END
    set -e
    (cd /tmp/ftl/TIB_ftl_*; echo tar/TIB_ftl_*_linux_x86_64-runtime.tar.gz | grep -o -E '\d+\.\d+')
END

cat > /tmp/get_eftl_version.sh << END
    set -e
    (cd /tmp/eftl/TIB_eftl_*; echo tar/TIB_eftl_*_linux_x86_64-runtime.tar.gz | grep -o -E '\d+\.\d+\.\d+')
END

cat > /tmp/get_eftl_version_short.sh << END
    set -e
    (cd /tmp/eftl/TIB_eftl_*; echo tar/TIB_eftl_*_linux_x86_64-runtime.tar.gz | grep -o -E '\d+\.\d+')
END

# unzip the packages
unzip_pkg ${ftlpkg} ${eftlpkg}

# get version
FTL_PKG_VERSION=$(docker run --rm ftleftlpkg -x /tmp/get_ftl_version.sh)
FTL_PKG_VERSION_SHORT=$(docker run --rm ftleftlpkg -x /tmp/get_ftl_version_short.sh)
export FTL_PKG_VERSION
export FTL_PKG_VERSION_SHORT

EFTL_PKG_VERSION=$(docker run --rm ftleftlpkg -x /tmp/get_eftl_version.sh)
EFTL_PKG_VERSION_SHORT=$(docker run --rm ftleftlpkg -x /tmp/get_eftl_version_short.sh)
export EFTL_PKG_VERSION
export EFTL_PKG_VERSION_SHORT

# build tibftlserver image
docker build -t eftl-tibftlserver:${FTL_PKG_VERSION} --build-arg BASE_IMAGE --build-arg FTL_PKG_VERSION --build-arg FTL_PKG_VERSION_SHORT --build-arg EFTL_PKG_VERSION --build-arg EFTL_PKG_VERSION_SHORT --target eftl-tibftlserver "$@" - < "${base}/Dockerfile.eftl"

# remove docker images
docker rmi ftleftlpkg

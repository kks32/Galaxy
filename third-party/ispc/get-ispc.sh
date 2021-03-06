#!/bin/bash
## ========================================================================== ##
## Copyright (c) 2014-2020 The University of Texas at Austin.                 ##
## All rights reserved.                                                       ##
##                                                                            ##
## Licensed under the Apache License, Version 2.0 (the "License");            ##
## you may not use this file except in compliance with the License.           ##
## A copy of the License is included with this software in the file LICENSE.  ##
## If your copy does not contain the License, you may obtain a copy of the    ##
## License at:                                                                ##
##                                                                            ##
##     https://www.apache.org/licenses/LICENSE-2.0                            ##
##                                                                            ##
## Unless required by applicable law or agreed to in writing, software        ##
## distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  ##
## WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           ##
## See the License for the specific language governing permissions and        ##
## limitations under the License.                                             ##
##                                                                            ##
## ========================================================================== ##

VERSION="1.12.0"

if [ "x$1" != "x" ]; then
	echo "usage: get-ispc.sh"
	echo "get-ispc will auto-detect the OS and download the appropriate ispc version ${VERSION}"
	exit 1
fi

function fail
{
	echo "GALAXY: ERROR - $1"
	echo "GALAXY: Try downloading ispc manually at https://ispc.github.io/downloads.html"
	exit 1
}

function report
{
	echo "GALAXY: $1"
}

OS_TYPE=$(uname)
if [ "${OS_TYPE}" == "Linux" ]; then
	TARGET_OS="linux"
	TARGET_OS_DIR="linux"
elif [ "${OS_TYPE}" == "Darwin" ]; then
	TARGET_OS="macOS"
	TARGET_OS_DIR="macOS"
else
	fail "Unrecognized OS type '${OS_TYPE}'"
fi

TARGET_DIR="install/ispc-v${VERSION}-${TARGET_OS_DIR}"
TARBALL="ispc-v${VERSION}-${TARGET_OS}.tar.gz"
BIN_TARGET_DIR="../install"

# ispc v1.12 linux download has a 'b' appended
if [ "x${OS_TYPE}" == "xLinux" ]; then
	TARBALL="ispc-v${VERSION}b-${TARGET_OS}.tar.gz"  
fi

if [ -x ${TARGET_DIR}/bin/ispc ] && [ -x ${BIN_TARGET_DIR}/bin/ispc ]; then
	report "ispc for ${OS_TYPE} already exists. Nothing more to do."
	exit 0
fi

if [ -f ${TARBALL} ]; then
	report "found ${TARBALL}"
else 
	report "downloading ispc ${VERSION} for ${OS_TYPE}"
	wget -q -O ${TARBALL} https://github.com/ispc/ispc/releases/download/v${VERSION}/${TARBALL}
	if [ $? != 0 ]; then
		fail "Download for ${TARBALL} failed."
	fi
fi

if [ -f ${TARBALL} ]; then
	report "untarring ${TARBALL}"
	mkdir -p install
	pushd install
	tar xf ../${TARBALL}
	popd
else
	fail "Could not find ${TARBALL}"
fi

if [ -x ${TARGET_DIR}/bin/ispc ]; then
	report "ispc for ${OS_TYPE} successfully retrieved!"
	mkdir -p ${BIN_TARGET_DIR}
	cp -R ${TARGET_DIR}/bin ${BIN_TARGET_DIR}
	rm ${TARBALL}
else
	fail "Executable ${TARGET_DIR}/bin/ispc not found"
fi

exit 0

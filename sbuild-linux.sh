#!/bin/sh

# Usage: sh sbuild-linux.sh [source directory] [build type] [config]
#
# The config names a file in etc/Config; "default" builds everything this
# application uses. Personal settings go in etc/Config/local.cmake, which is
# not tracked. For the number of build jobs, export CMAKE_BUILD_PARALLEL_LEVEL.
#
# The build and install directories are made in the current directory, so
# running this from inside the checkout keeps them there, and running it from a
# directory above puts them beside the checkout.

sh ${1:-.}/etc/Linux/sbuild.sh ${1:-.} ${2:-Release} ${3:-default}

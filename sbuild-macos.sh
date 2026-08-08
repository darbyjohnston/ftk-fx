#!/bin/sh

# Usage: sh sbuild-macos.sh [source directory] [build type] [config]
#
# The config names a file in etc/Config; "default" builds everything this
# application uses. Personal settings go in etc/Config/local.cmake, which is
# not tracked. For the number of build jobs, export CMAKE_BUILD_PARALLEL_LEVEL.
#
# Run this from the directory holding the checkout, not from inside it: the
# build and install directories are made in the current directory, and the
# point of the layout is to keep them out of the source tree.
#
#     cd ~/Dev/ftk-fx && sh ftk-fx/sbuild-macos.sh ftk-fx Debug

sh ${1:-ftk-fx}/etc/macOS/sbuild.sh ${1:-ftk-fx} ${2:-Release} ${3:-default}

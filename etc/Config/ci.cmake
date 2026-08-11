# What continuous integration builds everywhere: the default configuration,
# plus everything the dependencies ship.
#
# The application does not need feather-tk's examples or tlRender's player, and
# a developer build leaves them off because they are build time for nothing.
# Continuous integration is the opposite case: this project exists partly to
# exercise those two libraries, changes to them are made here, and code that
# nothing compiles is code that quietly stops compiling. Three of the bugs
# found in one week were in files this configuration did not build.
#
# Set before default.cmake is included, because a plain cache set does not
# overwrite a value already there and the first one wins.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(ftk_EXAMPLES ON CACHE BOOL "")
set(ftk_TESTS ON CACHE BOOL "")
set(TLRENDER_EXAMPLES ON CACHE BOOL "")
set(TLRENDER_TESTS ON CACHE BOOL "")
set(TLRENDER_PROGRAMS ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/default.cmake")

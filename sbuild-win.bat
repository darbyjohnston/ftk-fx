rem Usage: sbuild-win.bat [source directory] [build type] [config]
rem
rem The config names a file in etc/Config; "default" builds everything this
rem application uses. Personal settings go in etc/Config/local.cmake, which is
rem not tracked. For the number of build jobs, set CMAKE_BUILD_PARALLEL_LEVEL.
rem
rem The build and install directories are made in the current directory, so
rem running this from inside the checkout keeps them there, and running it from
rem a directory above puts them beside the checkout.

set SOURCE_DIR=%1
set BUILD_TYPE=%2
set CONFIG=%3
IF "%SOURCE_DIR%"=="" set SOURCE_DIR=.
IF "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
IF "%CONFIG%"=="" set CONFIG=default

%SOURCE_DIR%\etc\Windows\sbuild.bat %SOURCE_DIR% %BUILD_TYPE% %CONFIG%

rem Usage: sbuild-win.bat [source directory] [build type] [config]
rem
rem The config names a file in etc/Config; "default" builds everything this
rem application uses. Personal settings go in etc/Config/local.cmake, which is
rem not tracked. For the number of build jobs, set CMAKE_BUILD_PARALLEL_LEVEL.
rem
rem Run this from the directory holding the checkout, not from inside it: the
rem build and install directories are made in the current directory, and the
rem point of the layout is to keep them out of the source tree.
rem
rem     cd %USERPROFILE%\Dev\ftk-fx && ftk-fx\sbuild-win.bat ftk-fx Debug

set SOURCE_DIR=%1
set BUILD_TYPE=%2
set CONFIG=%3
IF "%SOURCE_DIR%"=="" set SOURCE_DIR=ftk-fx
IF "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
IF "%CONFIG%"=="" set CONFIG=default

%SOURCE_DIR%\etc\Windows\sbuild.bat %SOURCE_DIR% %BUILD_TYPE% %CONFIG%

@echo off
REM i run this from suugu\

ctime -begin misc\sandbox.ctm

pushd src

REM _____________________________________________________________________________________________________
REM                                       Includes/Sources/Libs
REM _____________________________________________________________________________________________________

@set INCLUDES=/I"..\src" /I"..\deshi\src" /I"..\deshi\src\external" /I"C:\src\glfw-3.3.2.bin.WIN64\include" /I"%VULKAN_SDK%\include"
@set SOURCES=..\deshi\src\deshi.cpp main.cpp
@set LIBS=/libpath:C:\src\glfw-3.3.2.bin.WIN64\lib-vc2019 /libpath:%VULKAN_SDK%\lib glfw3.lib opengl32.lib gdi32.lib shell32.lib vulkan-1.lib shaderc_combined.lib

REM _____________________________________________________________________________________________________
REM                                      Compiler and Linker Flags
REM _____________________________________________________________________________________________________

REM NOTE(delle): /MD is used because vulkan's shader compilation lib requires dynamic linking with the CRT
@set WARNINGS=/wd4201 /wd4100 /wd4189 /wd4706 /wd4311
@set COMPILE_FLAGS=/diagnostics:column /EHsc /nologo /MD /MP /Oi /GR /Gm- /Fm /std:c++17 /utf-8 %WARNINGS%
@set LINK_FLAGS=/nologo /opt:ref /incremental:no
@set OUT_EXE=sandbox.exe

REM _____________________________________________________________________________________________________
REM                                            Platform
REM _____________________________________________________________________________________________________

REM  BUILD_SLOW:      slow code allowed (Assert, etc)
REM  BUILD_INTERNAL:  build for developer only (Renderer debug, etc)
REM  BUILD_RELEASE:   build for final release
REM  DESHI_WINDOWS:   build for 64-bit windows
REM  DESHI_MAC:       build for Mac OS X
REM  DESHI_LINUX:     build for Linux
REM  DESHI_VULKAN:    build for Vulkan
REM  DESHI_OPENGL:    build for OpenGL
REM  DESHI_OPENGL:    build for OpenGL
REM  DESHI_DIRECTX12: build for DirectX12

@set FLAGS_DEBUG=/D"BUILD_INTERNAL=1" /D"BUILD_SLOW=1" /D"BUILD_RELEASE=0"
@set FLAGS_RELEASE=/D"BUILD_INTERNAL=0" /D"BUILD_SLOW=0" /D"BUILD_RELEASE=1"
@set FLAGS_OS=/D"DESHI_WINDOWS=1" /D"DESHI_MAC=0" /D"DESHI_LINUX=0"
@set FLAGS_RENDERER=/D"DESHI_VULKAN=0" /D"DESHI_OPENGL=1" /D"DESHI_DIRECTX12=0"

REM _____________________________________________________________________________________________________
REM                                    Command Line Arguments
REM _____________________________________________________________________________________________________

IF [%1]==[] GOTO DEBUG
IF [%1]==[-i] GOTO ONE_FILE
IF [%1]==[-l] GOTO LINK_ONLY
IF [%1]==[-r] GOTO RELEASE

REM _____________________________________________________________________________________________________
REM                              DEBUG (compiles without optimization)
REM _____________________________________________________________________________________________________

:DEBUG
ECHO %DATE% %TIME%    Debug
ECHO ---------------------------------
@set OUT_DIR="..\build\debug"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /Z7 /Od /W1 %COMPILE_FLAGS% %FLAGS_DEBUG% %FLAGS_OS% %FLAGS_RENDERER% %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LINK_FLAGS% %LIBS%
GOTO DONE

REM _____________________________________________________________________________________________________
REM    ONE FILE (compiles just one file with debug options, links with previously created .obj files)
REM _____________________________________________________________________________________________________

:ONE_FILE
ECHO %DATE% %TIME%    One File (Debug)
ECHO ---------------------------------
IF [%~2]==[] ECHO "Place the .cpp path after using -i"; GOTO DONE;
ECHO [93mWarning: debugging might not work with one-file compilation[0m

@set OUT_DIR="..\build\debug"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /c /Z7 /W1 %COMPILE_FLAGS% %FLAGS_DEBUG% %FLAGS_OS% %FLAGS_RENDERER% %INCLUDES% %~2 /Fo%OUT_DIR%/
pushd ..\build\Debug
link %LINK_FLAGS% *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM _____________________________________________________________________________________________________
REM                           LINK ONLY (links with previously created .obj files)
REM _____________________________________________________________________________________________________

:LINK_ONLY
ECHO %DATE% %TIME%    Link Only (Debug)
ECHO ---------------------------------
pushd ..\build\Debug
link %LINK_FLAGS% *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM _____________________________________________________________________________________________________
REM                                 RELEASE (compiles with optimization)
REM _____________________________________________________________________________________________________

:RELEASE
ECHO %DATE% %TIME%    Release
@set OUT_DIR="..\build\release"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /O2 /W4 %COMPILE_FLAGS% %FLAGS_RELEASE% %FLAGS_OS% %FLAGS_RENDERER% %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LINK_FLAGS% %LIBS%
GOTO DONE

:DONE
ECHO ---------------------------------
popd

ctime -end misc\sandbox.ctm
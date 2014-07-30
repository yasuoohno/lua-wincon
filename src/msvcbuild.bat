REM @ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM
REM remove unnecessary files.
REM
DEL *.obj >NUL 2>NUL
DEL *.exp >NUL 2>NUL
DEL *.lib >NUL 2>NUL
DEL *.dll >NUL 2>NUL

REM
REM Detect Architecture.
REM

IF "%LUAMOD64%" == "" GOTO :SETENV_ERROR
IF "%LUAMOD32%" == "" GOTO :SETENV_ERROR
GOTO :DETECT_ARCH
:SETENV_ERROR
ECHO "Please set Lua module directory path to LUAMOD32 and LUAMOD64."
ECHO "It contains include and lib directory to compile/link lua library."
GOTO :EOF

:DETECT_ARCH
IF "%LIB:~-4%" == "x86;" GOTO :x86

:x64
set LUAMODBIN=%LUAMOD64%
set LUALIB=%LUAMODBIN%\lib\lua51_x64.lib
goto :DETECT_ARCH_END

:x86
set LUAMODBIN=%LUAMOD32%
set LUALIB=%LUAMODBIN%\lib\lua51.lib
goto :DETECT_ARCH_END

:DETECT_ARCH_END
set LUAINC=%LUAMODBIN%\include

REM
REM LUAMODBIN -> the folder that the compiled dll module file will be copied.
REM LUAINC -> the include folder for Lua.
REM LUALIB -> the library file for Lua.
REM

REM
REM CL and LINK options.
REM
SET CL_OPT=/nologo /EHsc /GL /Gm- /Gy /O2 /Oi /Oy- /W3 /D_CRT_SECURE_NO_DEPRECATE /MD /I%LUAINC%
SET CL_DEF=/D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL"
SET LINK_OPT=/nologo /DLL /INCREMENTAL:NO /LTCG /NXCOMPAT /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF
SET LINK_LIB=user32.lib gdi32.lib advapi32.lib ole32.lib oleaut32.lib htmlhelp.lib shlwapi.lib %LUALIB%

REM
REM compile source to object.
REM
CL %CL_OPT% %CL_DEF% /c lua_wincon.c

REM
REM link object to dll.
REM
LINK %LINK_OPT%  /out:wincon.dll lua_wincon.obj %LINK_LIB%

REM
REM remove unnecessary files.
REM
DEL *.obj >NUL 2>NUL
DEL *.exp >NUL 2>NUL
DEL *.lib >NUL 2>NUL

REM
REM copy *.dll to LUAMODBIN
REM
COPY wincon.dll %LUAMODBIN%

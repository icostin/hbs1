@echo off

:: hbs1 - Host Basic Services - ver. 1
::
:: Build script for 32-bit Windows platforms using Visual Studio 9
::
:: Changelog:
::	* 2013/01/06 Costin Ionescu: initial release

set N=hbs1
set D=HBS1
set CSRC=src\mswin.c src\misc.c
call %VS90COMNTOOLS%\vsvars32.bat

if not exist out\win32-rls-sl mkdir out\win32-rls-sl
if not exist out\win32-dbg-sl mkdir out\win32-dbg-sl
if not exist out\win32-rls-dl mkdir out\win32-rls-dl
if not exist out\win32-dbg-dl mkdir out\win32-dbg-dl

echo Building dynamic release library...  
cl.exe /nologo /Ox /LD /Iinclude /I..\c41\include /D%D%_DL_BUILD /DNDEBUG /Foout\win32-rls-dl\ /Feout\win32-rls-dl\%N%.dll %CSRC% /link ..\c41\out\win32-rls-dl\c41.lib
echo Building static release library...
cl.exe /nologo /Ox /c  /Iinclude /I..\c41\include /D%D%_STATIC /DC41_STATIC /DNDEBUG /Foout\win32-rls-sl\ %CSRC% /link ..\c41\out\win32-rls-sl\c41.lib
set O=%CSRC:.c=.obj%
set O=%O:src=out\win32-rls-sl%
lib.exe /nologo /out:out\win32-rls-sl\%N%.lib %O%

echo Building dynamic debug library...  
cl.exe /nologo /Od /LDd /Iinclude /I..\c41\include /D%D%_DL_BUILD /D_DEBUG /Foout\win32-dbg-dl\ /Feout\win32-dbg-dl\%N%.dll %CSRC% /link ..\c41\out\win32-dbg-dl\c41.lib
echo Building static debug library...
cl.exe /nologo /Od /c  /Iinclude /I..\c41\include /D%D%_STATIC /DC41_STATIC /D_DEBUG /Foout\win32-dbg-sl\ %CSRC% /link ..\c41\out\win32-dbg-sl\c41.lib
set O=%CSRC:.c=.obj%
set O=%O:src=out\win32-dbg-sl%
lib.exe /nologo /out:out\win32-dbg-sl\%N%.lib %O%

echo Building dynamic-deps cli library...  
cl.exe /nologo /Ox /c /Iinclude /I..\c41\include /DNDEBUG /Foout\win32-rls-dl\ src\cli.c /link ..\c41\out\win32-rls-dl\c41.lib out\win32-rls-dl\hbs1.lib
lib.exe /nologo /out:out\win32-rls-dl\hbs1clid.lib out\win32-rls-dl\cli.obj

echo Building static-deps cli library...
cl.exe /nologo /Ox /c /Iinclude /I..\c41\include /DNDEBUG /DC41_STATIC /DHBS1_STATIC /Foout\win32-rls-sl\ src\cli.c /link ..\c41\out\win32-rls-sl\c41.lib out\win32-rls-sl\hbs1.lib
lib.exe /nologo /out:out\win32-rls-sl\hbs1clis.lib out\win32-rls-sl\cli.obj

echo Building static-deps hbs1 test program...
cl.exe /nologo /Ox /MT /Feout\win32-rls-sl\test.exe src\test.c /Iinclude /I..\c41\include /DC41_STATIC /DHBS1_STATIC /link ..\c41\out\win32-rls-sl\c41.lib out\win32-rls-sl\hbs1.lib out\win32-rls-sl\hbs1clis.lib /subsystem:console
out\win32-rls-sl\test.exe

echo Building static-deps hbs1cli test program...
cl.exe /nologo /Ox /MT /Feout\win32-rls-sl\clitest.exe src\clitest.c /Iinclude /I..\c41\include /DC41_STATIC /DHBS1_STATIC /link ..\c41\out\win32-rls-sl\c41.lib out\win32-rls-sl\hbs1.lib out\win32-rls-sl\hbs1clis.lib /subsystem:console
out\win32-rls-sl\clitest.exe

echo Building dynamic debug hbs1 test program...
cl.exe /nologo /Od /Iinclude /I..\c41\include /D_DEBUG /Feout\win32-dbg-dl\test.exe /Foout\win32-dbg-dl\ src\test.c /link ..\c41\out\win32-dbg-dl\c41.lib out\win32-dbg-dl\%N%.lib
set PATH=%PATH%;..\c41\out\win32-dbg-dl
out\win32-dbg-dl\test.exe

echo Building dynamic debug hbs1cli test program...
cl.exe /nologo /Od /Iinclude /I..\c41\include /D_DEBUG /Feout\win32-dbg-dl\clitest.exe /Foout\win32-dbg-dl\ src\clitest.c /link ..\c41\out\win32-dbg-dl\c41.lib out\win32-dbg-dl\%N%.lib out\win32-rls-dl\hbs1clid.lib /subsystem:console
set PATH=%PATH%;..\c41\out\win32-dbg-dl
out\win32-dbg-dl\clitest.exe

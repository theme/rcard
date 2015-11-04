SET PATH=C:\msys64\mingw64\qt5-static\bin;C:\msys64\mingw64\bin;%PATH%
SET LIBRARY_PATH=C:\msys64\mingw64\qt5-static\lib\

SET QMAKESPEC=win32-g++
SET BUILDDIR=..\build-RcardClient-static

MD %BUILDDIR%
cd %BUILDDIR%

qmake "CONFIG+=release static win32" ..\RcardClient\RcardClient.pro

mingw32-make clean
mingw32-make

pause

SET PATH=C:\msys64\usr\bin;C:\msys64\mingw64\bin;C:\msys64\mingw64\qt5-static\bin;%PATH%
SET QMAKESPEC=win32-g++
SET BUILDDIR=..\build-RcardClient-static

MD %BUILDDIR%
cd %BUILDDIR%

qmake "CONFIG+=release static win32" ..\RcardClient\RcardClient.pro

mingw32-make

pause

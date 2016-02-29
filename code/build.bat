@echo off

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1  -DHANDMADE_WIN32=1 -FC -Zi -FAsc -Fmwin32_handmade.map
set CommonLinkerFlags=  -opt:ref user32.lib gdi32.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp  /link -subsystem:windows,5.1 %CommonLinkerFlags% 


REM 64-bit build
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link %CommonLinkerFlags% 
popd

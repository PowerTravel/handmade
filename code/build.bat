@echo off

REM  -wd4505 turns off warnings that a function is not referenced
REM  -wd4244 turns off warning about truncation loss of data when converting from real to int
..\ctime\ctime -begin handmade_hero.ctm

REM /Od removes compiler optimization

REM set CommonCompilerFlags=-nologo -fp:fast             -Gm- -GR- -EHa-     -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -wd4706 -FC                     -Zi -FAsc 
set CommonCompilerFlags=-Od -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -wd4706 -FC -Z7 -GS- -Gs9999999 /EHsc
set CommonCompilerFlags=-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 %CommonCompilerFlags%

set CommonLinkerFlags= -incremental:no user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp  /link -subsystem:windows,5.1 %CommonLinkerFlags% 

set BuildFiles=..\handmade\code\handmade.cpp


REM 64-bit build
REM Optimization switches -O2
del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% -MTd %BuildFiles% -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples
set LastError=%ERRORLEVEL%
del lock.tmp
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags% 
cl %CommonCompilerFlags% ..\handmade\tests\memory.cpp /link %CommonLinkerFlags% 
popd

..\ctime\ctime -end handmade_hero.ctm %LastError%
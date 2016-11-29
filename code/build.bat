@echo off

REM -wd4505 turns off warnings that a function is not referenced
REM  -wd4244 turns off warning about truncation loss of data when converting from real to int

set CommonCompilerFlags=-MTd -nologo -fp:fast -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1  -DHANDMADE_WIN32=1 -FC -Zi -FAsc 
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp  /link -subsystem:windows,5.1 %CommonLinkerFlags% 


REM 64-bit build
REM Optimization switches -O2
del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% ..\handmade\code\handmade.cpp -Fmhandmade.map -LD /link -incremental:no -PDB:handmade_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples
del lock.tmp
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags% 
popd

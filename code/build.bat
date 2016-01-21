@echo off

mkdir ..\..\build
pushd ..\..\build
cl -Zi -FAsc ..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd

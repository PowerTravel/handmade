@echo off

REM This is for windows 7:

REM @subst w: "\Users\Mother Shabobo\Documents"
REM call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
REM set path=w:\handmade\misc;%path%

REM This is for windows 10:
@subst w: "\Users\jh\Documents\dev"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
call set path=w:\Users\jh\Documents\dev\handmade\misc;%path%

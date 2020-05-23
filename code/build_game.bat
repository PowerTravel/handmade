@echo off
REM This script first sets the environment to compile w vc - vcvarsall x64
REM then compiles. Used when compiling from the game
call "w:/handmade/misc/shell.bat"
call "w:/handmade/code/build.bat"
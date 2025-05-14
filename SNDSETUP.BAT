@echo off
call grvdet.exe
if errorlevel 1 goto LoadStuff
goto Exit

:LoadStuff
rem Found a Gravis Ultrasound
call loadpats

:Exit
sndset2

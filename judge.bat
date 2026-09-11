@echo off
title Conquer the Spire - judging

rem Everything runs from the folder this file sits in.
cd /d "%~dp0"

rem The python with torch and numpy in it, by name - the same one train.bat
rem uses. "python" on its own can be another install without them.
set "PYTHON=%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
if not exist "%PYTHON%" set "PYTHON=python"

set "JUDGE=Python\cts_judge.py"
set "LAST=runs\last.bat"

rem The climber being trained, from what train.bat last ran.
set "CHARACTER=ironclad"
if exist "%LAST%" call "%LAST%"

if exist "runs\%CHARACTER%\checkpoint.pt" goto judge
echo   Nothing to judge yet: runs\%CHARACTER%\checkpoint.pt is not there.
echo   Start train.bat first, then open this again.
echo.
pause
exit /b 1

:judge
echo   Judging runs\%CHARACTER% beside the training.
echo.
echo   Every hour this copies the latest checkpoint and plays it on fixed
echo   seeds, flat and looking at two moves everywhere: 400 climbs to pick
echo   the best of each kind, and 400 more it never picks on, to report.
echo   The winners are runs\%CHARACTER%\best-flat.pt and best-look2.pt;
echo   every reading is a row of runs\%CHARACTER%\judged.csv. cts_play
echo   reads the winners.
echo.
echo   Close this window to stop it. The training is not touched.
echo.

"%PYTHON%" "%JUDGE%" "runs\%CHARACTER%" %*

echo.
echo   The judge stopped.
pause

@echo off
title Conquer the Spire - lambda 0.98 against 0.95

rem Everything runs from the folder this file sits in.
cd /d "%~dp0"

rem The python with torch and numpy in it, by name - the same one train.bat
rem uses. "python" on its own can be another install without them.
set "PYTHON=%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
if not exist "%PYTHON%" set "PYTHON=python"

rem One experiment, kept apart from the main run: the same climber, the
rem same settings, one number changed. Both start from the same weights -
rem the main run's checkpoint from the moment the aim had stopped paying,
rem kept as aim-plateau.pt - and each gets a day. The judge scores both
rem on the same seeds, so the two judged.csv files read side by side.
set "OUT=runs-lam098"
set "START=runs\ironclad\aim-plateau.pt"
set "LAM=0.98"

rem Everything else exactly as the main run has it.
set "CHARACTER=ironclad"
set "ACTS=3"
set "ENVS=128"
set "WIDTH=1024"
set "GAMMA=0.999"
set "HPW=0.01"
set "DEEP=0.4"
set "LOOK=0"
if exist "runs\last.bat" call "runs\last.bat"
set "LAM=0.98"

set "FOLDER=%OUT%\%CHARACTER%"

cls
echo ==========================================================
echo   lambda 0.98 against 0.95 - %CHARACTER%
echo ==========================================================
echo.
echo   Both runs start from %START%
echo   and differ in lambda alone. This one trains into %FOLDER%\
echo   with lambda %LAM%; the main run in runs\%CHARACTER%\ is the 0.95 side.
echo.
echo     1. Train the 0.98 side   - carries on if it has started already
echo     2. Judge the 0.98 side   - a second window, beside the training
echo.
echo     Q. Quit
echo.
set "PICK="
set /p "PICK=  Choose [1]: "

if not defined PICK set "PICK=1"
if /i "%PICK%"=="q" exit /b 0
if "%PICK%"=="1" goto train
if "%PICK%"=="2" goto judge
echo.
echo   That was not one of them.
pause
exit /b 1

:train
if exist "%FOLDER%\checkpoint.pt" goto haveStart
if exist "%START%" goto copyStart
echo   %START% is not there. It is the main run's checkpoint from when the
echo   aim stopped paying; without it the two sides would not start level.
echo.
pause
exit /b 1

:copyStart
md "%FOLDER%" 2>nul
copy /y "%START%" "%FOLDER%\checkpoint.pt" >nul
echo   Started %FOLDER%\checkpoint.pt from %START%.
echo.

:haveStart
echo   Training %CHARACTER% with lambda %LAM% into %FOLDER%\
echo   act limit %ACTS%, %ENVS% climbs, %WIDTH% wide, gamma %GAMMA%,
echo   a point of health %HPW%, deep %DEEP%, look %LOOK%.
echo.
echo   Ctrl-C stops it; it saves first.
echo.
"%PYTHON%" Python\cts_train.py --out "%OUT%" --character %CHARACTER% --acts %ACTS% --envs %ENVS% --width %WIDTH% --gamma %GAMMA% --hp-weight %HPW% --deep %DEEP% --look %LOOK% --lam %LAM% --picks %*
echo.
echo   Stopped. Everything is in %FOLDER%\
pause
exit /b 0

:judge
if exist "%FOLDER%\checkpoint.pt" goto haveRun
echo   Nothing to judge yet: %FOLDER%\checkpoint.pt is not there.
echo   Choose 1 first, in another window.
echo.
pause
exit /b 1

:haveRun
echo   Judging %FOLDER% on the judge's usual seeds, so that its judged.csv
echo   reads against runs\%CHARACTER%\judged.csv line for line.
echo.
"%PYTHON%" Python\cts_judge.py "%FOLDER%" %*
echo.
echo   The judge stopped.
pause

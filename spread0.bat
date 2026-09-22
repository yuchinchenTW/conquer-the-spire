@echo off
title Conquer the Spire - entropy pressure held at its floor

rem Everything runs from the folder this file sits in.
cd /d "%~dp0"

rem The python with torch and numpy in it, by name - the same one train.bat
rem uses. "python" on its own can be another install without them.
set "PYTHON=%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
if not exist "%PYTHON%" set "PYTHON=python"

rem One experiment, kept apart from the main run. The entropy pressure is a
rem floor that leans up by 2%% a report whenever the policy's spread is under
rem --spread, and on these weights it has leaned to 0.032 in fights and
rem 0.066 out of them against the 0.01 asked for. --spread 0 stops the
rem leaning: nothing is ever under nothing, so every pressure leans back
rem down instead, 2%% a report, and settles on its floor after about three
rem hundred updates. So the first twenty minutes of this run are still
rem carrying most of the old coefficient - it is not a switch.
rem
rem It also makes the rate-decay gate read true, since every pushed region
rem is trivially at or over nothing. That changes nothing here: the rate is
rem already on its 5e-5 floor and cannot go lower.
set "OUT=runs-spread0"
set "START=runs\ironclad\well.pt"
set "SPREAD=0"

rem Everything else exactly as the runs that fell had it.
set "CHARACTER=ironclad"
set "ACTS=3"
set "ENVS=128"
set "WIDTH=1024"
set "GAMMA=0.999"
set "HPW=0.01"
set "DEEP=0.4"
set "LOOK=0"
set "LAM=0.95"
if exist "runs\last.bat" call "runs\last.bat"

rem The shelf as the two ten-thousand-update falls had it, whatever the
rem last run in this folder was doing - one thing at a time.
set "DEEP=0.4"
set "LAM=0.95"
set "SPREAD=0"

set "FOLDER=%OUT%\%CHARACTER%"

cls
echo ==========================================================
echo   entropy pressure held down - %CHARACTER%
echo ==========================================================
echo.
echo   Three runs have now peaked at 22.5%% to 22.8%% won and fallen,
echo   losing only the third act's boss. This one starts from the same
echo   healthy weights as the last two - %START% -
echo   and changes one thing: --spread %SPREAD%, so the entropy pressure
echo   stops leaning up and settles back on its 0.01 floor.
echo.
echo   How to read it: if the wins merely stop falling, the pressure was
echo   costing stability. If they go past 22.8%%, it was holding the
echo   climber down as well. Anything under +15000 updates is not an
echo   answer - the three falls began at +6000, +8000 and +18000.
echo.
echo     1. Train it      - carries on if it has started already
echo     2. Judge it      - a second window, beside the training
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
echo   %START% is not there. It is the healthy checkpoint the guard kept
echo   when the last run fell; without it this does not start level with
echo   the runs it is being compared against.
echo.
pause
exit /b 1

:copyStart
md "%FOLDER%" 2>nul
copy /y "%START%" "%FOLDER%\checkpoint.pt" >nul
echo   Started %FOLDER%\checkpoint.pt from %START%.
echo.

:haveStart
echo   Training %CHARACTER% into %FOLDER%\ with --spread %SPREAD%
echo   act limit %ACTS%, %ENVS% climbs, %WIDTH% wide, gamma %GAMMA%,
echo   a point of health %HPW%, deep %DEEP%, look %LOOK%, lambda %LAM%.
echo.
echo   The guard is on, so if this falls the way the others did it stops
echo   itself and keeps %FOLDER%\well.pt.
echo.
"%PYTHON%" Python\cts_train.py --out "%OUT%" --character %CHARACTER% --acts %ACTS% --envs %ENVS% --width %WIDTH% --gamma %GAMMA% --hp-weight %HPW% --deep %DEEP% --look %LOOK% --lam %LAM% --spread %SPREAD% --picks %*
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

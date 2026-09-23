@echo off
title Conquer the Spire - the last fight, a fifth of the batch

rem Everything runs from the folder this file sits in.
cd /d "%~dp0"

rem The python with torch and numpy in it, by name - the same one train.bat
rem uses. "python" on its own can be another install without them.
set "PYTHON=%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
if not exist "%PYTHON%" set "PYTHON=python"

rem Four runs have peaked at 22.5% won and lost the third act's boss and
rem nothing else. Counted by where its steps sit, that fight is 4.7% of a
rem batch - an act 1 fight is 23% - which is too little to hold a skill
rem against the rest of the spire rewriting the same weights.
rem
rem So: a second shelf holding the room in front of that boss, and most of
rem the deep starts taken from it. Two knobs make one treatment, because a
rem boss climb is short - fifty steps against five hundred - so the share
rem of climbs started there is much larger than the share of the batch it
rem turns into. Measured, not assumed:
rem
rem     shelf 0.4, boss 0     4.73%   (what the four runs had)
rem     shelf 0.4, boss 1.0   7.78%
rem     shelf 0.6, boss 0.8  12.00%
rem     shelf 0.7, boss 1.0  18.26%   (this)
rem
rem and the first two acts lose about two points each, not their place.
set "OUT=runs-bossshelf"
set "START=runs\ironclad\well.pt"
set "DEEP=0.7"
set "DEEPBOSS=1.0"
set "SPREAD=0"

rem Everything else as the run it is being compared against had it - the
rem --spread 0 run, which fell at +11500.
set "CHARACTER=ironclad"
set "ACTS=3"
set "ENVS=128"
set "WIDTH=1024"
set "GAMMA=0.999"
set "HPW=0.01"
set "LOOK=0"
set "LAM=0.95"
if exist "runs\last.bat" call "runs\last.bat"

set "DEEP=0.7"
set "DEEPBOSS=1.0"
set "SPREAD=0"
set "LAM=0.95"
set "LOOK=0"

set "FOLDER=%OUT%\%CHARACTER%"

cls
echo ==========================================================
echo   the last fight, a fifth of the batch - %CHARACTER%
echo ==========================================================
echo.
echo   From the same healthy weights as the last two runs,
echo   %START%, with the third act's boss
echo   taken from 4.7%% of the batch to about 18%%.
echo.
echo   How to read it, and none of these on their own:
echo.
echo     1. Does it hold? The falls began at +6000, +8000, +11500
echo        and +18000, so +15000 without one is the first thing.
echo     2. Does the fight come back? Python\cts_boss.py, 40 saved
echo        rooms it never trains on - healthy weights win 60-65%%
echo        there and fallen ones 25-32%%. Ten minutes, any time.
echo     3. Do the first two acts hold? The judge's flat and look2
echo        readings are whole climbs, so they say if the rest of
echo        the spire was paid for this.
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
echo   Training %CHARACTER% into %FOLDER%\
echo   deep %DEEP%, of which %DEEPBOSS% are boss rooms, spread %SPREAD%,
echo   act limit %ACTS%, %ENVS% climbs, %WIDTH% wide, gamma %GAMMA%,
echo   a point of health %HPW%, look %LOOK%, lambda %LAM%.
echo.
echo   The guard is on, so if this falls the way the others did it stops
echo   itself and keeps %FOLDER%\well.pt.
echo.
"%PYTHON%" Python\cts_train.py --out "%OUT%" --character %CHARACTER% --acts %ACTS% --envs %ENVS% --width %WIDTH% --gamma %GAMMA% --hp-weight %HPW% --deep %DEEP% --deep-boss %DEEPBOSS% --look %LOOK% --lam %LAM% --spread %SPREAD% --picks %*
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
echo   reads against runs\%CHARACTER%\judged.csv line for line - whole
echo   climbs, so the first two acts are in the reading too.
echo.
"%PYTHON%" Python\cts_judge.py "%FOLDER%" %*
echo.
echo   The judge stopped.
pause

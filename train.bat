@echo off
title Conquer the Spire - training

rem Everything runs from the folder this file sits in.
cd /d "%~dp0"

set "PYTHON=python"
set "ENGINE=build\bin\libconquer-the-spire.dll"
set "TRAINER=Python\cts_train.py"
set "EXTRA=%*"

rem ------------------------------------------------------------- the engine
if exist "%ENGINE%" goto haveEngine
echo The engine is not built yet: %ENGINE% is missing.
echo Build it first with:  bash Scripts/build_windows_mingw.sh
echo.
pause
exit /b 1

:haveEngine
%PYTHON% --version >nul 2>&1
if not errorlevel 1 goto havePython
echo Python was not found on the path.
echo.
pause
exit /b 1

:havePython

rem --------------------------------------------- the same as last time
rem What was chosen last time, written down by this file when it starts
rem the trainer. There is nothing to carry on from the first time, so the
rem questions get asked; after that they do not have to be.
set "LAST=runs\last.bat"
if not exist "%LAST%" goto character

call "%LAST%"

rem Never carried over. It is the one answer that throws the weights
rem away, so it is asked for every single time it is wanted.
set "FRESH="

if not exist "runs\%CHARACTER%\checkpoint.pt" goto character

cls
echo ==========================================================
echo   Conquer the Spire - training
echo ==========================================================
echo.
echo   Last time it was:
echo.
echo     %CHARACTER%, act limit %ACTS% - 0 is the whole spire
echo     %ENVS% climbs at once, %WIDTH% wide
echo     looks ahead %GAMMA%, a point of health %HPW%
echo     practises the later acts: %DEEP% of the climbs start part-way up
echo.
echo   and it will carry on from runs\%CHARACTER%\checkpoint.pt
echo.
echo     1. Carry on just like that   - starts on its own in ten seconds
echo     2. Change something
echo.
choice /c 12 /t 10 /d 1 /n /m "  Choose [1]: "
if errorlevel 2 goto character
goto go

rem ---------------------------------------------------------- the character
:character
cls
echo ==========================================================
echo   Conquer the Spire - training
echo ==========================================================
echo.
echo   Which climber?
echo.
echo     1. Ironclad
echo     2. Silent
echo     3. Defect
echo.
echo     Q. Quit
echo.
set "CHARACTER="
set "PICK="
set /p "PICK=  Choose [1]: "

if not defined PICK set "PICK=1"
if /i "%PICK%"=="q" exit /b 0
if "%PICK%"=="1" set "CHARACTER=ironclad"
if "%PICK%"=="2" set "CHARACTER=silent"
if "%PICK%"=="3" set "CHARACTER=defect"
if defined CHARACTER goto acts
echo.
echo   That was not one of them.
pause
goto character

rem ---------------------------------------------------------- how far to go
:acts

rem What to offer, which the first time through is nothing to go on.
if not defined ACTSWAS set "ACTSWAS=1"

cls
echo ==========================================================
echo   %CHARACTER% - how much of the spire?
echo ==========================================================
echo.
echo     1. Act 1 only        - learns quickest, start here
echo     2. Acts 1 to 2
echo     3. Acts 1 to 3
echo     4. As far as it can go - the third act's boss, for now
echo.
echo     B. Back
echo.
set "PICK="
set /p "PICK=  Choose [%ACTSWAS%]: "

if not defined PICK set "PICK=%ACTSWAS%"
set "ACTS="
if /i "%PICK%"=="b" goto character
if "%PICK%"=="1" set "ACTS=1"
if "%PICK%"=="2" set "ACTS=2"
if "%PICK%"=="3" set "ACTS=3"
if "%PICK%"=="4" set "ACTS=0"
if defined ACTS goto carry
echo.
echo   That was not one of them.
pause
goto acts

rem -------------------------------------------------- pick up, or start over
:carry
set "SAVE=runs\%CHARACTER%\checkpoint.pt"
set "FRESH="
cls
echo ==========================================================
echo   %CHARACTER%, act limit %ACTS%
echo ==========================================================
echo.
if not exist "%SAVE%" goto nothingSaved

echo   There is a %CHARACTER% already trained in:
echo     %SAVE%
echo.
echo     1. Carry on from there
echo     2. Start over - the old weights are written over
echo.
echo     B. Back
echo.
set "PICK="
set /p "PICK=  Choose [1]: "

if not defined PICK set "PICK=1"
if /i "%PICK%"=="b" goto acts
if "%PICK%"=="1" goto width
if "%PICK%"=="2" goto startOver
echo.
echo   That was not one of them.
pause
goto carry

:startOver
set "FRESH=--fresh"
goto width

:nothingSaved
echo   Nothing has been trained for %CHARACTER% yet, so this starts fresh.
echo.

rem ------------------------------------------------------- how big a brain
:width
cls
echo ==========================================================
echo   %CHARACTER%, act limit %ACTS% - how big a brain?
echo ==========================================================
echo.
echo     1. Normal   - 512 wide, 2.7M weights
echo     2. Bigger   - 1024 wide, 5.8M weights, about 2%% slower
echo.
echo   A brain of one size cannot pick up a climber trained at another, so
echo   changing this starts over however the last question was answered.
echo.
echo     B. Back
echo.
set "WIDTH="
set "PICK="
set /p "PICK=  Choose [2]: "

if not defined PICK set "PICK=2"
if /i "%PICK%"=="b" goto carry
if "%PICK%"=="1" set "WIDTH=512"
if "%PICK%"=="2" set "WIDTH=1024"
if defined WIDTH goto sight
echo.
echo   That was not one of them.
pause
goto width

rem ----------------------------------------------------- how far it looks
:sight
cls
echo ==========================================================
echo   %CHARACTER%, act limit %ACTS% - how far ahead does it look?
echo ==========================================================
echo.
echo     1. Near  - about 200 moves ahead
echo     2. Far   - about 1000 moves ahead
echo.
echo   A climb is six to nine hundred moves, so only the far one can see
echo   what sharpening a card at a fire in act one buys in act two. Looking
echo   near, it rests instead - which is the right answer to the question it
echo   can see.
echo.
echo   This changes what a climb is worth, so the reward on the curve cannot
echo   be read against a run made with the other answer. Floors and boss can.
echo.
echo     B. Back
echo.
set "GAMMA="
set "PICK="
set /p "PICK=  Choose [2]: "

if not defined PICK set "PICK=2"
if /i "%PICK%"=="b" goto width
if "%PICK%"=="1" set "GAMMA=0.995"
if "%PICK%"=="2" set "GAMMA=0.999"
if defined GAMMA goto blood
echo.
echo   That was not one of them.
pause
goto sight

rem ------------------------------------------------------ what blood is worth
:blood
cls
echo ==========================================================
echo   %CHARACTER%, act limit %ACTS% - what is a point of health worth?
echo ==========================================================
echo.
echo     1. Dear   - 0.05 a point, so a whole health bar is four floors
echo     2. Cheap  - 0.01 a point, so a whole bar is under one floor
echo.
echo   Cheap makes the cards that pay in health worth paying for - it drafted
echo   Bloodletting none of 3185 times it was offered - but it also leaves
echo   almost no reason to avoid damage until the climb actually dies, which
echo   is a much thinner thing to learn from.
echo.
echo   Either way this changes what a climb is worth, so the reward on the
echo   curve cannot be read against a run made with the other answer.
echo.
echo     B. Back
echo.
set "HPW="
set "PICK="
set /p "PICK=  Choose [1]: "

if not defined PICK set "PICK=1"
if /i "%PICK%"=="b" goto sight
if "%PICK%"=="1" set "HPW=0.05"
if "%PICK%"=="2" set "HPW=0.01"
if defined HPW goto size
echo.
echo   That was not one of them.
pause
goto blood

rem ------------------------------------------------------------ how hard to
:size
cls
echo ==========================================================
echo   %CHARACTER%, act limit %ACTS% - how hard to push the machine?
echo ==========================================================
echo.
echo     1. Light    -  64 climbs at once
echo     2. Normal   - 128 climbs at once
echo     3. Heavy    - 256 climbs at once, fastest if the GPU keeps up
echo.
echo     B. Back
echo.
set "ENVS="
set "PICK="
set /p "PICK=  Choose [2]: "

if not defined PICK set "PICK=2"
if /i "%PICK%"=="b" goto blood
if "%PICK%"=="1" set "ENVS=64"
if "%PICK%"=="2" set "ENVS=128"
if "%PICK%"=="3" set "ENVS=256"
if defined ENVS goto deep
echo.
echo   That was not one of them.
pause
goto size

rem ------------------------------------------------- the acts it loses in
:deep
cls
echo ==========================================================
echo   %CHARACTER%, act limit %ACTS% - practise the later acts?
echo ==========================================================
echo.
echo     1. Yes  - 40%% of the climbs start part-way up
echo     2. No   - every climb starts on the first floor
echo.
echo   Every climb starts on the first floor, so the first act is where
echo   nearly all of the moves go - and the acts the climber is losing in
echo   are the ones it practises least. Of the climbs that beat the first
echo   act's boss, under half live to see the second act's; nine climbs in
echo   a hundred ever meet the last boss at all.
echo.
echo   Saying yes keeps a copy of a climb whenever it comes up into a new
echo   act and starts some of the climbs from one of those copies. Those
echo   climbs are learned from and left out of every table, so the floors
echo   and the won share on the curve go on meaning what they meant.
echo.
echo     B. Back
echo.
set "PICK="
set /p "PICK=  Choose [1]: "

if not defined PICK set "PICK=1"
if /i "%PICK%"=="b" goto size
set "DEEP="
if "%PICK%"=="1" set "DEEP=0.4"
if "%PICK%"=="2" set "DEEP=0"
if defined DEEP goto go
echo.
echo   That was not one of them.
pause
goto deep

rem -------------------------------------------------------------------- off
:go

rem Anything the questions did not settle, because the questions were
rem skipped or because this file is newer than the notes it left.
if not defined DEEP set "DEEP=0.4"

rem And the answers, so that next time is one keypress or none of them.
rem Written before the trainer starts rather than after, because the way
rem this ends is Ctrl-C.
md runs 2>nul
> "runs\last.bat" echo set "CHARACTER=%CHARACTER%"
>>"runs\last.bat" echo set "ACTS=%ACTS%"
>>"runs\last.bat" echo set "ACTSWAS=%ACTS%"
>>"runs\last.bat" echo set "ENVS=%ENVS%"
>>"runs\last.bat" echo set "WIDTH=%WIDTH%"
>>"runs\last.bat" echo set "GAMMA=%GAMMA%"
>>"runs\last.bat" echo set "HPW=%HPW%"
>>"runs\last.bat" echo set "DEEP=%DEEP%"

cls
echo ==========================================================
echo   Training %CHARACTER%
echo ==========================================================
echo.
echo   act limit   : %ACTS%   - 0 means the whole spire
echo   climbs      : %ENVS% at once
echo   brain       : %WIDTH% wide
echo   looks ahead : %GAMMA%
echo   a point of hp: %HPW%
echo   later acts  : %DEEP% of climbs start part-way up
echo   saved to    : runs\%CHARACTER%\checkpoint.pt
echo   the curve   : runs\%CHARACTER%\curve.csv
echo.
echo   Ctrl-C stops it. It saves first, and running this again carries on
echo   at the rate it had worked its way down to - the checkpoint carries
echo   that, so it is not something to remember.
echo.
echo   floors is what moves first, then boss - the share of climbs that put
echo   an act boss down. win is the share that got as far as they were asked
echo   to get, so with an act limit it starts moving early; asked for the
echo   whole spire it waits on the third act's boss and takes much longer.
echo.

%PYTHON% "%TRAINER%" --character %CHARACTER% --acts %ACTS% --envs %ENVS% --width %WIDTH% --gamma %GAMMA% --hp-weight %HPW% --deep %DEEP% --picks %FRESH% %EXTRA%

echo.
echo ==========================================================
echo   Stopped. Everything is in runs\%CHARACTER%\
echo ==========================================================
echo.
pause

@echo off
title Conquer the Spire - playing

rem Everything runs from the folder this file sits in.
cd /d "%~dp0"

rem The python with torch and numpy in it, by name - the same one train.bat
rem uses. "python" on its own can be another install without them.
set "PYTHON=%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
if not exist "%PYTHON%" set "PYTHON=python"

set "CHARACTER=ironclad"
if exist "runs\last.bat" call "runs\last.bat"

set "FOLDER=runs\%CHARACTER%"

if exist "%FOLDER%\best-look2.pt" goto haveWeights
if exist "%FOLDER%\checkpoint.pt" goto haveWeights
echo   Nothing has been trained yet: %FOLDER% is empty.
echo   Run train.bat first.
echo.
pause
exit /b 1

:haveWeights

rem The climber that plays best, measured: best-look2.pt of update 356200
rem won 74.5%% of 200 climbs with the turn searched, against 72.0%% for the
rem newer well.pt on the same seeds. Older weights, better play - the
rem search is most of what wins, not the training.
set "WEIGHTS=%FOLDER%\best-look2.pt"
if not exist "%WEIGHTS%" set "WEIGHTS=%FOLDER%\well.pt"
if not exist "%WEIGHTS%" set "WEIGHTS=%FOLDER%\best.pt"
if not exist "%WEIGHTS%" set "WEIGHTS=%FOLDER%\checkpoint.pt"

:menu
cls
echo ==========================================================
echo   Conquer the Spire - playing %CHARACTER%
echo ==========================================================
echo.
echo   Playing %WEIGHTS%
echo.
echo   Measured on these weights, 200 climbs on seeds 500005,
echo   every one played to its end:
echo.
echo     the quick way - looks at 2 moves      45.5%% won
echo     the strong way - the turn played out  74.5%% won
echo.
echo   The strong way plays the whole turn on a copy of the
echo   fight before making its first move: sequences grown a
echo   move at a time, the best few kept, and the one that
echo   ends the turn scored after the monsters have answered.
echo   It costs about nine seconds a climb against a tenth of
echo   one, and it is worth thirty points of winning.
echo.
echo     1. Play 20 climbs, the strong way    - about 3 minutes
echo     2. Play 100 climbs, the strong way   - about 15 minutes
echo     3. Play 200 climbs, the strong way   - about 30 minutes
echo     4. Play 200 climbs, the quick way    - under a minute
echo     5. Play 20 climbs on seeds nobody has used
echo     6. Watch one climb - every card it took and passed on
echo.
echo     Q. Quit
echo.
set "PICK="
set /p "PICK=  Choose [1]: "

if not defined PICK set "PICK=1"
if /i "%PICK%"=="q" exit /b 0

rem The seeds the numbers above were taken on, so a short run here
rem reads against them. Choice 5 takes fresh ones instead.
set "CLIMBS="
set "SEED=500005"
set "HOW=--turn --envs 8"
if "%PICK%"=="1" set "CLIMBS=20"
if "%PICK%"=="2" set "CLIMBS=100"
if "%PICK%"=="3" set "CLIMBS=200"
if "%PICK%"=="4" set "CLIMBS=200" & set "HOW=--envs 32"
if "%PICK%"=="5" set "CLIMBS=20" & set "SEED=%RANDOM%00"
if "%PICK%"=="6" goto watch
if defined CLIMBS goto play
echo.
echo   That was not one of them.
pause
goto menu

:play
cls
echo   Playing %CLIMBS% climbs. Every one is played to its end, so this
echo   cannot be hurried - the count at the end is the whole of it.
echo.
echo   Ctrl-C stops it.
echo.
"%PYTHON%" Python\cts_play.py "%WEIGHTS%" %CLIMBS% %HOW% --seed %SEED%
echo.
pause
goto menu

:watch
cls
echo   One climb, written down: every card taken and passed on, every
echo   relic, every room, floor by floor. It plays seeds in turn until
echo   one is won, because that is the one worth reading.
echo.
echo   Add --cards to see every card it played in its fights as well.
echo.
"%PYTHON%" Python\cts_watch.py "%WEIGHTS%" --until-won --seed %RANDOM%00 %*
echo.
pause
goto menu

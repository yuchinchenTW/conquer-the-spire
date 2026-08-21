@echo off
title Conquer the Spire - watching

rem Everything runs from the folder this file sits in.
cd /d "%~dp0"

set "PYTHON=python"
set "PLOTTER=Python\cts_plot.py"
set "EXTRA=%*"

%PYTHON% --version >nul 2>&1
if not errorlevel 1 goto havePython
echo Python was not found on the path.
echo.
pause
exit /b 1

:havePython

rem ---------------------------------------------------------- the character
:character
cls
echo ==========================================================
echo   Conquer the Spire - watching a training run
echo ==========================================================
echo.
echo   Whose run?
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
if defined CHARACTER goto found
echo.
echo   That was not one of them.
pause
goto character

:found
if exist "runs\%CHARACTER%\curve.csv" goto how
cls
echo   Nothing has been trained for %CHARACTER% yet.
echo   Start it with train.bat, then come back here.
echo.
pause
goto character

rem ------------------------------------------------------------- how to see
:how
cls
echo ==========================================================
echo   %CHARACTER% - how would you like to watch?
echo ==========================================================
echo.
echo     1. A window that keeps redrawing itself     ^(curves + picks^)
echo     2. The page in a browser, refreshing itself ^(no libraries^)
echo     3. Tensorboard                              ^(zoom, smoothing^)
echo     4. Save one picture and stop                ^(progress.png^)
echo.
echo     B. Back
echo.
set "PICK="
set /p "PICK=  Choose [1]: "

if not defined PICK set "PICK=1"
if /i "%PICK%"=="b" goto character
if "%PICK%"=="1" goto liveWindow
if "%PICK%"=="2" goto page
if "%PICK%"=="3" goto board
if "%PICK%"=="4" goto picture
echo.
echo   That was not one of them.
pause
goto how

:liveWindow
cls
echo Drawing %CHARACTER%. Close the window to stop watching; the training
echo itself carries on either way.
echo.
%PYTHON% "%PLOTTER%" %CHARACTER% %EXTRA%
goto done

:page
%PYTHON% "%PLOTTER%" %CHARACTER% --html
start "" "runs\%CHARACTER%\progress.html"
cls
echo Opened runs\%CHARACTER%\progress.html
echo.
echo The trainer writes that page again every time it reports, and the page
echo reloads itself every fifteen seconds. Leave it open while it trains.
echo.
pause
goto how

:board
cls
echo Starting tensorboard on http://localhost:6006
echo Ctrl-C in this window stops it.
echo.
start "" "http://localhost:6006"
%PYTHON% -m tensorboard.main --logdir runs
goto done

:picture
%PYTHON% "%PLOTTER%" %CHARACTER% --once
echo.
start "" "runs\%CHARACTER%\progress.png"
pause
goto how

:done
echo.
pause

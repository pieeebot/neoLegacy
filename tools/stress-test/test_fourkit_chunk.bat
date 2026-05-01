@echo off
REM FourKit chunk + move event stress: 50 concurrent moving bots held for 1-2min
REM each, exercising FireChunkLoad / FireChunkUnload / FirePlayerMove. Validates
REM the HasHandlers fast-path and Server GC at the 50-player target.
REM
REM Set require-secure-client=false in server.properties before running. The
REM 100-tick cipher handshake grace cannot keep up with 50 simultaneous bot
REM joins, which is unrelated to what this test is measuring.
set /p HOST="Server IP [127.0.0.1]: " || set HOST=127.0.0.1
set /p PORT="Server Port [25565]: " || set PORT=25565
if "%HOST%"=="" set HOST=127.0.0.1
if "%PORT%"=="" set PORT=25565
python "%~dp0stress_test.py" %HOST% %PORT% --bots 50 --burst 10 --move --hold 60 120 --ramp 0.5 --duration 600 --cycles 0 --quiet
pause

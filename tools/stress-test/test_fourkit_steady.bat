@echo off
REM FourKit steady-state TPS validation: 50 concurrent moving bots that stay
REM connected for the full duration with NO disconnect/reconnect rotation. The
REM "real workload" complement to test_fourkit_chunk.bat. Isolates steady-state
REM per-tick cost from the mass-disconnect cleanup spikes that bot rotation
REM triggers. Validates that chunk-send throttles hold 20 TPS at scale once
REM the initial scatter chunk-load wave has drained.
REM
REM Suggested in-game routine while this is running:
REM   1. Wait ~10s for bots to fully join.
REM   2. Run /fktest scatter to spread them across the world.
REM   3. Wait ~30-60s for chunk-load wave to drain.
REM   4. Run /fktest tps and read the 5s/30s/60s windows.
REM
REM Set require-secure-client=false in server.properties before running. The
REM 100-tick cipher handshake grace cannot keep up with 50 simultaneous bot
REM joins, which is unrelated to what this test is measuring.
set /p HOST="Server IP [127.0.0.1]: " || set HOST=127.0.0.1
set /p PORT="Server Port [25565]: " || set PORT=25565
if "%HOST%"=="" set HOST=127.0.0.1
if "%PORT%"=="" set PORT=25565
REM --hold 99999 99999  : bots never reach the hold timeout during the run, so
REM                       they stay connected for the full duration.
REM --cycles 0          : the spawner keeps launching until --bots is reached.
REM                       (cycles is a global spawn counter, NOT a per-bot
REM                       reconnect cap; cycles=1 would cap total spawns at 1.)
REM --duration 600      : 10-minute run; cap at whatever you need
python "%~dp0stress_test.py" %HOST% %PORT% --bots 50 --burst 10 --move --hold 99999 99999 --ramp 0.5 --duration 600 --cycles 0 --quiet
pause

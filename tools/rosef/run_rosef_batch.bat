@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%..\.."
python "%SCRIPT_DIR%run_rosef_batch.py" %*
set RESULT=%ERRORLEVEL%
echo.
if "%RESULT%"=="0" echo Toate verificarile au trecut.
if "%RESULT%"=="1" echo Una sau mai multe verificari au esuat. Consultati raportul.
if "%RESULT%"=="2" echo Pipeline-ul complet este blocat de o dependenta lipsa. Consultati raportul.
exit /b %RESULT%

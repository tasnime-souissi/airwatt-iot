@echo off
rem AirWatt - lance la passerelle PC (BLE -> ThingsBoard).
rem L'environnement Python est cree hors de OneDrive, dans %LOCALAPPDATA%\airwatt\venv.
setlocal
set VENV=%LOCALAPPDATA%\airwatt\venv
if not exist "%VENV%\Scripts\python.exe" (
  echo Premiere utilisation : installation de bleak et paho-mqtt...
  python -m venv "%VENV%" || goto :erreur
  "%VENV%\Scripts\python.exe" -m pip install --quiet --upgrade pip
  "%VENV%\Scripts\python.exe" -m pip install --quiet -r "%~dp0requirements.txt" || goto :erreur
)
"%VENV%\Scripts\python.exe" "%~dp0gateway_pc.py"
goto :eof
:erreur
echo Echec de l'installation. Verifier que Python est installe et la connexion Internet.
pause

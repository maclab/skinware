@echo off
if "%1" == "--uninstall" (
	if exist %SystemRoot%\system32\docthis.bat del %SystemRoot%\system32\docthis.bat
) else (
	echo %2/docthis.exe %%* > %SystemRoot%\system32\docthis.bat
)

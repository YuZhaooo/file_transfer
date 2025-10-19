@echo off
set THISDIR=%cd%

if "%UEDIR%"=="" (echo UEDIR is NOT set, using default UE path) && (set UEDIR=C:\Program Files\Epic Games\UE_4.27)
echo UEDIR %UEDIR%

set UBT=%UEDIR%\Engine\Build\BatchFiles\Build.bat
set UAT=%UEDIR%\Engine\Build\BatchFiles\RunUAT.bat
set UEEDITOR=%UEDIR%\Engine\Binaries\Win64\UE4Editor-Cmd.exe
set PROJECT=%cd%\DMS_Simulation\DMS_Simulation.uproject
set PROJECTNAME=UnrealDmsSimulation

:COMPILE
echo "Building UE4 project DMS_Simulation"
call "%UBT%" Development Win64 -Project="%PROJECT%" -TargetType=Editor -Progress -NoEngineChanges -NoHotReloadFromIDE || goto END
call "%UBT%" Development Win64 -Project="%PROJECT%" -TargetType=Editor -Progress -NoEngineChanges -NoHotReloadFromIDE || goto END

echo Preparing animations
call "%UEEDITOR%"  "%PROJECT%"  -run=pythonscript -script="%THISDIR%\DMS_Simulation\Content\Python\BuildAnimations.py" -nullRHI ^
	|| exit /b

if "%1" == "Publish" (
	echo 'b'
	set exportdir=%2
	set
	if "%2"=="" (
		echo "Provide export directory name as a second argument to publish"
		set ERRORLEVEL=6
		goto END
	)
	
	echo Building animation catalog
	call "%UEEDITOR%"  "%PROJECT%"  -run=pythonscript -script="%THISDIR%\DMS_Simulation\Content\Python\ExportAnimationCatalog.py" -nullRHI ^
	
	:COOK
	echo Exporting project to %exportdir%
	call "%UAT%" ^
		BuildCookRun ^
		-nocompileeditor ^
		-installed ^
		-nop4 ^
		-cook ^
		-stage ^
		-archive ^
		-package ^
		-compressed ^
		-ddc=InstalledDerivedDataBackendGraph ^
		-pak ^
		-prereqs ^
		-nodebuginfo ^
		-targetplatform=Win64 ^
		-build ^
		-clientconfig=Development ^
		-utf8output ^
		-ScriptsForProject="%PROJECT%" ^
		-project="%PROJECT%" ^
		-archivedirectory="%2" ^
		-ue4exe="%UEEDITOR%" ^
		-target=%PROJECTNAME% ^
		-CookAll ^
		-allmaps ^
		-iterate ^
		|| exit /b
	copy %THISDIR%\Distribution\*.* %2\WindowsNoEditor\
	copy %THISDIR%\RELEASE.txt %2\WindowsNoEditor\
	copy %THISDIR%\AnimationCatalog\Generated\*.html %2\WindowsNoEditor\
	copy %THISDIR%\AnimationCatalog\Generated\*.md %2\WindowsNoEditor\

)



:END
if "%UNATTENDED%"=="" pause else exit /b

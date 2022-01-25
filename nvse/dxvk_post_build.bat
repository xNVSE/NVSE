:: Simple batch file to disable/enable DXVK when EDITOR or RUNTIME is built as DXVK makes GECK loading slow
@ECHO OFF
set arg=%1
if %arg% == runtime (
    set filepath1="%FalloutNVPath%\d3d9.disabled"
    set filepath2="%FalloutNVPath%\dxgi.disabled"
    set filepath1renamed=d3d9.dll
    set filepath2renamed=dxgi.dll
    echo Enabling DXVK with debug EDITOR build
)
if %arg% == editor (
    set filepath1="%FalloutNVPath%\d3d9.dll"
    set filepath2="%FalloutNVPath%\dxgi.dll"
    set filepath1renamed=d3d9.disabled
    set filepath2renamed=dxgi.disabled
    echo Disabling DXVK with debug RELEASE build
)

if exist %filepath1% (
    ren %filepath1% %filepath1renamed%
    if exist %filepath2% (
        ren %filepath2% %filepath2renamed%
    )
)

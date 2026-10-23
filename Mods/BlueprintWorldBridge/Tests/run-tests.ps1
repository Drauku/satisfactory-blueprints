$ErrorActionPreference = "Stop"

$TestDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SourceDir = Join-Path $TestDir "..\Source\BlueprintWorldBridge\Public"
$Output = Join-Path $env:TEMP "blueprint-world-names-test.exe"

try {
    cl /nologo /std:c++20 /W4 /WX /EHsc /I $SourceDir `
        (Join-Path $TestDir "BlueprintWorldNamesTest.cpp") /Fe:$Output
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    & $Output
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
finally {
    Remove-Item -ErrorAction SilentlyContinue $Output
    Remove-Item -ErrorAction SilentlyContinue ($Output -replace '\.exe$', '.obj')
}

param(
    [string]$PythonVersion = "3.12.3",
    [string]$PythonArch = "amd64",
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$BuildArgs
)

$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$PythonRoot = Join-Path $RootDir "tools\cache\python\windows-embed"
$PythonExe = Join-Path $PythonRoot "python.exe"
$BuildScript = Join-Path $RootDir "tools\build\build.py"

if (-not $BuildArgs -or $BuildArgs.Count -eq 0) {
    $BuildArgs = @("bootstrap")
}

function Install-EmbeddedPython {
    param(
        [string]$Version,
        [string]$Arch,
        [string]$Destination
    )

    $ZipName = "python-$Version-embed-$Arch.zip"
    $Url = "https://www.python.org/ftp/python/$Version/$ZipName"
    $ArchivePath = Join-Path ([System.IO.Path]::GetTempPath()) $ZipName

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null

    Write-Host "Downloading embedded Python $Version ($Arch)..."
    Invoke-WebRequest -Uri $Url -OutFile $ArchivePath

    Write-Host "Extracting embedded Python to $Destination..."
    Expand-Archive -LiteralPath $ArchivePath -DestinationPath $Destination -Force
}

if (-not (Test-Path $PythonExe)) {
    Install-EmbeddedPython -Version $PythonVersion -Arch $PythonArch -Destination $PythonRoot
}

& $PythonExe $BuildScript @BuildArgs

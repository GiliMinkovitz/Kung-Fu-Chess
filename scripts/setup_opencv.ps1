$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Dest = Join-Path $Root "third_party\CTD26\cpp\OpenCV_451"

if (Test-Path (Join-Path $Dest "include\opencv2")) {
    Write-Host "OpenCV already at $Dest"
    exit 0
}

$Candidates = @(
    (Join-Path $Root "third_party\OpenCV_451-20260714T120224Z-1-001\OpenCV_451"),
    (Join-Path $Root "third_party\OpenCV_451")
)

$Source = $null
foreach ($Candidate in $Candidates) {
    if (Test-Path (Join-Path $Candidate "include\opencv2")) {
        $Source = $Candidate
        break
    }
}

if (-not $Source) {
    Write-Error "Could not find OpenCV_451 (expected include\opencv2)."
}

New-Item -ItemType Directory -Force -Path (Split-Path $Dest) | Out-Null
Move-Item -Path $Source -Destination $Dest
Write-Host "Moved OpenCV to $Dest"

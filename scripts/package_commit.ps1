param(
    [Parameter(Mandatory=$true)]
    [string]$ZipName
)

$targetDir = "C:\Users\aidanag\Documents\Obsidian\Motores de videojuegos\Entrega 2\commits_zips"
if (-not (Test-Path $targetDir)) {
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
}

$zipPath = Join-Path $targetDir "$ZipName.zip"
if (Test-Path $zipPath) {
    Remove-Item -Force $zipPath
}

# Collect items to zip, excluding build artifacts and git internals
$items = Get-ChildItem -Path . | Where-Object {
    $_.Name -notin @('build', '.vs', '.git', 'out')
} | Select-Object -ExpandProperty FullName

Compress-Archive -Path $items -DestinationPath $zipPath -Force
Write-Host "Created commit archive: $zipPath"
Get-Item $zipPath | Select-Object Name, Length

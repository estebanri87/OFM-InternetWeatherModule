# Prueft jede im Katalog behauptete Kombination (Zeitebene x Open-Meteo-Variable)
# gegen die echte Open-Meteo-API. Die API-Dokumentation ist an mehreren Stellen
# ungenau, daher ist die empirische Pruefung massgeblich.
#
#   pwsh -File tools/Verify-Catalog.ps1
#
# Exit-Code 0 = alle Kombinationen liefern Daten, 1 = mindestens eine nicht.

param(
    [string] $CatalogPath = "$PSScriptRoot/../src/weather-catalog.json",
    [double] $Latitude = 48.137,
    [double] $Longitude = 11.575,
    [int]    $DelayMs = 250
)

$ErrorActionPreference = 'Stop'

$catalog = Get-Content $CatalogPath -Raw -Encoding UTF8 | ConvertFrom-Json

# Nur echte API-Variablen pruefen; '@...' sind im Modul berechnete Werte.
$pairs = $catalog.measurements |
    Where-Object { -not $_.omVariable.StartsWith('@') } |
    ForEach-Object { [pscustomobject]@{ Level = $_.level; Variable = $_.omVariable } } |
    Sort-Object Level, Variable -Unique

Write-Host ("Pruefe {0} Kombinationen gegen api.open-meteo.com ..." -f $pairs.Count)
Write-Host ""

$failed = @()

foreach ($p in $pairs)
{
    # Zeitraum minimal halten, es geht nur um die Frage ob die Variable existiert.
    $range = switch ($p.Level)
    {
        'minutely_15' { '&forecast_minutely_15=4' }
        'hourly'      { '&forecast_hours=1' }
        'daily'       { '&forecast_days=1' }
        default       { '' }
    }

    $url = 'https://api.open-meteo.com/v1/forecast' +
           "?latitude=$Latitude&longitude=$Longitude&timezone=auto&timeformat=unixtime" +
           ('&{0}={1}' -f $p.Level, $p.Variable) + $range

    $status = 'OK'
    try
    {
        $resp = Invoke-RestMethod -Uri $url -Method Get -TimeoutSec 20
        $section = $resp.($p.Level)
        if ($null -eq $section)
        {
            $status = 'FEHLT (Abschnitt nicht in der Antwort)'
        }
        elseif ($null -eq $section.($p.Variable))
        {
            $status = 'FEHLT (Variable nicht in der Antwort)'
        }
    }
    catch
    {
        $msg = $_.Exception.Message
        # Open-Meteo liefert bei unbekannter Variable HTTP 400 mit 'reason'.
        if ($_.ErrorDetails.Message)
        {
            try { $msg = (($_.ErrorDetails.Message | ConvertFrom-Json).reason) } catch { }
        }
        $status = "FEHLER: $msg"
    }

    if ($status -ne 'OK') { $failed += "$($p.Level)/$($p.Variable): $status" }

    $marker = if ($status -eq 'OK') { '  ok ' } else { ' FAIL' }
    Write-Host ('{0} {1,-13} {2,-32} {3}' -f $marker, $p.Level, $p.Variable, $status)

    Start-Sleep -Milliseconds $DelayMs
}

Write-Host ""
if ($failed.Count -eq 0)
{
    Write-Host ("Alle {0} Kombinationen bestaetigt." -f $pairs.Count) -ForegroundColor Green
    exit 0
}

Write-Host ("{0} von {1} Kombinationen fehlerhaft:" -f $failed.Count, $pairs.Count) -ForegroundColor Red
$failed | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
exit 1

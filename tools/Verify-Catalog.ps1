# Prüft jede im Katalog behauptete Kombination aus Zeitebene und Anbieter-Variable
# gegen die echte API. Die Anbieter-Dokumentation ist an mehreren Stellen ungenau,
# daher ist die empirische Prüfung maßgeblich.
#
#   pwsh -File tools/Verify-Catalog.ps1
#   pwsh -File tools/Verify-Catalog.ps1 -OwmApiKey <key>
#
# Ohne API-Key wird nur Open-Meteo geprüft; One Call 3.0 erfordert ein Abonnement.
# Exit-Code 0 = alle geprüften Kombinationen liefern Daten, 1 = mindestens eine nicht.

param(
    [string] $CatalogPath = "$PSScriptRoot/../src/weather-catalog.json",
    [double] $Latitude = 48.137,
    [double] $Longitude = 11.575,
    [string] $OwmApiKey = '',
    [int]    $DelayMs = 250
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$catalog = Get-Content $CatalogPath -Raw -Encoding UTF8 | ConvertFrom-Json

function Get-ProviderVar($m, [string] $key)
{
    $prop = $m.providers.PSObject.Properties | Where-Object Name -eq $key
    if ($prop) { $prop.Value } else { $null }
}

$failed = @()

# ---------------------------------------------------------------- Open-Meteo

$omPairs = $catalog.measurements |
    ForEach-Object {
        $pv = Get-ProviderVar $_ 'openmeteo'
        if ($pv -and -not $pv.var.StartsWith('@')) { [pscustomobject]@{ Level = $_.level; Variable = $pv.var } }
    } | Sort-Object Level, Variable -Unique

Write-Host ("Open-Meteo: prüfe {0} Kombinationen ..." -f $omPairs.Count)
foreach ($p in $omPairs)
{
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
        if ($null -eq $section) { $status = 'FEHLT (Abschnitt fehlt)' }
        elseif ($null -eq $section.($p.Variable)) { $status = 'FEHLT (Variable fehlt)' }
    }
    catch
    {
        $msg = $_.Exception.Message
        if ($_.ErrorDetails.Message) { try { $msg = ($_.ErrorDetails.Message | ConvertFrom-Json).reason } catch { } }
        $status = "FEHLER: $msg"
    }

    if ($status -ne 'OK') { $failed += "open-meteo $($p.Level)/$($p.Variable): $status" }
    Write-Host ('{0} {1,-13} {2,-34} {3}' -f $(if ($status -eq 'OK') { '  ok ' } else { ' FAIL' }), $p.Level, $p.Variable, $status)
    Start-Sleep -Milliseconds $DelayMs
}

# ---------------------------------------------------------------- OpenWeatherMap

Write-Host ''
if ([string]::IsNullOrWhiteSpace($OwmApiKey))
{
    $owmCount = @($catalog.measurements | Where-Object { Get-ProviderVar $_ 'openweathermap' }).Count
    Write-Host ("OpenWeatherMap: {0} Messwerte nicht geprüft - kein API-Key uebergeben (-OwmApiKey)." -f $owmCount) -ForegroundColor Yellow
}
else
{
    # One Call 3.0 liefert alle Abschnitte in einer Antwort; ein Aufruf genügt.
    $url = "https://api.openweathermap.org/data/3.0/onecall?lat=$Latitude&lon=$Longitude&units=metric&lang=de&exclude=minutely,alerts&appid=$OwmApiKey"
    $resp = Invoke-RestMethod -Uri $url -Method Get -TimeoutSec 20

    $sectionOf = @{ current = { $resp.current }; hourly = { $resp.hourly[0] }; daily = { $resp.daily[0] } }

    $owmPairs = $catalog.measurements |
        ForEach-Object {
            $pv = Get-ProviderVar $_ 'openweathermap'
            if ($pv -and -not $pv.var.StartsWith('@')) { [pscustomobject]@{ Level = $_.level; Variable = $pv.var } }
        } | Sort-Object Level, Variable -Unique

    Write-Host ("OpenWeatherMap: prüfe {0} Kombinationen ..." -f $owmPairs.Count)
    foreach ($p in $owmPairs)
    {
        $node = & $sectionOf[$p.Level]
        foreach ($part in $p.Variable.Split('.'))
        {
            if ($null -eq $node) { break }
            $node = if ($part -match '^\d+$') { $node[[int]$part] } else { $node.$part }
        }
        # Regen und Schnee fehlen bei trockenem Wetter voellig - das ist kein Katalogfehler.
        $optional = $p.Variable -like 'rain*' -or $p.Variable -like 'snow*'
        $status = if ($null -ne $node) { 'OK' } elseif ($optional) { 'ok (derzeit ohne Niederschlag)' } else { 'FEHLT' }
        if ($status -eq 'FEHLT') { $failed += "openweathermap $($p.Level)/$($p.Variable): FEHLT" }
        Write-Host ('{0} {1,-13} {2,-34} {3}' -f $(if ($status -eq 'FEHLT') { ' FAIL' } else { '  ok ' }), $p.Level, $p.Variable, $status)
    }
}

Write-Host ''
if ($failed.Count -eq 0)
{
    Write-Host 'Alle geprüften Kombinationen bestätigt.' -ForegroundColor Green
    exit 0
}
Write-Host ("{0} Kombinationen fehlerhaft:" -f $failed.Count) -ForegroundColor Red
$failed | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
exit 1

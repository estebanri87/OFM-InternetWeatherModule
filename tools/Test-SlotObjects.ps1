# Prüft im erzeugten Slot-Fragment, dass jeder Messwert des Katalogs auf genau
# eine Variante des Kommunikationsobjekts führt und dass diese Variante den
# richtigen Datenpunkttyp, die richtige Größe und den Namen des Messwerts in der
# Objektfunktion trägt.
#
#   pwsh -File tools/Test-SlotObjects.ps1
#
# Exit-Code 0 = alle Messwerte korrekt, 1 = mindestens ein Fehler.

param(
    [string] $Root = "$PSScriptRoot/.."
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$catalog = Get-Content (Join-Path $Root 'src/weather-catalog.json') -Raw -Encoding UTF8 | ConvertFrom-Json
[xml] $slot = Get-Content (Join-Path $Root 'src/InternetWeatherModule.slot.part.xml') -Raw -Encoding UTF8

$ns = New-Object System.Xml.XmlNamespaceManager($slot.NameTable)
$ns.AddNamespace('k', 'http://knx.org/xml/project/14')

# Varianten nach Id
$refs = @{}
foreach ($r in $slot.SelectNodes('//k:ComObjectRefs/k:ComObjectRef', $ns)) { $refs[$r.Id] = $r }

# Letzter choose am Schattenparameter mit ComObjectRefRef: Messwert-Id -> Ref-Id
$chooser = $slot.SelectNodes('//k:choose[k:when/k:ComObjectRefRef]', $ns) | Select-Object -Last 1
$target = @{}
$duplicates = @()
foreach ($when in $chooser.SelectNodes('k:when', $ns))
{
    $refId = $when.SelectSingleNode('k:ComObjectRefRef', $ns).RefId
    foreach ($id in ($when.test -split '\s+'))
    {
        if ($target.ContainsKey([int]$id)) { $duplicates += $id }
        $target[[int]$id] = $refId
    }
}

$errors = @()
foreach ($id in $duplicates) { $errors += "Messwert $id führt auf mehrere Varianten" }

foreach ($m in $catalog.measurements)
{
    if (-not $target.ContainsKey([int]$m.id)) { $errors += "Messwert $($m.id) '$($m.label)': keine Variante"; continue }
    $ref = $refs[$target[[int]$m.id]]
    if ($null -eq $ref) { $errors += "Messwert $($m.id): Variante $($target[[int]$m.id]) existiert nicht"; continue }

    if ($ref.DatapointType -ne $m.dpt)        { $errors += "Messwert $($m.id) '$($m.label)': DPT $($ref.DatapointType) statt $($m.dpt)" }
    if ($ref.ObjectSize -ne $m.objectSize)    { $errors += "Messwert $($m.id) '$($m.label)': Größe $($ref.ObjectSize) statt $($m.objectSize)" }
    if (-not $ref.FunctionText.EndsWith(", $($m.label)")) { $errors += "Messwert $($m.id) '$($m.label)': Objektfunktion '$($ref.FunctionText)'" }
}

Write-Host ("{0} Messwerte, {1} Varianten geprüft" -f $catalog.measurements.Count, $refs.Count)
if ($errors.Count -eq 0)
{
    Write-Host 'Alle Messwerte führen auf die richtige Variante.' -ForegroundColor Green
    exit 0
}
$errors | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
exit 1

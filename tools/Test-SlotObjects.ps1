# Prüft im erzeugten Slot-Fragment, dass jeder Messwert des Katalogs auf die
# richtige Variante des Kommunikationsobjekts führt: Objektfunktion mit dem
# Namen des Messwerts, passender Datenpunkttyp und passende Größe.
#
# Windgeschwindigkeiten führen je nach globaler Windeinheit auf zwei Varianten:
# km/h (DPST-9-28) und m/s (DPST-9-5). Beide werden geprüft.
#
#   pwsh -File tools/Test-SlotObjects.ps1
#
# Exit-Code 0 = alle Messwerte korrekt, 1 = mindestens ein Fehler.

param(
    [string] $Root = "$PSScriptRoot/.."
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$windKmhDpt  = 'DPST-9-28'
$windMsDpt   = 'DPST-9-5'
$windUnitRef = '%AID%_UP-%TT%00030_R-%TT%0003001'

$catalog = Get-Content (Join-Path $Root 'src/weather-catalog.json') -Raw -Encoding UTF8 | ConvertFrom-Json
[xml] $slot = Get-Content (Join-Path $Root 'src/InternetWeatherModule.slot.part.xml') -Raw -Encoding UTF8

$ns = New-Object System.Xml.XmlNamespaceManager($slot.NameTable)
$ns.AddNamespace('k', 'http://knx.org/xml/project/14')

$refs = @{}
foreach ($r in $slot.SelectNodes('//k:ComObjectRefs/k:ComObjectRef', $ns)) { $refs[$r.Id] = $r }

# Die DPT-Auswahl ist das choose am Schattenparameter, das Kommunikationsobjekte enthält.
$chooser = $slot.SelectNodes('//k:choose[contains(@ParamRefId, "SPP+26%") and .//k:ComObjectRefRef]', $ns)
if ($chooser.Count -ne 1) { throw "Erwartet genau ein KO-choose am Schattenparameter, gefunden: $($chooser.Count)" }
$chooser = $chooser[0]

# Messwert-Id -> when-Knoten
$whenOf = @{}
$errors = @()
foreach ($when in $chooser.SelectNodes('k:when', $ns))
{
    foreach ($id in ($when.test -split '\s+'))
    {
        if ($whenOf.ContainsKey([int]$id)) { $errors += "Messwert $id führt auf mehrere Zweige" }
        $whenOf[[int]$id] = $when
    }
}

function Test-Ref($m, $ref, [string] $expectDpt, [string] $context)
{
    $e = @()
    if ($null -eq $ref) { return @("Messwert $($m.id) '$($m.label)'${context}: Variante existiert nicht") }
    if ($ref.DatapointType -ne $expectDpt)  { $e += "Messwert $($m.id) '$($m.label)'${context}: DPT $($ref.DatapointType) statt $expectDpt" }
    if ($ref.ObjectSize -ne $m.objectSize)  { $e += "Messwert $($m.id) '$($m.label)'${context}: Größe $($ref.ObjectSize) statt $($m.objectSize)" }
    if (-not $ref.FunctionText.EndsWith(", $($m.label)")) { $e += "Messwert $($m.id) '$($m.label)'${context}: Objektfunktion '$($ref.FunctionText)'" }
    return $e
}

$checked = 0
foreach ($m in $catalog.measurements)
{
    $when = $whenOf[[int]$m.id]
    if ($null -eq $when) { $errors += "Messwert $($m.id) '$($m.label)': keine Variante"; continue }

    if ($m.dpt -eq $windKmhDpt)
    {
        $unit = $when.SelectSingleNode('k:choose', $ns)
        if ($null -eq $unit -or $unit.ParamRefId -ne $windUnitRef) { $errors += "Messwert $($m.id) '$($m.label)': keine Auswahl nach Windeinheit"; continue }
        $kmh = $unit.SelectSingleNode('k:when[@test="0"]/k:ComObjectRefRef', $ns)
        $ms  = $unit.SelectSingleNode('k:when[@test="1"]/k:ComObjectRefRef', $ns)
        $errors += Test-Ref $m $refs[$kmh.RefId] $windKmhDpt ' (km/h)'
        $errors += Test-Ref $m $refs[$ms.RefId]  $windMsDpt  ' (m/s)'
        $checked += 2
    }
    else
    {
        $direct = @($when.SelectNodes('k:ComObjectRefRef', $ns))
        if ($direct.Count -ne 1) { $errors += "Messwert $($m.id) '$($m.label)': $($direct.Count) Varianten statt einer"; continue }
        $errors += Test-Ref $m $refs[$direct[0].RefId] $m.dpt ''
        $checked++
    }
}

Write-Host ("{0} Messwerte, {1} Kombinationen, {2} Varianten geprüft" -f $catalog.measurements.Count, $checked, $refs.Count)
if ($errors.Count -eq 0)
{
    Write-Host 'Alle Messwerte führen auf die richtige Variante.' -ForegroundColor Green
    exit 0
}
$errors | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
exit 1

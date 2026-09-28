# Erzeugt aus src/weather-catalog.json die vom Katalog abhängigen Artefakte:
#
#   src/InternetWeatherModule.share.xml       Region zwischen den CATALOG-Markern
#   src/InternetWeatherModule.slot.part.xml   vollständig (op:part-Fragment, 3x expandiert)
#   src/WeatherCatalog.gen.h                  C++-Metadaten
#
# Der Messwert-Enum selbst wird NICHT hier erzeugt, sondern vom OpenKNXproducer
# aus op:headerExport="enum" - siehe knxprod.h, enum class PT_Measurand.
#
#   pwsh -File tools/Generate-Catalog.ps1

param(
    [string] $Root = "$PSScriptRoot/.."
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$catalogPath = Join-Path $Root 'src/weather-catalog.json'
$sharePath   = Join-Path $Root 'src/InternetWeatherModule.share.xml'
$slotPath    = Join-Path $Root 'src/InternetWeatherModule.slot.part.xml'
$headerPath  = Join-Path $Root 'src/WeatherCatalog.gen.h'

$catalog = Get-Content $catalogPath -Raw -Encoding UTF8 | ConvertFrom-Json

$levelSuffix = @{ current = 'Cur'; minutely_15 = 'Q15'; hourly = 'Hour'; daily = 'Day' }
$levelEnum   = @{ current = 'Current'; minutely_15 = 'Minutely15'; hourly = 'Hourly'; daily = 'Daily' }
$offsetLevelOrder = @('minutely_15', 'hourly', 'daily')
$nl = "`r`n"

# Parameter-IDs innerhalb des Slot-Fragments, relativ zu %SPP%
$slotIds = @{
    Label = 0; ValueType = 1; Aggregation = 2; Send = 3
    CategoryBase = 4          #  +4 .. +5    je Anbieter
    MeasurandBase = 10        # +10 .. +25   je Anbieter (8er-Block) und Kategorie
    Shadow = 26
    OffsetFromBase = 27       # +27 .. +32   je Anbieter (3er-Block) und Zeitebene
    OffsetToBase = 33         # +33 .. +38
}

# ---------------------------------------------------------------- Hilfsfunktionen

function ConvertTo-Identifier([string] $text)
{
    # Umlaute bewusst umschreiben: hieraus entstehen C++-Bezeichner und XML-Ids,
    # die keine Nicht-ASCII-Zeichen enthalten dürfen.
    $t = $text
    foreach ($p in @(@('ä','ae'), @('ö','oe'), @('ü','ue'), @('Ä','Ae'), @('Ö','Oe'), @('Ü','Ue'), @('ß','ss')))
    {
        $t = $t.Replace($p[0], $p[1])
    }
    $parts = [regex]::Split($t, '[^A-Za-z0-9]+') | Where-Object { $_ -ne '' }
    -join ($parts | ForEach-Object {
        if ($_ -cmatch '^[A-Z0-9]+$') { $_ } else { $_.Substring(0,1).ToUpper() + $_.Substring(1) }
    })
}

function ConvertTo-DptIdentifier([string] $dpt)
{
    if ($dpt -match '^DPST-(\d+)-(\d+)$') { "Dpt$($Matches[1])_$($Matches[2])" }
    else { throw "Unerwartetes DPT-Format: $dpt" }
}

function Get-DisplayText($m)
{
    $suffix = ($catalog.levels | Where-Object key -eq $m.level).suffix
    "$($m.label) - $suffix"
}

function Get-HeaderName($m) { (ConvertTo-Identifier $m.label) + $levelSuffix[$m.level] }

function Format-TestList([int[]] $values) { ($values | Sort-Object) -join ' ' }

function Protect-Xml([string] $s) { $s.Replace('&', '&amp;').Replace('<', '&lt;').Replace('>', '&gt;') }

function Get-ProviderVar($m, [string] $providerKey)
{
    $prop = $m.providers.PSObject.Properties | Where-Object Name -eq $providerKey
    if ($prop) { $prop.Value } else { $null }
}

function Test-Supports($m, [string] $providerKey) { $null -ne (Get-ProviderVar $m $providerKey) }

# ---------------------------------------------------------------- Prüfung und Indizes

$names = @{}
foreach ($m in $catalog.measurements)
{
    $n = Get-HeaderName $m
    if ($names.ContainsKey($n)) { throw "Doppelter headerName '$n' (IDs $($names[$n]) und $($m.id))" }
    $names[$n] = $m.id
    if (-not $levelSuffix.ContainsKey($m.level)) { throw "ID $($m.id): unbekannte Zeitebene '$($m.level)'" }
    if (@($m.providers.PSObject.Properties).Count -eq 0) { throw "ID $($m.id): kein Anbieter hinterlegt" }
}

$provIndex = @{}
for ($i = 0; $i -lt $catalog.providers.Count; $i++) { $provIndex[$catalog.providers[$i].key] = $i }
$catIndex = @{}
for ($i = 0; $i -lt $catalog.categories.Count; $i++) { $catIndex[$catalog.categories[$i].key] = $i }

$byProvider = @{}; $byProvCat = @{}; $byProvLvl = @{}
foreach ($p in $catalog.providers)
{
    $byProvider[$p.key] = @($catalog.measurements | Where-Object { Test-Supports $_ $p.key })
    foreach ($c in $catalog.categories) { $byProvCat["$($p.key)|$($c.key)"] = @($byProvider[$p.key] | Where-Object category -eq $c.key) }
    foreach ($l in $catalog.levels)     { $byProvLvl["$($p.key)|$($l.key)"] = @($byProvider[$p.key] | Where-Object level -eq $l.key) }
}

function Get-ProviderCategories([string] $pk) { @($catalog.categories | Where-Object { $byProvCat["$pk|$($_.key)"].Count -gt 0 }) }
function Get-ProviderLevels([string] $pk)     { @($catalog.levels     | Where-Object { $byProvLvl["$pk|$($_.key)"].Count -gt 0 }) }

# DPT-Klassen, Reihenfolge stabil über die erste Verwendung
$dptOrder = @(); $byDpt = @{}
foreach ($m in $catalog.measurements)
{
    $key = "$($m.dpt)|$($m.objectSize)"
    if (-not $byDpt.ContainsKey($key)) { $byDpt[$key] = @(); $dptOrder += $key }
    $byDpt[$key] += $m
}

Write-Host ("Katalog geprüft: {0} Messwerte, {1} eindeutige headerNames, {2} DPT-Klassen" -f `
    $catalog.measurements.Count, $names.Count, $dptOrder.Count)
foreach ($p in $catalog.providers)
{
    Write-Host ("  {0,-16} {1,3} Messwerte, {2} Kategorien, {3} Zeitebenen" -f `
        $p.label, $byProvider[$p.key].Count, (Get-ProviderCategories $p.key).Count, (Get-ProviderLevels $p.key).Count)
}

# ---------------------------------------------------------------- 1) share.xml-Region

$sb = [System.Text.StringBuilder]::new()
function Add([string] $line) { [void]$sb.Append($line).Append($nl) }

Add '              <!-- BEGIN GENERATED CATALOG - erzeugt von tools/Generate-Catalog.ps1 aus src/weather-catalog.json, nicht von Hand ändern -->'

foreach ($p in $catalog.providers)
{
    $provId = ConvertTo-Identifier $p.key
    $isFirst = $provIndex[$p.key] -eq 0
    $exportAttr = if ($isFirst) { 'op:headerExport="enum" op:headerName="Category"' } else { 'op:headerExport="base" op:headerName="Category"' }
    Add ('              <ParameterType Id="%AID%_PT-IWCategory{0}" Name="IWCategory{0}" {1}>' -f $provId, $exportAttr)
    Add '                <TypeRestriction Base="Value" SizeInBit="4" UIHint="DropDown">'
    Add ('                  <Enumeration Text="Bitte wählen..." Value="0" Id="%ENID%"{0} />' -f $(if ($isFirst) { ' op:headerName="None"' } else { '' }))
    foreach ($c in Get-ProviderCategories $p.key)
    {
        $hn = if ($isFirst) { ' op:headerName="' + (ConvertTo-Identifier $c.key) + '"' } else { '' }
        Add ('                  <Enumeration Text="{0}" Value="{1}" Id="%ENID%"{2} Icon="{3}" />' -f (Protect-Xml $c.label), $c.value, $hn, $c.icon)
    }
    Add '                </TypeRestriction>'
    Add '              </ParameterType>'
}
Add ''

Add '              <!-- Schattenparameter-Typ: einziger choose-Treiber, erzeugt enum class PT_Measurand -->'
Add '              <ParameterType Id="%AID%_PT-IWMeasurandAll" Name="IWMeasurandAll" op:headerExport="enum" op:headerName="Measurand">'
Add '                <TypeRestriction Base="Value" SizeInBit="8" UIHint="DropDown">'
Add '                  <Enumeration Text="Bitte wählen..." Value="0" Id="%ENID%" op:headerName="None" />'
foreach ($m in ($catalog.measurements | Sort-Object id))
{
    Add ('                  <Enumeration Text="{0}" Value="{1}" Id="%ENID%" op:headerName="{2}" />' -f `
        (Protect-Xml (Get-DisplayText $m)), $m.id, (Get-HeaderName $m))
}
Add '                </TypeRestriction>'
Add '              </ParameterType>'
Add ''

foreach ($p in $catalog.providers)
{
    $provId = ConvertTo-Identifier $p.key
    foreach ($c in Get-ProviderCategories $p.key)
    {
        $cid = ConvertTo-Identifier $c.key
        Add ('              <ParameterType Id="%AID%_PT-IWMeasurand{0}{1}" Name="IWMeasurand{0}{1}" op:headerExport="base" op:headerName="Measurand">' -f $provId, $cid)
        Add '                <TypeRestriction Base="Value" SizeInBit="8" UIHint="DropDown">'
        Add '                  <Enumeration Text="Bitte wählen..." Value="0" Id="%ENID%" />'
        foreach ($m in ($byProvCat["$($p.key)|$($c.key)"] | Sort-Object id))
        {
            Add ('                  <Enumeration Text="{0}" Value="{1}" Id="%ENID%" />' -f (Protect-Xml (Get-DisplayText $m)), $m.id)
        }
        Add '                </TypeRestriction>'
        Add '              </ParameterType>'
    }
}
Add ''

Add '              <ParameterType Id="%AID%_PT-IWSlotValueType" Name="IWSlotValueType" op:headerExport="enum" op:headerName="SlotValueType">'
Add '                <TypeRestriction Base="Value" SizeInBit="1">'
Add '                  <Enumeration Text="Einzelwert" Value="0" Id="%ENID%" op:headerName="Single" />'
Add '                  <Enumeration Text="Intervall-Aggregationen" Value="1" Id="%ENID%" op:headerName="Interval" />'
Add '                </TypeRestriction>'
Add '              </ParameterType>'
Add ''
Add '              <ParameterType Id="%AID%_PT-IWAggregation" Name="IWAggregation" op:headerExport="enum" op:headerName="Aggregation">'
Add '                <TypeRestriction Base="Value" SizeInBit="2" UIHint="DropDown">'
foreach ($a in $catalog.aggregations)
{
    Add ('                  <Enumeration Text="{0}" Value="{1}" Id="%ENID%" op:headerName="{2}" />' -f (Protect-Xml $a.label), $a.value, (ConvertTo-Identifier $a.key))
}
Add '                </TypeRestriction>'
Add '              </ParameterType>'
Add ''
Add '              <ParameterType Id="%AID%_PT-IWSendBehaviour" Name="IWSendBehaviour" op:headerExport="enum" op:headerName="SendBehaviour">'
Add '                <TypeRestriction Base="Value" SizeInBit="1">'
Add '                  <Enumeration Text="Nur bei Änderung" Value="0" Id="%ENID%" op:headerName="OnChange" />'
Add '                  <Enumeration Text="Bei jedem Abruf" Value="1" Id="%ENID%" op:headerName="Always" />'
Add '                </TypeRestriction>'
Add '              </ParameterType>'
Add ''

# Offset-Bereiche je Anbieter und Zeitebene. Kein op:headerExport auf TypeNumber -
# der Producer entfernt es dort nicht und die Schema-Validierung schlägt fehl.
foreach ($p in $catalog.providers)
{
    $provId = ConvertTo-Identifier $p.key
    foreach ($l in Get-ProviderLevels $p.key)
    {
        if ($l.key -eq 'current') { continue }
        $r = $p.levels.($l.key)
        Add ('              <ParameterType Id="%AID%_PT-IWOffset{0}{1}" Name="IWOffset{0}{1}">' -f $provId, $levelSuffix[$l.key])
        Add ('                <TypeNumber SizeInBit="16" Type="signedInt" minInclusive="{0}" maxInclusive="{1}" />' -f $r.offsetMin, $r.offsetMax)
        Add '              </ParameterType>'
    }
}

Add '              <!-- END GENERATED CATALOG -->'

$region = $sb.ToString().TrimEnd()
# share.xml ist tab-eingerückt - erzeugte Region angleichen (2 Leerzeichen = 1 Tab)
$region = ($region -split "`r`n" | ForEach-Object {
    if ($_ -match '^( +)(.*)$') { ("`t" * [int]($Matches[1].Length / 2)) + $Matches[2] } else { $_ }
}) -join "`r`n"

$share = Get-Content $sharePath -Raw -Encoding UTF8
$beginRx = '(?s)[ \t]*<!-- BEGIN GENERATED CATALOG.*?<!-- END GENERATED CATALOG -->'
if ($share -notmatch $beginRx) { throw "In $sharePath fehlen die CATALOG-Marker." }
$share = [regex]::Replace($share, $beginRx, { $region }, 1)
Set-Content -Path $sharePath -Value $share -Encoding UTF8 -NoNewline
Write-Host "geschrieben: src/InternetWeatherModule.share.xml (Katalog-Region)"

# ---------------------------------------------------------------- 2) slot.part.xml

$s = [System.Text.StringBuilder]::new()
function AddS([string] $line) { [void]$s.Append($line).Append($nl) }

function UpId([int] $n)  { '%AID%_UP-%TT%%CC%%SPP+' + $n + '%' }
function UpRef([int] $n) { (UpId $n) + '_R-%TT%%CC%%SPP+' + $n + '%01' }
function PRef([int] $n)  { '%AID%_P-%TT%%CC%%SPP+' + $n + '%_R-%TT%%CC%%SPP+' + $n + '%01' }

function CategoryId([string] $pk)                 { $slotIds.CategoryBase + $provIndex[$pk] }
function MeasurandId([string] $pk, [string] $ck)  { $slotIds.MeasurandBase + $provIndex[$pk] * 8 + $catIndex[$ck] }
function OffsetFromId([string] $pk, [string] $lk) { $slotIds.OffsetFromBase + $provIndex[$pk] * 3 + [array]::IndexOf($offsetLevelOrder, $lk) }
function OffsetToId([string] $pk, [string] $lk)   { $slotIds.OffsetToBase   + $provIndex[$pk] * 3 + [array]::IndexOf($offsetLevelOrder, $lk) }

AddS '<?xml version="1.0" encoding="utf-8"?>'
AddS '<KNX xmlns:op="http://github.com/OpenKNX/OpenKNXproducer" xmlns="http://knx.org/xml/project/14" CreatedBy="KNX MT" ToolVersion="5.1.255.16695">'
AddS '  <ManufacturerData>'
AddS '    <Manufacturer>'
AddS '      <ApplicationPrograms>'
AddS '        <ApplicationProgram>'
AddS '          <!-- ERZEUGT von tools/Generate-Catalog.ps1 aus src/weather-catalog.json - nicht von Hand ändern. -->'
AddS '          <!-- Ein Wetter-Wert-Slot. Wird per op:part dreimal expandiert (Wetter A/B/C). -->'
AddS '          <Static>'

AddS '            <Parameters>'
AddS '              <!-- Bezeichnung: kein Union, keine Memory -> nur in der ETS, nicht auf dem Gerät -->'
AddS ('              <Parameter Id="%AID%_P-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%Label" ParameterType="%AID%_PT-Text40Byte" Text="Bezeichnung" Value="" />' -f $slotIds.Label)
AddS ''
AddS '              <Union SizeInBit="48">'
AddS '                <Memory CodeSegment="%MID%" Offset="%SO%" BitOffset="0" />'
AddS '                <!-- Byte +0 Bit 7-4: Kategorie, je Anbieter eine Sicht auf dieselbe Stelle -->'
foreach ($p in $catalog.providers)
{
    AddS ('                <Parameter Id="{0}" Name="CH%C%Slot%SL%Category{1}" ParameterType="%AID%_PT-IWCategory{1}" Offset="0" BitOffset="0" Text="Kategorie" Value="0" op:nowarn="true" />' -f `
        (UpId (CategoryId $p.key)), (ConvertTo-Identifier $p.key))
}
AddS ('                <Parameter Id="{0}" Name="CH%C%Slot%SL%ValueType"   ParameterType="%AID%_PT-IWSlotValueType" Offset="0" BitOffset="4" Text="Typ" Value="0" />' -f (UpId $slotIds.ValueType))
AddS ('                <Parameter Id="{0}" Name="CH%C%Slot%SL%Aggregation" ParameterType="%AID%_PT-IWAggregation"   Offset="0" BitOffset="5" Text="Aggregation" Value="0" />' -f (UpId $slotIds.Aggregation))
AddS ('                <Parameter Id="{0}" Name="CH%C%Slot%SL%Send"        ParameterType="%AID%_PT-IWSendBehaviour" Offset="0" BitOffset="7" Text="Senden" Value="0" />' -f (UpId $slotIds.Send))
AddS ''
AddS '                <!-- Byte +1: gefilterte Messwert-Sichten je Anbieter und Kategorie -->'
foreach ($p in $catalog.providers)
{
    foreach ($c in Get-ProviderCategories $p.key)
    {
        AddS ('                <Parameter Id="{0}" Name="CH%C%Slot%SL%Measurand{1}{2}" ParameterType="%AID%_PT-IWMeasurand{1}{2}" Offset="1" BitOffset="0" Text="Messwert" Value="0" op:nowarn="true" />' -f `
            (UpId (MeasurandId $p.key $c.key)), (ConvertTo-Identifier $p.key), (ConvertTo-Identifier $c.key))
    }
}
AddS ''
AddS '                <!-- Schattenparameter: unsichtbar, per Assign gefüllt, einziger choose-Treiber -->'
AddS ('                <Parameter Id="{0}" Name="CH%C%Slot%SL%Measurand" ParameterType="%AID%_PT-IWMeasurandAll" Offset="1" BitOffset="0" Text="Messwert" Value="0" Access="None" op:nowarn="true" />' -f (UpId $slotIds.Shadow))
AddS ''
AddS '                <!-- Byte +2/+3 und +4/+5: Offset-Varianten je Anbieter und Zeitebene -->'
foreach ($p in $catalog.providers)
{
    foreach ($l in Get-ProviderLevels $p.key)
    {
        if ($l.key -eq 'current') { continue }
        $tid = (ConvertTo-Identifier $p.key) + $levelSuffix[$l.key]
        AddS ('                <Parameter Id="{0}" Name="CH%C%Slot%SL%OffsetFrom{1}" ParameterType="%AID%_PT-IWOffset{1}" Offset="2" BitOffset="0" Text="Offset" SuffixText="{2}" Value="0" op:nowarn="true" />' -f `
            (UpId (OffsetFromId $p.key $l.key)), $tid, $l.unitText)
        AddS ('                <Parameter Id="{0}" Name="CH%C%Slot%SL%OffsetTo{1}"   ParameterType="%AID%_PT-IWOffset{1}" Offset="4" BitOffset="0" Text="bis Offset" SuffixText="{2}" Value="0" op:nowarn="true" />' -f `
            (UpId (OffsetToId $p.key $l.key)), $tid, $l.unitText)
    }
}
AddS '              </Union>'
AddS '            </Parameters>'
AddS ''

$unionIds = @($slotIds.ValueType, $slotIds.Aggregation, $slotIds.Send, $slotIds.Shadow)
foreach ($p in $catalog.providers)
{
    $unionIds += (CategoryId $p.key)
    foreach ($c in Get-ProviderCategories $p.key) { $unionIds += (MeasurandId $p.key $c.key) }
    foreach ($l in Get-ProviderLevels $p.key)
    {
        if ($l.key -eq 'current') { continue }
        $unionIds += (OffsetFromId $p.key $l.key); $unionIds += (OffsetToId $p.key $l.key)
    }
}
AddS '            <ParameterRefs>'
AddS ('              <ParameterRef Id="{0}" RefId="%AID%_P-%TT%%CC%%SPP+{1}%" />' -f (PRef $slotIds.Label), $slotIds.Label)
foreach ($n in ($unionIds | Sort-Object)) { AddS ('              <ParameterRef Id="{0}" RefId="{1}" />' -f (UpRef $n), (UpId $n)) }
AddS '            </ParameterRefs>'
AddS ''

AddS '            <ComObjectTable>'
AddS '              <!-- Basisgröße ist die größte Variante; die Refs überschreiben sie nach unten -->'
AddS '              <ComObject Id="%AID%_O-%TT%%CC%%SPP+0%" Number="%K%SK%%" Name="CH%C%Slot%SL%" Text="Wert %SL%" FunctionText="Wetter %C% %SL%: Ausgang" ObjectSize="14 Bytes" DatapointType="DPST-16-1" ReadFlag="Enabled" WriteFlag="Disabled" CommunicationFlag="Enabled" TransmitFlag="Enabled" UpdateFlag="Disabled" ReadOnInitFlag="Disabled" />'
AddS '            </ComObjectTable>'
AddS ''
AddS '            <ComObjectRefs>'
$dptRefIndex = @{}
for ($i = 0; $i -lt $dptOrder.Count; $i++)
{
    $key = $dptOrder[$i]
    $dpt, $size = $key -split '\|'
    $dptRefIndex[$key] = $i + 1
    $sample = $byDpt[$key][0]
    AddS ('              <ComObjectRef Id="%AID%_O-%TT%%CC%%SPP+0%_R-%TT%%CC%%SPP+0%{0:00}" RefId="%AID%_O-%TT%%CC%%SPP+0%" ObjectSize="{1}" DatapointType="{2}" Text="{{{{0:Wert %SL%}}}}" FunctionText="Wetter %C% %SL%: Ausgang, {3}" TextParameterRefId="{4}" />' -f `
        ($i + 1), $size, $dpt, (Protect-Xml $sample.label), (PRef $slotIds.Label))
}
AddS '            </ComObjectRefs>'
AddS '          </Static>'
AddS ''

AddS '          <Dynamic>'
AddS '            <ChannelIndependentBlock>'
AddS '              <ParameterBlock Id="%AID%_PB-nnn" Name="Slot">'
AddS ('                <ParameterBlock Id="%AID%_PB-nnn" Name="f%CC%IWSlot%SL%" Text="Wetter %SL%: {{{{0: ...}}}}" TextParameterRefId="{0}" Icon="gauge" ShowInComObjectTree="true" HelpContext="IW-SlotBezeichnung">' -f (PRef $slotIds.Label))
AddS ('                  <ParameterRefRef RefId="{0}" HelpContext="IW-SlotBezeichnung" />' -f (PRef $slotIds.Label))
AddS '                  <!-- Schattenparameter muss im Dynamic mindestens einmal referenziert sein -->'
AddS ('                  <ParameterRefRef RefId="{0}" />' -f (UpRef $slotIds.Shadow))
AddS ''
AddS '                  <!-- Der Anbieter bestimmt Kategorie-, Messwert- und Offset-Auswahl -->'
AddS '                  <choose ParamRefId="%AID%_UP-%TT%%CC%001_R-%TT%%CC%00101">'
foreach ($p in $catalog.providers)
{
    AddS ('                    <when test="{0}">' -f $p.value)
    AddS ('                      <ParameterRefRef RefId="{0}" HelpContext="IW-SlotKategorie" />' -f (UpRef (CategoryId $p.key)))
    AddS ('                      <choose ParamRefId="{0}">' -f (UpRef (CategoryId $p.key)))
    foreach ($c in Get-ProviderCategories $p.key)
    {
        $mid = MeasurandId $p.key $c.key
        AddS ('                        <when test="{0}">' -f $c.value)
        AddS ('                          <ParameterRefRef RefId="{0}" HelpContext="IW-SlotMesswert" />' -f (UpRef $mid))
        AddS ('                          <Assign TargetParamRefRef="{0}" SourceParamRefRef="{1}" />' -f (UpRef $slotIds.Shadow), (UpRef $mid))
        AddS '                        </when>'
    }
    AddS '                      </choose>'
    AddS ''
    AddS ('                      <choose ParamRefId="{0}">' -f (UpRef $slotIds.Shadow))
    foreach ($l in Get-ProviderLevels $p.key)
    {
        $ids = @($byProvLvl["$($p.key)|$($l.key)"] | ForEach-Object { $_.id })
        if ($l.key -eq 'current')
        {
            AddS '                        <!-- Aktuell: weder Offset noch Typ -->'
            AddS ('                        <when test="{0}" />' -f (Format-TestList $ids))
            continue
        }
        $nonAgg = @($byProvLvl["$($p.key)|$($l.key)"] | Where-Object { -not $_.aggregatable } | ForEach-Object { $_.id })
        $offFrom = UpRef (OffsetFromId $p.key $l.key)
        $offTo   = UpRef (OffsetToId $p.key $l.key)
        AddS ('                        <when test="{0}">' -f (Format-TestList $ids))
        if ($nonAgg.Count -gt 0)
        {
            AddS ('                          <choose ParamRefId="{0}">' -f (UpRef $slotIds.Shadow))
            AddS ('                            <when test="{0}">' -f (Format-TestList $nonAgg))
            AddS ('                              <ParameterRefRef RefId="{0}" HelpContext="IW-SlotOffset" />' -f $offFrom)
            AddS '                            </when>'
            AddS '                            <when default="true">'
            $ind = '                              '
        }
        else { $ind = '                          ' }
        AddS ($ind + ('<ParameterRefRef RefId="{0}" HelpContext="IW-SlotTyp" />' -f (UpRef $slotIds.ValueType)))
        AddS ($ind + ('<ParameterRefRef RefId="{0}" HelpContext="IW-SlotOffset" />' -f $offFrom))
        AddS ($ind + ('<choose ParamRefId="{0}">' -f (UpRef $slotIds.ValueType)))
        AddS ($ind + '  <when test="1">')
        AddS ($ind + ('    <ParameterRefRef RefId="{0}" HelpContext="IW-SlotOffsetBis" />' -f $offTo))
        AddS ($ind + ('    <ParameterRefRef RefId="{0}" HelpContext="IW-SlotAggregation" />' -f (UpRef $slotIds.Aggregation)))
        AddS ($ind + '  </when>')
        AddS ($ind + '</choose>')
        if ($nonAgg.Count -gt 0)
        {
            AddS '                            </when>'
            AddS '                          </choose>'
        }
        AddS '                        </when>'
    }
    AddS '                      </choose>'
    AddS '                    </when>'
}
AddS '                  </choose>'
AddS ''
AddS '                  <!-- Sendeverhalten und passendes Kommunikationsobjekt, anbieterunabhängig -->'
AddS ('                  <choose ParamRefId="{0}">' -f (UpRef $slotIds.Shadow))
AddS '                    <when test="0" />'
AddS '                    <when default="true">'
AddS ('                      <ParameterRefRef RefId="{0}" HelpContext="IW-SlotSenden" />' -f (UpRef $slotIds.Send))
AddS '                    </when>'
AddS '                  </choose>'
AddS ('                  <choose ParamRefId="{0}">' -f (UpRef $slotIds.Shadow))
foreach ($key in $dptOrder)
{
    $ids = @($byDpt[$key] | ForEach-Object { $_.id })
    $dpt, $size = $key -split '\|'
    AddS ('                    <when test="{0}">' -f (Format-TestList $ids))
    AddS ('                      <ComObjectRefRef RefId="%AID%_O-%TT%%CC%%SPP+0%_R-%TT%%CC%%SPP+0%{0:00}" />   <!-- {1} -->' -f $dptRefIndex[$key], $dpt)
    AddS '                    </when>'
}
AddS '                  </choose>'
AddS '                </ParameterBlock>'
AddS '              </ParameterBlock>'
AddS '            </ChannelIndependentBlock>'
AddS '          </Dynamic>'
AddS '        </ApplicationProgram>'
AddS '      </ApplicationPrograms>'
AddS '    </Manufacturer>'
AddS '  </ManufacturerData>'
AddS '</KNX>'

Set-Content -Path $slotPath -Value $s.ToString() -Encoding UTF8 -NoNewline
Write-Host "geschrieben: src/InternetWeatherModule.slot.part.xml"

# ---------------------------------------------------------------- 3) WeatherCatalog.gen.h

$h = [System.Text.StringBuilder]::new()
function AddH([string] $line) { [void]$h.Append($line).Append($nl) }

AddH '// ERZEUGT von tools/Generate-Catalog.ps1 aus src/weather-catalog.json - nicht von Hand ändern.'
AddH '//'
AddH '// Der Messwert-Enum (enum class PT_Measurand) kommt vom OpenKNXproducer aus der'
AddH '// share.xml (op:headerExport="enum") und steht in knxprod.h. Diese Datei ergänzt'
AddH '// ihn um die Metadaten, die die ETS nicht kennt.'
AddH '#pragma once'
AddH '#include <stdint.h>'
AddH ''
AddH ('#define IW_MEASURAND_COUNT ' + $catalog.measurements.Count)
AddH ''
AddH 'enum class WeatherLevel : uint8_t'
AddH '{'
foreach ($l in $catalog.levels) { AddH ('    {0},' -f $levelEnum[$l.key]) }
AddH '};'
AddH ''
AddH 'enum class WeatherDpt : uint8_t'
AddH '{'
foreach ($key in $dptOrder) { $dpt, $size = $key -split '\|'; AddH ('    {0},' -f (ConvertTo-DptIdentifier $dpt)) }
AddH '};'
AddH ''
AddH '// Rohwert * scale = KNX-Wert. var == nullptr: Anbieter liefert diese Größe nicht.'
AddH 'struct WeatherProviderVar'
AddH '{'
AddH '    const char* var;'
AddH '    float       scale;'
AddH '};'
AddH ''
AddH 'struct WeatherMeasurandInfo'
AddH '{'
AddH '    uint8_t            id;            // = PT_Measurand'
foreach ($p in $catalog.providers)
{
    $fieldId = ConvertTo-Identifier $p.key
    $field = $fieldId.Substring(0,1).ToLower() + $fieldId.Substring(1)
    AddH ('    WeatherProviderVar {0,-16} // {1}' -f ($field + ';'), $p.label)
}
AddH '    WeatherLevel       level;'
AddH '    WeatherDpt         dpt;'
AddH '    bool               aggregatable;'
AddH '};'
AddH ''
AddH '// Nach id aufsteigend sortiert - binäre Suche zulässig.'
AddH 'static const WeatherMeasurandInfo IW_MEASURANDS[IW_MEASURAND_COUNT] ='
AddH '{'
foreach ($m in ($catalog.measurements | Sort-Object id))
{
    $cells = @()
    foreach ($p in $catalog.providers)
    {
        $pv = Get-ProviderVar $m $p.key
        if ($null -eq $pv) { $cells += '{ nullptr, 1.0f },' }
        else { $cells += ('{{ {0}, {1}f }},' -f ('"' + $pv.var + '"'), $pv.scale.ToString('0.0##', [cultureinfo]::InvariantCulture)) }
    }
    AddH ('    {{ {0,3}, {1,-34} {2,-34} WeatherLevel::{3,-12} WeatherDpt::{4,-11} {5} }},   // {6}' -f `
        $m.id, $cells[0], $cells[1], ($levelEnum[$m.level] + ','), ((ConvertTo-DptIdentifier $m.dpt) + ','),
        $(if ($m.aggregatable) { 'true ' } else { 'false' }), (Get-HeaderName $m))
}
AddH '};'

Set-Content -Path $headerPath -Value $h.ToString() -Encoding UTF8 -NoNewline
Write-Host "geschrieben: src/WeatherCatalog.gen.h"
Write-Host ""
Write-Host "Fertig."

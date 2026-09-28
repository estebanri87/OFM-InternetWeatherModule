# Erzeugt aus src/weather-catalog.json die vom Katalog abhaengigen Artefakte:
#
#   src/InternetWeatherModule.share.xml       Region zwischen den CATALOG-Markern
#   src/InternetWeatherModule.slot.part.xml   vollstaendig (op:part-Fragment, 3x expandiert)
#   src/WeatherCatalog.gen.h                  C++-Metadaten (Variable, Ebene, DPT, Skalierung)
#
# Der Messwert-Enum selbst wird NICHT hier erzeugt, sondern vom OpenKNXproducer
# aus op:headerExport="enum" - siehe knxprod.h, enum class PT_Measurand.
#
#   pwsh -File tools/Generate-Catalog.ps1

param(
    [string] $Root = "$PSScriptRoot/.."
)

$ErrorActionPreference = 'Stop'

$catalogPath = Join-Path $Root 'src/weather-catalog.json'
$sharePath   = Join-Path $Root 'src/InternetWeatherModule.share.xml'
$slotPath    = Join-Path $Root 'src/InternetWeatherModule.slot.part.xml'
$headerPath  = Join-Path $Root 'src/WeatherCatalog.gen.h'

$catalog = Get-Content $catalogPath -Raw -Encoding UTF8 | ConvertFrom-Json

$levelSuffix = @{ current = 'Cur'; minutely_15 = 'Q15'; hourly = 'Hour'; daily = 'Day' }
$levelEnum   = @{ current = 'Current'; minutely_15 = 'Minutely15'; hourly = 'Hourly'; daily = 'Daily' }

# ---------------------------------------------------------------- Hilfsfunktionen

function ConvertTo-Identifier([string] $text)
{
    $t = $text
    foreach ($p in @(@('ä','ae'), @('ö','oe'), @('ü','ue'), @('Ä','Ae'), @('Ö','Oe'), @('Ü','Ue'), @('ß','ss')))
    {
        $t = $t.Replace($p[0], $p[1])
    }
    # Woerter an Nicht-Alphanumerischem trennen und in PascalCase zusammenziehen
    $parts = [regex]::Split($t, '[^A-Za-z0-9]+') | Where-Object { $_ -ne '' }
    -join ($parts | ForEach-Object {
        if ($_ -cmatch '^[A-Z0-9]+$') { $_ } else { $_.Substring(0,1).ToUpper() + $_.Substring(1) }
    })
}

function ConvertTo-DptIdentifier([string] $dpt)
{
    # "DPST-9-26" -> "Dpt9_26"; eindeutig, anders als ein blosses Entfernen der Bindestriche
    if ($dpt -match '^DPST-(\d+)-(\d+)$') { "Dpt$($Matches[1])_$($Matches[2])" }
    else { throw "Unerwartetes DPT-Format: $dpt" }
}

function Get-DisplayText($m)
{
    $suffix = ($catalog.levels | Where-Object key -eq $m.level).suffix
    "$($m.label) - $suffix"
}

function Get-HeaderName($m)
{
    (ConvertTo-Identifier $m.label) + $levelSuffix[$m.level]
}

function Format-TestList([int[]] $values)
{
    ($values | Sort-Object) -join ' '
}

function Protect-Xml([string] $s)
{
    $s.Replace('&', '&amp;').Replace('<', '&lt;').Replace('>', '&gt;')
}

# ---------------------------------------------------------------- Konsistenzpruefung

$names = @{}
foreach ($m in $catalog.measurements)
{
    $n = Get-HeaderName $m
    if ($names.ContainsKey($n)) { throw "Doppelter headerName '$n' (IDs $($names[$n]) und $($m.id))" }
    $names[$n] = $m.id

    $expected = [math]::Floor($m.id / 16)
    $actual   = ($catalog.categories | Where-Object key -eq $m.category).value
    if ($expected -ne $actual) { throw "ID $($m.id) passt nicht zur Kategorie '$($m.category)' (High-Nibble $expected, erwartet $actual)" }

    if (-not $levelSuffix.ContainsKey($m.level)) { throw "ID $($m.id): unbekannte Zeitebene '$($m.level)'" }
}
Write-Host ("Katalog geprueft: {0} Messwerte, {1} eindeutige headerNames" -f $catalog.measurements.Count, $names.Count)

# ---------------------------------------------------------------- Wertelisten

# Je Kategorie
$byCategory = @{}
foreach ($c in $catalog.categories)
{
    $byCategory[$c.key] = @($catalog.measurements | Where-Object category -eq $c.key)
}

# Je Zeitebene
$byLevel = @{}
foreach ($l in $catalog.levels)
{
    $byLevel[$l.key] = @($catalog.measurements | Where-Object level -eq $l.key)
}

# Je DPT-Klasse (Reihenfolge stabil ueber die erste Verwendung)
$dptOrder = @()
$byDpt = @{}
foreach ($m in $catalog.measurements)
{
    $key = "$($m.dpt)|$($m.objectSize)"
    if (-not $byDpt.ContainsKey($key)) { $byDpt[$key] = @(); $dptOrder += $key }
    $byDpt[$key] += $m
}
Write-Host ("DPT-Klassen: {0}" -f $dptOrder.Count)

# ---------------------------------------------------------------- 1) share.xml-Region

$sb = [System.Text.StringBuilder]::new()
$nl = "`r`n"
function Add([string] $line) { [void]$sb.Append($line).Append($nl) }

Add '              <!-- BEGIN GENERATED CATALOG - erzeugt von tools/Generate-Catalog.ps1 aus src/weather-catalog.json, nicht von Hand aendern -->'

Add '              <ParameterType Id="%AID%_PT-IWCategory" Name="IWCategory" op:headerExport="enum" op:headerName="Category">'
Add '                <TypeRestriction Base="Value" SizeInBit="4" UIHint="DropDown">'
Add '                  <Enumeration Text="Bitte waehlen..." Value="0" Id="%ENID%" op:headerName="None" />'
foreach ($c in $catalog.categories)
{
    Add ('                  <Enumeration Text="{0}" Value="{1}" Id="%ENID%" op:headerName="{2}" Icon="{3}" />' -f `
        (Protect-Xml $c.label), $c.value, (ConvertTo-Identifier $c.key), $c.icon)
}
Add '                </TypeRestriction>'
Add '              </ParameterType>'
Add ''

# Vollstaendiger Messwert-Enum: nur dieser exportiert den C++-Enum.
Add '              <!-- Schattenparameter-Typ: einziger choose-Treiber, erzeugt enum class PT_Measurand -->'
Add '              <ParameterType Id="%AID%_PT-IWMeasurandAll" Name="IWMeasurandAll" op:headerExport="enum" op:headerName="Measurand">'
Add '                <TypeRestriction Base="Value" SizeInBit="8" UIHint="DropDown">'
Add '                  <Enumeration Text="Bitte waehlen..." Value="0" Id="%ENID%" op:headerName="None" />'
foreach ($m in $catalog.measurements)
{
    Add ('                  <Enumeration Text="{0}" Value="{1}" Id="%ENID%" op:headerName="{2}" />' -f `
        (Protect-Xml (Get-DisplayText $m)), $m.id, (Get-HeaderName $m))
}
Add '                </TypeRestriction>'
Add '              </ParameterType>'
Add ''

# Kategoriegefilterte Sichten: gleiche SizeInBit, headerExport="base" -> kein zweiter Enum.
foreach ($c in $catalog.categories)
{
    $id = ConvertTo-Identifier $c.key
    Add ('              <ParameterType Id="%AID%_PT-IWMeasurand{0}" Name="IWMeasurand{0}" op:headerExport="base" op:headerName="Measurand">' -f $id)
    Add '                <TypeRestriction Base="Value" SizeInBit="8" UIHint="DropDown">'
    Add '                  <Enumeration Text="Bitte waehlen..." Value="0" Id="%ENID%" />'
    foreach ($m in $byCategory[$c.key])
    {
        Add ('                  <Enumeration Text="{0}" Value="{1}" Id="%ENID%" />' -f (Protect-Xml (Get-DisplayText $m)), $m.id)
    }
    Add '                </TypeRestriction>'
    Add '              </ParameterType>'
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
    Add ('                  <Enumeration Text="{0}" Value="{1}" Id="%ENID%" op:headerName="{2}" />' -f `
        (Protect-Xml $a.label), $a.value, (ConvertTo-Identifier $a.key))
}
Add '                </TypeRestriction>'
Add '              </ParameterType>'
Add ''

Add '              <ParameterType Id="%AID%_PT-IWSendBehaviour" Name="IWSendBehaviour" op:headerExport="enum" op:headerName="SendBehaviour">'
Add '                <TypeRestriction Base="Value" SizeInBit="1">'
Add '                  <Enumeration Text="Nur bei Aenderung" Value="0" Id="%ENID%" op:headerName="OnChange" />'
Add '                  <Enumeration Text="Bei jedem Abruf" Value="1" Id="%ENID%" op:headerName="Always" />'
Add '                </TypeRestriction>'
Add '              </ParameterType>'
Add ''

foreach ($l in $catalog.levels)
{
    if ($l.key -eq 'current') { continue }
    Add ('              <ParameterType Id="%AID%_PT-IWOffset{0}" Name="IWOffset{0}">' -f $levelSuffix[$l.key])
    Add ('                <TypeNumber SizeInBit="16" Type="signedInt" minInclusive="{0}" maxInclusive="{1}" />' -f $l.offsetMin, $l.offsetMax)
    Add '              </ParameterType>'
}

Add '              <!-- END GENERATED CATALOG -->'

$region = $sb.ToString().TrimEnd()

# share.xml ist tab-eingerueckt - erzeugte Region angleichen (2 Leerzeichen = 1 Tab)
$region = ($region -split "`r`n" | ForEach-Object {
    if ($_ -match '^( +)(.*)$') { ("`t" * [int]($Matches[1].Length / 2)) + $Matches[2] } else { $_ }
}) -join "`r`n"

$share = Get-Content $sharePath -Raw -Encoding UTF8
$beginRx = '(?s)[ \t]*<!-- BEGIN GENERATED CATALOG.*?<!-- END GENERATED CATALOG -->'
if ($share -notmatch $beginRx) { throw "In $sharePath fehlen die CATALOG-Marker. Bitte einmalig einfuegen." }
$share = [regex]::Replace($share, $beginRx, { $region }, 1)
Set-Content -Path $sharePath -Value $share -Encoding UTF8 -NoNewline
Write-Host "geschrieben: src/InternetWeatherModule.share.xml (Katalog-Region)"

# ---------------------------------------------------------------- 2) slot.part.xml

# Parameter-ID-Vergabe innerhalb des Slot-Fragments, relativ zu %SPP%
$P = @{
    Label = 0; Category = 1; ValueType = 2; Aggregation = 3; Send = 4
    MeasurandBase = 5          # +5 .. +12  (8 Kategorien)
    Shadow = 13
    OffsetFromBase = 14        # +14 Day, +15 Hour, +16 Q15
    OffsetToBase = 17          # +17 Day, +18 Hour, +19 Q15
}
$catIndex = @{}
for ($i = 0; $i -lt $catalog.categories.Count; $i++) { $catIndex[$catalog.categories[$i].key] = $i }

$offsetLevels = @('daily', 'hourly', 'minutely_15')   # Reihenfolge = Offset der IDs

$s = [System.Text.StringBuilder]::new()
function AddS([string] $line) { [void]$s.Append($line).Append($nl) }

function RefOf([int] $n) { '%AID%_UP-%TT%%CC%%SPP+' + $n + '%_R-%TT%%CC%%SPP+' + $n + '%01' }
function PRefOf([int] $n) { '%AID%_P-%TT%%CC%%SPP+' + $n + '%_R-%TT%%CC%%SPP+' + $n + '%01' }

AddS '<?xml version="1.0" encoding="utf-8"?>'
AddS '<KNX xmlns:op="http://github.com/OpenKNX/OpenKNXproducer" xmlns="http://knx.org/xml/project/20" CreatedBy="KNX MT" ToolVersion="5.1.255.16695">'
AddS '  <ManufacturerData>'
AddS '    <Manufacturer RefId="M-00FA">'
AddS '      <ApplicationPrograms>'
AddS '        <ApplicationProgram>'
AddS '          <!-- ERZEUGT von tools/Generate-Catalog.ps1 aus src/weather-catalog.json - nicht von Hand aendern. -->'
AddS '          <!-- Ein Wetter-Wert-Slot. Wird per op:part dreimal expandiert (Wetter A/B/C). -->'
AddS '          <Static>'

# ---- Parameters
AddS '            <Parameters>'
AddS '              <!-- Bezeichnung: kein Union, keine Memory -> nur in der ETS, nicht auf dem Geraet -->'
AddS ('              <Parameter Id="%AID%_P-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%Label" ParameterType="%AID%_PT-Text40Byte" Text="Bezeichnung" Value="" />' -f $P.Label)
AddS ''
AddS '              <Union SizeInBit="48">'
AddS '                <Memory CodeSegment="%MID%" Offset="%SO%" BitOffset="0" />'
AddS ('                <Parameter Id="%AID%_UP-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%Category"    ParameterType="%AID%_PT-IWCategory"      Offset="0" BitOffset="0" Text="Kategorie" Value="0" />' -f $P.Category)
AddS ('                <Parameter Id="%AID%_UP-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%ValueType"   ParameterType="%AID%_PT-IWSlotValueType" Offset="0" BitOffset="4" Text="Typ" Value="0" />' -f $P.ValueType)
AddS ('                <Parameter Id="%AID%_UP-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%Aggregation" ParameterType="%AID%_PT-IWAggregation"   Offset="0" BitOffset="5" Text="Aggregation" Value="0" />' -f $P.Aggregation)
AddS ('                <Parameter Id="%AID%_UP-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%Send"        ParameterType="%AID%_PT-IWSendBehaviour" Offset="0" BitOffset="7" Text="Senden" Value="0" />' -f $P.Send)
AddS ''
AddS '                <!-- Byte +1: acht kategoriegefilterte Sichten auf dieselbe Speicherstelle -->'
foreach ($c in $catalog.categories)
{
    $n = $P.MeasurandBase + $catIndex[$c.key]
    AddS ('                <Parameter Id="%AID%_UP-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%Measurand{1}" ParameterType="%AID%_PT-IWMeasurand{1}" Offset="1" BitOffset="0" Text="Messwert" Value="0" op:nowarn="true" />' -f $n, (ConvertTo-Identifier $c.key))
}
AddS ''
AddS '                <!-- Schattenparameter: unsichtbar, per Assign gefuellt, einziger choose-Treiber -->'
AddS ('                <Parameter Id="%AID%_UP-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%Measurand" ParameterType="%AID%_PT-IWMeasurandAll" Offset="1" BitOffset="0" Text="Messwert" Value="0" Access="None" op:nowarn="true" />' -f $P.Shadow)
AddS ''
AddS '                <!-- Byte +2/+3 und +4/+5: je drei Einheitenvarianten auf derselben Speicherstelle -->'
for ($i = 0; $i -lt $offsetLevels.Count; $i++)
{
    $lk = $offsetLevels[$i]
    $l  = $catalog.levels | Where-Object key -eq $lk
    AddS ('                <Parameter Id="%AID%_UP-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%OffsetFrom{1}" ParameterType="%AID%_PT-IWOffset{1}" Offset="2" BitOffset="0" Text="Offset" SuffixText="{2}" Value="0" op:nowarn="true" />' -f ($P.OffsetFromBase + $i), $levelSuffix[$lk], $l.unitText)
}
for ($i = 0; $i -lt $offsetLevels.Count; $i++)
{
    $lk = $offsetLevels[$i]
    $l  = $catalog.levels | Where-Object key -eq $lk
    AddS ('                <Parameter Id="%AID%_UP-%TT%%CC%%SPP+{0}%" Name="CH%C%Slot%SL%OffsetTo{1}"   ParameterType="%AID%_PT-IWOffset{1}" Offset="4" BitOffset="0" Text="bis Offset" SuffixText="{2}" Value="0" op:nowarn="true" />' -f ($P.OffsetToBase + $i), $levelSuffix[$lk], $l.unitText)
}
AddS '              </Union>'
AddS '            </Parameters>'
AddS ''

# ---- ParameterRefs
AddS '            <ParameterRefs>'
AddS ('              <ParameterRef Id="%AID%_P-%TT%%CC%%SPP+{0}%_R-%TT%%CC%%SPP+{0}%01" RefId="%AID%_P-%TT%%CC%%SPP+{0}%" />' -f $P.Label)
$unionIds = @($P.Category, $P.ValueType, $P.Aggregation, $P.Send, $P.Shadow)
foreach ($c in $catalog.categories) { $unionIds += ($P.MeasurandBase + $catIndex[$c.key]) }
for ($i = 0; $i -lt $offsetLevels.Count; $i++) { $unionIds += ($P.OffsetFromBase + $i); $unionIds += ($P.OffsetToBase + $i) }
foreach ($n in ($unionIds | Sort-Object))
{
    AddS ('              <ParameterRef Id="%AID%_UP-%TT%%CC%%SPP+{0}%_R-%TT%%CC%%SPP+{0}%01" RefId="%AID%_UP-%TT%%CC%%SPP+{0}%" />' -f $n)
}
AddS '            </ParameterRefs>'
AddS ''

# ---- ComObjectTable: ein KO je Slot, Basisgroesse = groesste Variante
AddS '            <ComObjectTable>'
AddS '              <ComObject Id="%AID%_O-%TT%%CC%%SPP+0%" Number="%K%SK%%" Name="CH%C%Slot%SL%" Text="Wert %SL%" FunctionText="Wetter %C% %SL%: Ausgang" ObjectSize="14 Bytes" DatapointType="DPST-16-1" ReadFlag="Enabled" WriteFlag="Disabled" CommunicationFlag="Enabled" TransmitFlag="Enabled" UpdateFlag="Disabled" ReadOnInitFlag="Disabled" />'
AddS '            </ComObjectTable>'
AddS ''

# ---- ComObjectRefs: je DPT-Klasse eine Variante
AddS '            <ComObjectRefs>'
$dptRefIndex = @{}
for ($i = 0; $i -lt $dptOrder.Count; $i++)
{
    $key = $dptOrder[$i]
    $dpt, $size = $key -split '\|'
    $dptRefIndex[$key] = $i + 1
    $sample = $byDpt[$key][0]
    AddS ('              <ComObjectRef Id="%AID%_O-%TT%%CC%%SPP+0%_R-%TT%%CC%%SPP+0%{0:00}" RefId="%AID%_O-%TT%%CC%%SPP+0%" ObjectSize="{1}" DatapointType="{2}" Text="{{{{0:Wert %SL%}}}}" FunctionText="Wetter %C% %SL%: Ausgang, {3}" TextParameterRefId="{4}" />' -f `
        ($i + 1), $size, $dpt, (Protect-Xml $sample.label), (PRefOf $P.Label))
}
AddS '            </ComObjectRefs>'
AddS '          </Static>'
AddS ''

# ---- Dynamic
AddS '          <Dynamic>'
AddS '            <ChannelIndependentBlock>'
AddS '              <ParameterBlock Id="%AID%_PB-nnn" Name="Slot">'
AddS ('                <ParameterBlock Id="%AID%_PB-nnn" Name="f%CC%IWSlot%SL%" Text="Wetter %SL%: {{{{0: ...}}}}" TextParameterRefId="{0}" Icon="gauge" ShowInComObjectTree="true" HelpContext="IW-SlotLabel">' -f (PRefOf $P.Label))
AddS ('                  <ParameterRefRef RefId="{0}" HelpContext="IW-SlotLabel" />' -f (PRefOf $P.Label))
AddS ('                  <ParameterRefRef RefId="{0}" HelpContext="IW-SlotKategorie" />' -f (RefOf $P.Category))
AddS '                  <!-- Schattenparameter muss im Dynamic mindestens einmal referenziert sein -->'
AddS ('                  <ParameterRefRef RefId="{0}" />' -f (RefOf $P.Shadow))
AddS ''
AddS '                  <!-- (1) Kategorie blendet die passende Messwert-Sicht ein und kopiert sie in den Schattenparameter -->'
AddS ('                  <choose ParamRefId="{0}">' -f (RefOf $P.Category))
foreach ($c in $catalog.categories)
{
    $n = $P.MeasurandBase + $catIndex[$c.key]
    AddS ('                    <when test="{0}">' -f $c.value)
    AddS ('                      <ParameterRefRef RefId="{0}" HelpContext="IW-SlotMesswert" />' -f (RefOf $n))
    AddS ('                      <Assign TargetParamRefRef="{0}" SourceParamRefRef="{1}" />' -f (RefOf $P.Shadow), (RefOf $n))
    AddS '                    </when>'
}
AddS '                  </choose>'
AddS ''
AddS '                  <!-- (2) Offset-Einheit je Zeitebene; Typ/Aggregation nur bei aggregierbaren Groessen -->'
AddS ('                  <choose ParamRefId="{0}">' -f (RefOf $P.Shadow))
# Aktuell: weder Offset noch Typ
$curIds = @($byLevel['current'] | ForEach-Object { $_.id })
AddS ('                    <when test="{0}" />' -f (Format-TestList $curIds))
for ($i = 0; $i -lt $offsetLevels.Count; $i++)
{
    $lk = $offsetLevels[$i]
    $ids = @($byLevel[$lk] | ForEach-Object { $_.id })
    $nonAgg = @($byLevel[$lk] | Where-Object { -not $_.aggregatable } | ForEach-Object { $_.id })
    AddS ('                    <when test="{0}">' -f (Format-TestList $ids))
    AddS ('                      <ParameterRefRef RefId="{0}" HelpContext="IW-SlotOffset" />' -f (RefOf ($P.OffsetFromBase + $i)))
    if ($nonAgg.Count -gt 0)
    {
        AddS ('                      <choose ParamRefId="{0}">' -f (RefOf $P.Shadow))
        AddS ('                        <when test="{0}" />' -f (Format-TestList $nonAgg))
        AddS '                        <when default="true">'
        $ind = '                          '
    }
    else
    {
        $ind = '                      '
    }
    AddS ($ind + ('<ParameterRefRef RefId="{0}" HelpContext="IW-SlotTyp" />' -f (RefOf $P.ValueType)))
    AddS ($ind + ('<choose ParamRefId="{0}">' -f (RefOf $P.ValueType)))
    AddS ($ind + '  <when test="1">')
    AddS ($ind + ('    <ParameterRefRef RefId="{0}" HelpContext="IW-SlotOffsetBis" />' -f (RefOf ($P.OffsetToBase + $i))))
    AddS ($ind + ('    <ParameterRefRef RefId="{0}" HelpContext="IW-SlotAggregation" />' -f (RefOf $P.Aggregation)))
    AddS ($ind + '  </when>')
    AddS ($ind + '</choose>')
    if ($nonAgg.Count -gt 0)
    {
        AddS '                        </when>'
        AddS '                      </choose>'
    }
    AddS '                    </when>'
}
AddS '                  </choose>'
AddS ''
AddS '                  <!-- (3) Sendeverhalten und das zum Messwert passende Kommunikationsobjekt -->'
AddS ('                  <choose ParamRefId="{0}">' -f (RefOf $P.Shadow))
AddS '                    <when test="0" />'
AddS '                    <when default="true">'
AddS ('                      <ParameterRefRef RefId="{0}" HelpContext="IW-SlotSenden" />' -f (RefOf $P.Send))
AddS '                    </when>'
AddS '                  </choose>'
AddS ('                  <choose ParamRefId="{0}">' -f (RefOf $P.Shadow))
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

AddH '// ERZEUGT von tools/Generate-Catalog.ps1 aus src/weather-catalog.json - nicht von Hand aendern.'
AddH '//'
AddH '// Der Messwert-Enum selbst (enum class PT_Measurand) kommt vom OpenKNXproducer'
AddH '// aus der share.xml (op:headerExport="enum") und steht in knxprod.h.'
AddH '// Diese Datei ergaenzt ihn um die Metadaten, die die ETS nicht kennt.'
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
foreach ($key in $dptOrder)
{
    $dpt, $size = $key -split '\|'
    AddH ('    {0},' -f (ConvertTo-DptIdentifier $dpt))
}
AddH '};'
AddH ''
AddH 'struct WeatherMeasurandInfo'
AddH '{'
AddH '    uint8_t      id;            // = PT_Measurand'
AddH '    const char*  omVariable;    // Open-Meteo-Variable, nullptr bei abgeleiteten Werten'
AddH '    WeatherLevel level;'
AddH '    WeatherDpt   dpt;'
AddH '    float        scale;         // Rohwert * scale = KNX-Wert'
AddH '    bool         aggregatable;'
AddH '};'
AddH ''
AddH 'static const WeatherMeasurandInfo IW_MEASURANDS[IW_MEASURAND_COUNT] ='
AddH '{'
foreach ($m in $catalog.measurements)
{
    $var = if ($m.omVariable.StartsWith('@')) { 'nullptr' } else { '"' + $m.omVariable + '"' }
    $agg = if ($m.aggregatable) { 'true ' } else { 'false' }
    $scale = $m.scale.ToString('0.0##', [cultureinfo]::InvariantCulture) + 'f'
    AddH ('    {{ {0,3}, {1,-34} WeatherLevel::{2,-12} WeatherDpt::{3,-11} {4,7}, {5} }},   // {6}' -f `
        $m.id, ($var + ','), ($levelEnum[$m.level] + ','), ((ConvertTo-DptIdentifier $m.dpt) + ','), $scale, $agg, (Get-HeaderName $m))
}
AddH '};'

Set-Content -Path $headerPath -Value $h.ToString() -Encoding UTF8 -NoNewline
Write-Host "geschrieben: src/WeatherCatalog.gen.h"
Write-Host ""
Write-Host "Fertig."

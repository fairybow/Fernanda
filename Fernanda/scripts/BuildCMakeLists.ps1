<#
.SYNOPSIS
    Generates a CMakeLists.txt from a Visual Studio .vcxproj file (Qt project).
    This will for sure break easily if we add any other submodules or libraries.

.DESCRIPTION
    Parses a .vcxproj file created by Qt VS Tools and generates an equivalent
    CMakeLists.txt. Designed for the Fernanda project but handles most of the
    parsing generically.

.PARAMETER VcxprojPath
    Path to the .vcxproj file. Defaults to finding one in the current directory.

.EXAMPLE
    .\BuildCMakeLists.ps1
    .\BuildCMakeLists.ps1 -VcxprojPath .\Fernanda.vcxproj
#>

param(
    [string]$VcxprojPath
)

# ============================================================================
# Setup
# ============================================================================

$ErrorActionPreference = "Stop"

if (-not $VcxprojPath) {
    $found = Get-ChildItem -Path . -Filter "*.vcxproj" -File | Select-Object -First 1
    if (-not $found) {
        Write-Error "No .vcxproj file found in current directory. Use -VcxprojPath."
        exit 1
    }
    $VcxprojPath = $found.FullName
}

if (-not (Test-Path $VcxprojPath)) {
    Write-Error "File not found: $VcxprojPath"
    exit 1
}

$projectDir = Split-Path -Parent (Resolve-Path $VcxprojPath)
$projectName = [System.IO.Path]::GetFileNameWithoutExtension($VcxprojPath)
$projectNameUpper = $projectName.ToUpper()

Write-Host "Parsing: $VcxprojPath"
Write-Host "Project: $projectName"

# ============================================================================
# XML Loading
# ============================================================================

[xml]$xml = Get-Content $VcxprojPath
$ns = New-Object System.Xml.XmlNamespaceManager($xml.NameTable)
$ns.AddNamespace("ms", "http://schemas.microsoft.com/developer/msbuild/2003")

# ============================================================================
# Helper Functions
# ============================================================================

function ConvertPath([string]$p) {
    # Strip $(ProjectDir) prefix, convert backslashes to forward slashes
    $p = $p -replace '^\$\(ProjectDir\)', ''
    return $p.Replace('\', '/')
}

function FormatList([string[]]$items, [int]$indent = 4) {
    $pad = ' ' * $indent
    return ($items | ForEach-Object { "$pad$_" }) -join "`n"
}

# ============================================================================
# Qt Module Mapping
# ============================================================================

$qtModuleMap = @{
    'core'              = 'Core'
    'gui'               = 'Gui'
    'network'           = 'Network'
    'widgets'           = 'Widgets'
    'svg'               = 'Svg'
    'svgwidgets'        = 'SvgWidgets'
    'xml'               = 'Xml'
    'pdfwidgets'        = 'PdfWidgets'
    'opengl'            = 'OpenGL'
    'openglwidgets'     = 'OpenGLWidgets'
    'multimedia'        = 'Multimedia'
    'multimediawidgets' = 'MultimediaWidgets'
    'websockets'        = 'WebSockets'
    'concurrent'        = 'Concurrent'
    'dbus'              = 'DBus'
    'sql'               = 'Sql'
    'printsupport'      = 'PrintSupport'
    'quick'             = 'Quick'
    'qml'               = 'Qml'
}

function MapQtModule([string]$name) {
    $lower = $name.ToLower().Trim()
    if ($qtModuleMap.ContainsKey($lower)) {
        return $qtModuleMap[$lower]
    }
    # Fallback: capitalize first letter
    Write-Warning "Unknown Qt module '$name'; using capitalized form."
    return $lower.Substring(0,1).ToUpper() + $lower.Substring(1)
}

# ============================================================================
# MSVC Compiler Settings Mapping
# ============================================================================

$warningLevelMap = @{
    'Level1'              = '/W1'
    'Level2'              = '/W2'
    'Level3'              = '/W3'
    'Level4'              = '/W4'
    'EnableAllWarnings'   = '/Wall'
}

# ============================================================================
# Parse: C++ Standard
# ============================================================================

$langStdNode = $xml.SelectSingleNode(
    "//ms:ItemDefinitionGroup[not(@Label)]/ms:ClCompile/ms:LanguageStandard", $ns)
$cppStandard = "20"
if ($langStdNode) {
    $match = [regex]::Match($langStdNode.InnerText, 'stdcpp(\d+)')
    if ($match.Success) {
        $cppStandard = $match.Groups[1].Value
    }
}

Write-Host "  C++ Standard: $cppStandard"

# ============================================================================
# Parse: Qt Modules
# ============================================================================

$qtModulesNode = $xml.SelectSingleNode(
    "//ms:PropertyGroup[@Label='QtSettings']/ms:QtModules", $ns)
$qtModulesRaw = @()
$qtModulesCMake = @()
if ($qtModulesNode) {
    $qtModulesRaw = $qtModulesNode.InnerText -split ';' | Where-Object { $_.Trim() }
    $qtModulesCMake = $qtModulesRaw | ForEach-Object { MapQtModule $_ }
}

Write-Host "  Qt Modules: $($qtModulesCMake -join ', ')"

# ============================================================================
# Parse: Source and Header Files
# ============================================================================

# ClCompile items (.cpp)
$clCompileNodes = $xml.SelectNodes("//ms:ClCompile[@Include]", $ns)
$allSources = @()
foreach ($node in $clCompileNodes) {
    $allSources += ConvertPath $node.GetAttribute("Include")
}

# ClInclude + QtMoc items (.h)
$clIncludeNodes = $xml.SelectNodes("//ms:ClInclude[@Include]", $ns)
$qtMocNodes = $xml.SelectNodes("//ms:QtMoc[@Include]", $ns)
$allHeaders = @()
foreach ($node in $clIncludeNodes) {
    $allHeaders += ConvertPath $node.GetAttribute("Include")
}
foreach ($node in $qtMocNodes) {
    $allHeaders += ConvertPath $node.GetAttribute("Include")
}

# Split into project vs Coco (submodules/Coco prefix)
$cocoPrefix = "submodules/Coco"
$ferSources = $allSources | Where-Object { $_ -notlike "$cocoPrefix*" } | Sort-Object
$cocoSources = $allSources | Where-Object { $_ -like "$cocoPrefix*" } | Sort-Object
$ferHeaders = $allHeaders | Where-Object { $_ -notlike "$cocoPrefix*" } | Sort-Object
$cocoHeaders = $allHeaders | Where-Object { $_ -like "$cocoPrefix*" } | Sort-Object

Write-Host "  Sources: $($ferSources.Count) project + $($cocoSources.Count) Coco"
Write-Host "  Headers: $($ferHeaders.Count) project + $($cocoHeaders.Count) Coco"

# ============================================================================
# Parse: Resources (.qrc and .rc)
# ============================================================================

$qtRccNodes = $xml.SelectNodes("//ms:QtRcc[@Include]", $ns)
$rcNodes = $xml.SelectNodes("//ms:ResourceCompile[@Include]", $ns)
$resources = @()
foreach ($node in $rcNodes) {
    $resources += ConvertPath $node.GetAttribute("Include")
}
foreach ($node in $qtRccNodes) {
    $resources += ConvertPath $node.GetAttribute("Include")
}

Write-Host "  Resources: $($resources.Count)"

# ============================================================================
# Parse: Include Directories
# ============================================================================

$includeNode = $xml.SelectSingleNode(
    "//ms:ItemDefinitionGroup[not(@Label)]/ms:ClCompile/ms:AdditionalIncludeDirectories", $ns)
$includeDirs = @()
if ($includeNode) {
    $includeDirs = $includeNode.InnerText -split ';' |
        Where-Object { $_.Trim() -and $_ -ne '%(AdditionalIncludeDirectories)' } |
        ForEach-Object { ConvertPath $_ }
}

Write-Host "  Include dirs: $($includeDirs -join ', ')"

# ============================================================================
# Parse: Preprocessor Definitions
# ============================================================================

$definesNode = $xml.SelectSingleNode(
    "//ms:ItemDefinitionGroup[not(@Label)]/ms:ClCompile/ms:PreprocessorDefinitions", $ns)
$autoDefines = @('_UNICODE', 'UNICODE', '%(PreprocessorDefinitions)')
$defines = @()
if ($definesNode) {
    $defines = $definesNode.InnerText -split ';' |
        Where-Object { $_.Trim() -and $_ -notin $autoDefines }
}

Write-Host "  Defines: $($defines -join ', ')"

# ============================================================================
# Parse: Additional Dependencies (bit7z / imported libraries)
# ============================================================================

# We look for .lib entries in AdditionalDependencies that are not MSBuild macros.
# Parse from each config's ItemDefinitionGroup.

$importedLibs = @{}  # name -> @{ Debug = path; Release = path }

$itemDefGroups = $xml.SelectNodes("//ms:ItemDefinitionGroup[not(@Label)]", $ns)
foreach ($idg in $itemDefGroups) {
    $condition = $idg.GetAttribute("Condition")
    $config = $null
    if ($condition -match "Debug") { $config = "Debug" }
    elseif ($condition -match "Release") { $config = "Release" }
    if (-not $config) { continue }

    $depsNode = $idg.SelectSingleNode("ms:Link/ms:AdditionalDependencies", $ns)
    if (-not $depsNode) { continue }

    $entries = $depsNode.InnerText -split ';' | Where-Object {
        $_.Trim() -and
        $_ -ne '%(AdditionalDependencies)' -and
        $_ -notmatch '^\$\(CoreLibraryDependencies\)$' -and
        $_ -match '\.lib$'
    }

    foreach ($entry in $entries) {
        $clean = ConvertPath $entry
        $libName = [System.IO.Path]::GetFileNameWithoutExtension($clean)
        if (-not $importedLibs.ContainsKey($libName)) {
            $importedLibs[$libName] = @{}
        }
        $importedLibs[$libName][$config] = $clean
    }
}

foreach ($lib in $importedLibs.Keys) {
    Write-Host "  Imported lib: $lib"
    foreach ($cfg in $importedLibs[$lib].Keys) {
        Write-Host "    $cfg`: $($importedLibs[$lib][$cfg])"
    }
}

# ============================================================================
# Parse: Translations
# ============================================================================

$tsNodes = $xml.SelectNodes("//ms:QtTranslation[@Include]", $ns)
$pluralOnlyFiles = @()
$regularTsFiles = @()
$allTsFiles = @()

foreach ($node in $tsNodes) {
    $tsPath = ConvertPath $node.GetAttribute("Include")
    $allTsFiles += $tsPath

    # Check for per-file PluralOnly
    $isPluralOnly = $false
    $pluralNodes = $node.SelectNodes("ms:PluralOnly", $ns)
    foreach ($pn in $pluralNodes) {
        if ($pn.InnerText -eq 'true') {
            $isPluralOnly = $true
            break
        }
    }

    if ($isPluralOnly) {
        $pluralOnlyFiles += $tsPath
    } else {
        $regularTsFiles += $tsPath
    }
}

$hasTranslations = $allTsFiles.Count -gt 0
Write-Host "  Translations: $($allTsFiles.Count) ($($pluralOnlyFiles.Count) pluralonly)"

# ============================================================================
# Parse: Compiler/Linker Settings
# ============================================================================

# Common (from both configs, take first match)
$commonCompileFlags = @()
$releaseCompileFlags = @()
$releaseLinkerFlags = @()

$allIdgs = $xml.SelectNodes("//ms:ItemDefinitionGroup", $ns)
foreach ($idg in $allIdgs) {
    $condition = $idg.GetAttribute("Condition")
    $isRelease = $condition -match "Release"

    $clNode = $idg.SelectSingleNode("ms:ClCompile", $ns)
    if (-not $clNode) { continue }

    # Warning level (common)
    $wl = $clNode.SelectSingleNode("ms:WarningLevel", $ns)
    if ($wl -and $warningLevelMap.ContainsKey($wl.InnerText)) {
        $flag = $warningLevelMap[$wl.InnerText]
        if ($flag -notin $commonCompileFlags) { $commonCompileFlags += $flag }
    }

    # MultiProcessorCompilation (common)
    $mp = $clNode.SelectSingleNode("ms:MultiProcessorCompilation", $ns)
    if ($mp -and $mp.InnerText -eq 'true') {
        if ('/MP' -notin $commonCompileFlags) { $commonCompileFlags += '/MP' }
    }

    # SDLCheck (common)
    $sdl = $clNode.SelectSingleNode("ms:SDLCheck", $ns)
    if ($sdl -and $sdl.InnerText -eq 'true') {
        if ('/sdl' -notin $commonCompileFlags) { $commonCompileFlags += '/sdl' }
    }

    # ConformanceMode (common)
    $conf = $clNode.SelectSingleNode("ms:ConformanceMode", $ns)
    if ($conf -and $conf.InnerText -eq 'true') {
        if ('/permissive-' -notin $commonCompileFlags) { $commonCompileFlags += '/permissive-' }
    }

    # Release-only compiler flags
    if ($isRelease) {
        $fl = $clNode.SelectSingleNode("ms:FunctionLevelLinking", $ns)
        if ($fl -and $fl.InnerText -eq 'true') { $releaseCompileFlags += '/Gy' }

        $intr = $clNode.SelectSingleNode("ms:IntrinsicFunctions", $ns)
        if ($intr -and $intr.InnerText -eq 'true') { $releaseCompileFlags += '/Oi' }
    }
}

# WholeProgramOptimization is at the PropertyGroup level, not ItemDefinitionGroup
$wpoNode = $xml.SelectSingleNode(
    "//ms:PropertyGroup[contains(@Condition,'Release') and @Label='Configuration']/ms:WholeProgramOptimization", $ns)
if ($wpoNode -and $wpoNode.InnerText -eq 'true') {
    $releaseCompileFlags = @('/GL') + $releaseCompileFlags
}

# Release linker flags (from labeled Configuration ItemDefinitionGroup)
$relLinkIdgs = $xml.SelectNodes(
    "//ms:ItemDefinitionGroup[contains(@Condition,'Release') and @Label='Configuration']/ms:Link", $ns)
foreach ($linkNode in $relLinkIdgs) {
    $optRef = $linkNode.SelectSingleNode("ms:OptimizeReferences", $ns)
    if ($optRef -and $optRef.InnerText -eq 'true') { $releaseLinkerFlags += '/OPT:REF' }

    $comdat = $linkNode.SelectSingleNode("ms:EnableCOMDATFolding", $ns)
    if ($comdat -and $comdat.InnerText -eq 'true') { $releaseLinkerFlags += '/OPT:ICF' }
}

# LTCG pairs with /GL
if ($releaseCompileFlags -contains '/GL') {
    $releaseLinkerFlags += '/LTCG'
}

Write-Host "  Common MSVC flags: $($commonCompileFlags -join ' ')"
Write-Host "  Release compile flags: $($releaseCompileFlags -join ' ')"
Write-Host "  Release linker flags: $($releaseLinkerFlags -join ' ')"

# ============================================================================
# Parse: Pre-Build Events
# ============================================================================

# Look for pre-build commands in each config
$preBuildCommands = @{}  # config -> command string

foreach ($idg in $itemDefGroups) {
    $condition = $idg.GetAttribute("Condition")
    $config = $null
    if ($condition -match "Debug") { $config = "Debug" }
    elseif ($condition -match "Release") { $config = "Release" }
    if (-not $config) { continue }

    $preNode = $idg.SelectSingleNode("ms:PreBuildEvent/ms:Command", $ns)
    if ($preNode -and $preNode.InnerText.Trim()) {
        $preBuildCommands[$config] = $preNode.InnerText.Trim()
    }
}

# We only handle Release pre-build (the common case for version generation).
# Parse PowerShell invocations of the form:
#   powershell -ExecutionPolicy Bypass -File "$(ProjectDir)path\to\script.ps1"
$preBuildScript = $null
if ($preBuildCommands.ContainsKey("Release")) {
    $cmd = $preBuildCommands["Release"]
    $psMatch = [regex]::Match($cmd, '-File\s+"?\$\(ProjectDir\)([^"]+)"?')
    if ($psMatch.Success) {
        $preBuildScript = $psMatch.Groups[1].Value.Replace('\', '/')
    }
}

if ($preBuildScript) {
    Write-Host "  Pre-build (Release): $preBuildScript"
}

# ============================================================================
# Generate CMakeLists.txt
# ============================================================================

$out = [System.Text.StringBuilder]::new()

function W([string]$line = "") { [void]$out.AppendLine($line) }
function WN([string]$line = "") { [void]$out.Append($line) }  # No newline

# --- Header ---

W "# WARNING: This file is generated by scripts/BuildCMakeLists.ps1."
W "# If issues arise, change the script, not this file."
W ""
W "# WARNING: This does not compile on non-Windows platforms yet!"
W ""
W "# Run:"
W "#   mkdir build"
W "#   cd build"
W "#   cmake .. -DCMAKE_PREFIX_PATH=<Qt path, e.g., C:\Qt\6.10.2\msvc2022_64>"
W "#   cmake --build . --config Release"
W "#   cmake --install . --config Release --prefix <output_dir>"
W ""

# --- Project setup ---

W "cmake_minimum_required(VERSION 3.21)"
W "project($projectName LANGUAGES CXX)"
W ""
W "set(CMAKE_CXX_STANDARD $cppStandard)"
W "set(CMAKE_CXX_STANDARD_REQUIRED ON)"
W ""

# --- Qt auto-processing ---

W "# --- Qt auto-processing ---"
W "# AUTOMOC: Runs moc on headers that contain Q_OBJECT/Q_GADGET (handles both"
W "#          ClInclude and QtMoc items from the .vcxproj; CMake detects which"
W "#          headers need moc automatically)"
W "# AUTORCC: Compiles .qrc resource files into the binary"
W "# AUTOUIC: Not currently needed (no .ui files), but harmless to enable"
W ""
W "set(CMAKE_AUTOMOC ON)"
W "set(CMAKE_AUTORCC ON)"
W "set(CMAKE_AUTOUIC ON)"
W ""

# --- Qt modules ---

$findComponents = $qtModulesCMake -join ' '
if ($hasTranslations) {
    $findComponents += ' LinguistTools'
}

W "# --- Qt modules ---"
W "# Parsed from <QtModules> in the .vcxproj QtSettings PropertyGroup."
W "# The .vcxproj uses lowercase semicolon-delimited names (e.g., `"pdfwidgets`");"
W "# CMake's find_package requires PascalCase component names (e.g., `"PdfWidgets`")."
if ($hasTranslations) {
    W "# The LinguistTools component is added for translation support (lupdate/lrelease)."
}
W ""
W "find_package(Qt6 REQUIRED COMPONENTS"
W "    $findComponents"
W ")"
W "qt_standard_project_setup()"
W ""

# --- Sources ---

W "# --- Sources ---"
W "# From <ClCompile> items in the .vcxproj (excluding Coco sources, listed"
W "# separately below). Backslashes converted to forward slashes."
W ""
W "set(${projectNameUpper}_SOURCES"
W (FormatList $ferSources)
W ")"
W ""

# --- Headers ---

W "# --- Headers ---"
W "# From both <ClInclude> and <QtMoc> items in the .vcxproj (excluding Coco"
W "# headers, listed separately below). CMake's AUTOMOC makes the QtMoc vs"
W "# ClInclude distinction irrelevant; all headers are treated uniformly and"
W "# moc is run on whichever ones contain Q_OBJECT/Q_GADGET."
W ""
W "set(${projectNameUpper}_HEADERS"
W (FormatList $ferHeaders)
W ")"
W ""

# --- Coco ---

if ($cocoSources.Count -gt 0 -or $cocoHeaders.Count -gt 0) {
    W "# --- Coco (compiled as part of $projectName) ---"
    W "# Coco is a git submodule with no CMakeLists.txt of its own. Its sources are"
    W "# compiled directly as part of the $projectName target, matching the .vcxproj"
    W "# behavior where they appear in <ClCompile> and <ClInclude>/<QtMoc> items."
    W ""
    if ($cocoSources.Count -gt 0) {
        W "set(COCO_SOURCES"
        W (FormatList $cocoSources)
        W ")"
        W ""
    }
    if ($cocoHeaders.Count -gt 0) {
        W "set(COCO_HEADERS"
        W (FormatList $cocoHeaders)
        W ")"
        W ""
    }
}

# --- Resources ---

W "# --- Resources ---"
W "# From <QtRcc> and <ResourceCompile> items in the .vcxproj."
W "# .qrc files are handled by AUTORCC. The .rc file (Windows resource script"
W "# for app icon, version info, etc.) is passed to the resource compiler"
W "# automatically by CMake on Windows."
W ""
W "set(${projectNameUpper}_RESOURCES"
W (FormatList $resources)
W ")"
W ""

# --- Target ---

$targetSources = "`${${projectNameUpper}_SOURCES}"
$targetSources += "`n    `${${projectNameUpper}_HEADERS}"
if ($cocoSources.Count -gt 0) { $targetSources += "`n    `${COCO_SOURCES}" }
if ($cocoHeaders.Count -gt 0) { $targetSources += "`n    `${COCO_HEADERS}" }
$targetSources += "`n    `${${projectNameUpper}_RESOURCES}"

W "# --- Target ---"
W "# WIN32 sets the Windows subsystem (no console window), matching the"
W "# .vcxproj <SubSystem>Windows</SubSystem> setting."
W ""
W "qt_add_executable($projectName WIN32"
W "    $targetSources"
W ")"
W ""

# --- Include directories ---

W "# --- Include directories ---"
W "# From <AdditionalIncludeDirectories> in the .vcxproj, minus `$(ProjectDir)"
W "# prefix (CMake resolves relative to CMakeLists.txt location) and minus the"
W "# %(AdditionalIncludeDirectories) inheritance macro (MSBuild-only concept)."
W ""
W "target_include_directories($projectName PRIVATE"
W (FormatList $includeDirs)
W ")"
W ""

# --- Preprocessor definitions ---

W "# --- Preprocessor definitions ---"
W "# From <PreprocessorDefinitions> in the .vcxproj, minus:"
W "#   _UNICODE / UNICODE: Set automatically by CMake for Unicode CharacterSet"
W "#   %(PreprocessorDefinitions): MSBuild inheritance macro (no CMake equivalent)"
W ""
W "target_compile_definitions($projectName PRIVATE"
W (FormatList $defines)
W ")"
W ""

# --- Imported libraries (bit7z etc.) ---

foreach ($libName in $importedLibs.Keys) {
    $lib = $importedLibs[$libName]

    W "# --- $libName (pre-built static library) ---"
    W "# $libName is linked as a pre-built library. Pre-built .lib files are stored"
    W "# under external/$libName/lib/x64/{Config}/. An IMPORTED target with per-config"
    W "# paths replaces the .vcxproj <AdditionalDependencies> entries."
    W ""
    W "add_library($libName STATIC IMPORTED)"

    $propsLines = @()
    if ($lib.ContainsKey("Debug")) {
        $propsLines += "    IMPORTED_LOCATION_DEBUG `"`${CMAKE_CURRENT_SOURCE_DIR}/$($lib['Debug'])`""
    }
    if ($lib.ContainsKey("Release")) {
        $propsLines += "    IMPORTED_LOCATION_RELEASE `"`${CMAKE_CURRENT_SOURCE_DIR}/$($lib['Release'])`""
    }
    $propsLines += "    MAP_IMPORTED_CONFIG_MINSIZEREL Release"
    $propsLines += "    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release"

    W "set_target_properties($libName PROPERTIES"
    foreach ($pl in $propsLines) { W $pl }
    W ")"
    W "target_link_libraries($projectName PRIVATE $libName)"
    W ""
}

# --- Qt linking ---

$qtLinkTargets = $qtModulesCMake | ForEach-Object { "Qt6::$_" }

W "# --- Qt linking ---"
W "# Each Qt6::Module target transitively provides its required include paths,"
W "# preprocessor definitions (e.g., QT_DEBUG/QT_NO_DEBUG), and link dependencies."
W "# This replaces the implicit handling that Qt VS Tools performs via MSBuild."
W ""
W "target_link_libraries($projectName PRIVATE"
W (FormatList $qtLinkTargets)
W ")"
W ""

# --- Translations ---

if ($hasTranslations) {
    $allTsSorted = $allTsFiles | Sort-Object

    # Build the comment about pluralonly
    $pluralNote = ""
    if ($pluralOnlyFiles.Count -gt 0) {
        $pluralNames = ($pluralOnlyFiles | ForEach-Object { [System.IO.Path]::GetFileName($_) }) -join ', '
        $pluralNote = @"
#
# Separate lupdate calls are needed because $pluralNames requires the
# -pluralonly flag (from per-file <PluralOnly>true</PluralOnly> in the
# .vcxproj), while other translation files do not.
"@
    }

    W "# --- Translations ---"
    W "# From <QtTranslation> items in the .vcxproj. The build action is"
    W "# lupdate + lrelease (matching <BuildAction>lupdate_lrelease</BuildAction>)."
    W "#"
    W "# lupdate scans sources and updates .ts files; lrelease compiles .ts to .qm."
    W "# The .qm files are output to the build directory (next to the .exe), NOT"
    W "# embedded as Qt resources. The application loads them at runtime via"
    W "# QCoreApplication::applicationDirPath()."
    if ($pluralNote) { W $pluralNote }
    W ""

    W "set(${projectNameUpper}_TS_FILES"
    W (FormatList $allTsSorted)
    W ")"
    W ""

    # Pluralonly files: one lupdate call per file
    foreach ($tsFile in $pluralOnlyFiles) {
        $tsName = [System.IO.Path]::GetFileNameWithoutExtension($tsFile) -replace '^Translation_', ''
        W "# $($tsName.ToUpper()): lupdate with -pluralonly"
        W "qt_add_lupdate($projectName"
        W "    TS_FILES $tsFile"
        W "    OPTIONS -pluralonly"
        W ")"
        W ""
    }

    # Non-pluralonly files: single lupdate call
    if ($regularTsFiles.Count -gt 0) {
        $regularSorted = @($regularTsFiles | Sort-Object)
        W "# All other translations (single call)"
        W "qt_add_lupdate($projectName"
        if ($regularSorted.Count -eq 1) {
            W "    TS_FILES $($regularSorted[0])"
        } else {
            W "    TS_FILES"
            W (FormatList $regularSorted 8)
        }
        W ")"
        W ""
    }

    # lrelease
    W "# lrelease: compile all .ts to .qm, output next to the executable"
    W "qt_add_lrelease($projectName"
    W "    TS_FILES `${${projectNameUpper}_TS_FILES}"
    W "    QM_FILES_OUTPUT_VARIABLE QM_FILES"
    W ")"
    W ""
}

# --- Compiler options ---

W "# --- Compiler options (MSVC) ---"
W "# From the .vcxproj <ItemDefinitionGroup> compiler and linker settings:"

# Build the comment mapping
$flagComments = @{
    '/W1' = 'WarningLevel Level1'; '/W2' = 'WarningLevel Level2'
    '/W3' = 'WarningLevel Level3'; '/W4' = 'WarningLevel Level4'
    '/Wall' = 'WarningLevel EnableAllWarnings'
    '/MP' = 'MultiProcessorCompilation'; '/sdl' = 'SDLCheck'
    '/permissive-' = 'ConformanceMode'
    '/GL' = 'WholeProgramOptimization (Release)'
    '/Gy' = 'FunctionLevelLinking (Release)'
    '/Oi' = 'IntrinsicFunctions (Release)'
    '/OPT:REF' = 'OptimizeReferences (Release)'
    '/OPT:ICF' = 'EnableCOMDATFolding (Release)'
    '/LTCG' = 'Linker counterpart of /GL (Release)'
}

$allFlags = $commonCompileFlags + $releaseCompileFlags + $releaseLinkerFlags
$maxFlagLen = ($allFlags | ForEach-Object { $_.Length } | Measure-Object -Maximum).Maximum
foreach ($flag in $allFlags) {
    if ($flagComments.ContainsKey($flag)) {
        $padded = ($flag + ':').PadRight($maxFlagLen + 1)
        W "#   $padded $($flagComments[$flag])"
    }
}
W ""
W "if(MSVC)"

if ($commonCompileFlags.Count -gt 0) {
    $commonStr = $commonCompileFlags -join ' '
    W "    target_compile_options($projectName PRIVATE"
    W "        $commonStr"
    W "    )"
}

if ($releaseCompileFlags.Count -gt 0) {
    $relStr = $releaseCompileFlags -join ' '
    W "    target_compile_options($projectName PRIVATE"
    W "        `$<`$<CONFIG:Release>:$relStr>"
    W "    )"
}

if ($releaseLinkerFlags.Count -gt 0) {
    $relLnkStr = $releaseLinkerFlags -join ' '
    W "    target_link_options($projectName PRIVATE"
    W "        `$<`$<CONFIG:Release>:$relLnkStr>"
    W "    )"
}

W "endif()"
W ""

# --- Pre-build ---

if ($preBuildScript) {
    W "# --- Pre-build: version string generation (Release only) ---"
    W "# From <PreBuildEvent> in the Release ItemDefinitionGroup. Runs a PowerShell"
    W "# script that generates Version.txt in the build output directory."
    W ""
    W "add_custom_command(TARGET $projectName PRE_BUILD"
    W "    COMMAND `$<`$<CONFIG:Release>:powershell>"
    W "            `$<`$<CONFIG:Release>:-ExecutionPolicy>"
    W "            `$<`$<CONFIG:Release>:Bypass>"
    W "            `$<`$<CONFIG:Release>:-File>"
    W "            `$<`$<CONFIG:Release>:`"`${CMAKE_CURRENT_SOURCE_DIR}/$preBuildScript`">"
    W "            `$<`$<CONFIG:Release>:-OutputDir>"
    W "            `$<`$<CONFIG:Release>:`"`$<TARGET_FILE_DIR:$projectName>`">"
    W "    COMMENT `"Generating version string (Release only)`""
    W ")"
    W ""
}

# --- Install ---

W "# --- Install ---"
W "# Defines the deployment layout for ``cmake --install``. This is the CMake"
W "# equivalent of the Windows packaging batch file's copy steps, and is what"
W "# Linux packagers use to build .deb/.rpm/AppImage packages."
W "#"
W "# Usage:"
W "#   cmake --install . --config Release --prefix <output_dir>"
W "#"
W "# Produces:"
W "#   <output_dir>/bin/$projectName(.exe)"
if ($hasTranslations) {
    foreach ($ts in ($allTsFiles | Sort-Object)) {
        $qmName = [System.IO.Path]::GetFileNameWithoutExtension($ts) + ".qm"
        W "#   <output_dir>/bin/$qmName"
    }
}
if ($preBuildScript) {
    W "#   <output_dir>/bin/Version.txt"
}
W ""
W "install(TARGETS $projectName RUNTIME DESTINATION bin)"
if ($hasTranslations) {
    W "install(FILES `${QM_FILES} DESTINATION bin)"
}
if ($preBuildScript) {
    W "install(FILES `$<TARGET_FILE_DIR:$projectName>/Version.txt DESTINATION bin OPTIONAL)"
}

# ============================================================================
# Write Output
# ============================================================================

$outputPath = Join-Path $projectDir "CMakeLists.txt"
$content = $out.ToString().TrimEnd() + "`n"

# Normalize line endings to LF for cross-platform compatibility
$content = $content -replace "`r`n", "`n"

[System.IO.File]::WriteAllText($outputPath, $content)

Write-Host ""
Write-Host "Generated: $outputPath"
Write-Host "Done!"

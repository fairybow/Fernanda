# Visual Studio Project Settings

Configuration Properties > General > C++ Language Standard

```
ISO C++20 Standard (/std:c++20)
```

Configuration Properties > General > C Language Standard

```
ISO C17 (2018) Standard (/std:c17)
```

Configuration Properties > Qt Project Settings > Qt Modules

```
core;gui;network;widgets;svg;xml;pdfwidgets
```

Configuration Properties > C/C++ > General > Additional Include Directories

```
$(ProjectDir)src;$(ProjectDir)submodules\Coco\Coco\include;$(ProjectDir)external\bit7z\include;%(AdditionalIncludeDirectories)
```

Configuration Properties > C/C++ > Preprocessor > Preprocessor Definitions

```
_UNICODE;UNICODE;NOMINMAX;WIN32_LEAN_AND_MEAN;%(PreprocessorDefinitions)
```

(Debug) Configuration Properties > Linker > Input > Additional Dependencies

```
$(ProjectDir)external\bit7z\lib\x64\Debug\bit7z.lib;$(CoreLibraryDependencies);%(AdditionalDependencies)
```

(Release) Configuration Properties > Linker > Input > Additional Dependencies

```
$(ProjectDir)external\bit7z\lib\x64\Release\bit7z.lib;$(CoreLibraryDependencies);%(AdditionalDependencies)
```

(Release) Configuration Properties > Build Events > Pre-Build Event > Command Line

```
powershell -ExecutionPolicy Bypass -File "$(ProjectDir)packaging\VSPreBuildGenVersionFullStringTxt.ps1"
```

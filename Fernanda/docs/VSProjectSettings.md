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
core;gui;network;widgets;svg;xml
```

Configuration Properties > C/C++ > General > Additional Include Directories

```
$(ProjectDir)src;$(ProjectDir)submodules\Coco\Coco\include;$(ProjectDir)external\bit7z\include;%(AdditionalIncludeDirectories)
```

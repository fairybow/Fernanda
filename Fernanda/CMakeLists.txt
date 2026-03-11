# Run:
#   mkdir build
#   cd build
#   cmake .. -DCMAKE_PREFIX_PATH=<Qt path, e.g., C:\Qt\6.10.2\msvc2022_64>
#   cmake --build . --config Release
#   cmake --install . --config Release --prefix <output_dir>

# Fernanda - A distraction-free plain text editor for long-form fiction
# https://github.com/fairybow/Fernanda

cmake_minimum_required(VERSION 3.21)
project(Fernanda LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Qt auto-processing ---

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Qt modules ---

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Network Widgets Svg Xml PdfWidgets LinguistTools
)
qt_standard_project_setup()

# =============================================================================
# File lists
# =============================================================================

# --- Fernanda sources ---

set(FERNANDA_SOURCES
    src/AboutDialog.cpp
    src/CollapsibleWidget.cpp
    src/FnxModel.cpp
    src/Main.cpp
    src/PlainTextEdit.cpp
    src/TabWidget.cpp
    src/TextFileView.cpp
    src/Tr.cpp
    src/Window.cpp
    src/WindowService.cpp
    src/Workspace.cpp
)

# --- Fernanda headers ---

set(FERNANDA_HEADERS
    src/AboutDialog.h
    src/AbstractFileModel.h
    src/AbstractFileView.h
    src/AbstractService.h
    src/AccordionWidget.h
    src/AppDirs.h
    src/Application.h
    src/BetaAlert.h
    src/BuildMessages.h
    src/Bus.h
    src/CollapsibleWidget.h
    src/CollapsibleWidgetHeader.h
    src/ColorBar.h
    src/ColorBarModule.h
    src/ColorBarPanel.h
    src/Commander.h
    src/ControlField.h
    src/Debug.h
    src/DisplaySlider.h
    src/EditorPanel.h
    src/FileMeta.h
    src/FileService.h
    src/FileTypes.h
    src/Fnx.h
    src/FnxModel.h
    src/FnxModelCache.h
    src/FontPanel.h
    src/Formatters.h
    src/ImageFileModel.h
    src/ImageFileView.h
    src/Ini.h
    src/Io.h
    src/KeyFilters.h
    src/KeyFiltersPanel.h
    src/MagicBytes.h
    src/MenuBuilder.h
    src/MenuShortcuts.h
    src/MenuState.h
    src/NewNotebookPrompt.h
    src/Notebook.h
    src/Notepad.h
    src/NotepadFileSystemModel.h
    src/PdfFileModel.h
    src/PdfFileView.h
    src/PlainTextEdit.h
    src/Qss.h
    src/SaveFailMessageBox.h
    src/SavePrompt.h
    src/SelectionHandleOverlay.h
    src/SettingsDialog.h
    src/SettingsPanel.h
    src/SettingsService.h
    src/StyleContext.h
    src/StyleModule.h
    src/TabWidget.h
    src/TabWidgetButton.h
    src/TabWidgetCloseButton.h
    src/TabWidgetTabBar.h
    src/TabWidgetUnderlay.h
    src/TempDir.h
    src/TextFileModel.h
    src/TextFileView.h
    src/Themes.h
    src/ThemesPanel.h
    src/TieredSettings.h
    src/Timers.h
    src/ToString.h
    src/Tr.h
    src/TrashPrompt.h
    src/TreeView.h
    src/TreeViewService.h
    src/UpdateDialog.h
    src/Version.h
    src/ViewService.h
    src/Window.h
    src/WindowService.h
    src/WordCounter.h
    src/WordCounterModule.h
    src/WordCounterPanel.h
    src/Workspace.h
    src/XPlatform.h
    src/ZoomControl.h
)

# --- Coco (compiled as part of Fernanda) ---

set(COCO_SOURCES
    submodules/Coco/Coco/src/Path.cpp
    submodules/Coco/Coco/src/StartCop.cpp
)

set(COCO_HEADERS
    submodules/Coco/Coco/include/Coco/Bool.h
    submodules/Coco/Coco/include/Coco/Concepts.h
    submodules/Coco/Coco/include/Coco/Fx.h
    submodules/Coco/Coco/include/Coco/Path.h
    submodules/Coco/Coco/include/Coco/Private.h
    submodules/Coco/Coco/include/Coco/StartCop.h
    submodules/Coco/Coco/include/Coco/Utility.h
)

# --- Resources ---

set(FERNANDA_RESOURCES
    resources/Fernanda.rc
    external/External.qrc
    resources/Icons.qrc
    resources/Themes.qrc
    resources/Ui.qrc
)

# =============================================================================
# Target
# =============================================================================

qt_add_executable(Fernanda WIN32
    ${FERNANDA_SOURCES}
    ${FERNANDA_HEADERS}
    ${COCO_SOURCES}
    ${COCO_HEADERS}
    ${FERNANDA_RESOURCES}
)

# =============================================================================
# Source groups (VS Solution Explorer filters)
# =============================================================================

source_group("Code" FILES
    src/Main.cpp
)

source_group("Code\\Core" FILES
    src/Application.h
    src/AppDirs.h
    src/BuildMessages.h
    src/Debug.h
    src/Formatters.h
    src/TempDir.h
    src/Timers.h
    src/ToString.h
    src/Tr.h
    src/Tr.cpp
    src/Version.h
    src/XPlatform.h
)

source_group("Code\\Core\\Files" FILES
    src/FileMeta.h
)

source_group("Code\\Core\\Files\\Models" FILES
    src/AbstractFileModel.h
    src/ImageFileModel.h
    src/PdfFileModel.h
    src/TextFileModel.h
)

source_group("Code\\Core\\Files\\Views" FILES
    src/AbstractFileView.h
    src/ImageFileView.h
    src/PdfFileView.h
)

source_group("Code\\Core\\Files\\Views\\TextFileView" FILES
    src/KeyFilters.h
    src/TextFileView.h
    src/TextFileView.cpp
)

source_group("Code\\Core\\Services" FILES
    src/AbstractService.h
    src/FileService.h
    src/TreeViewService.h
    src/ViewService.h
    src/WindowService.h
    src/WindowService.cpp
)

source_group("Code\\Core\\Services\\Settings" FILES
    src/Ini.h
    src/SettingsDialog.h
    src/SettingsService.h
    src/TieredSettings.h
)

source_group("Code\\Core\\UI" FILES
    src/AboutDialog.h
    src/AboutDialog.cpp
    src/BetaAlert.h
    src/TreeView.h
    src/UpdateDialog.h
    src/Window.h
    src/Window.cpp
    src/ZoomControl.h
)

source_group("Code\\Core\\UI\\AccordionWidget" FILES
    src/AccordionWidget.h
    src/CollapsibleWidget.h
    src/CollapsibleWidget.cpp
    src/CollapsibleWidgetHeader.h
)

source_group("Code\\Core\\UI\\Menus" FILES
    src/MenuBuilder.h
    src/MenuState.h
)

source_group("Code\\Core\\UI\\PlainTextEdit" FILES
    src/PlainTextEdit.h
    src/PlainTextEdit.cpp
    src/SelectionHandleOverlay.h
)

source_group("Code\\Core\\UI\\Saving" FILES
    src/SaveFailMessageBox.h
    src/SavePrompt.h
)

source_group("Code\\Core\\UI\\Settings" FILES
    src/ColorBarPanel.h
    src/ControlField.h
    src/DisplaySlider.h
    src/EditorPanel.h
    src/FontPanel.h
    src/KeyFiltersPanel.h
    src/SettingsPanel.h
    src/ThemesPanel.h
    src/WordCounterPanel.h
)

source_group("Code\\Core\\UI\\TabWidget" FILES
    src/TabWidget.h
    src/TabWidget.cpp
    src/TabWidgetButton.h
    src/TabWidgetCloseButton.h
    src/TabWidgetTabBar.h
    src/TabWidgetUnderlay.h
)

source_group("Code\\Core\\Workspaces" FILES
    src/FileTypes.h
    src/Io.h
    src/MagicBytes.h
    src/MenuShortcuts.h
    src/Workspace.h
    src/Workspace.cpp
)

source_group("Code\\Core\\Workspaces\\Bus" FILES
    src/Bus.h
    src/Commander.h
)

source_group("Code\\Core\\Workspaces\\Notebook" FILES
    src/Fnx.h
    src/FnxModel.h
    src/FnxModel.cpp
    src/FnxModelCache.h
    src/NewNotebookPrompt.h
    src/Notebook.h
    src/TrashPrompt.h
)

source_group("Code\\Core\\Workspaces\\Notepad" FILES
    src/Notepad.h
    src/NotepadFileSystemModel.h
)

source_group("Code\\Auxiliary\\Modules\\ColorBar" FILES
    src/ColorBar.h
    src/ColorBarModule.h
)

source_group("Code\\Auxiliary\\Modules\\Style" FILES
    src/Qss.h
    src/StyleContext.h
    src/StyleModule.h
    src/Themes.h
)

source_group("Code\\Auxiliary\\Modules\\WordCounter" FILES
    src/WordCounter.h
    src/WordCounterModule.h
)

source_group("Submodules\\Coco" FILES
    ${COCO_SOURCES}
    ${COCO_HEADERS}
)

source_group("Resources" FILES ${FERNANDA_RESOURCES})

# =============================================================================
# Build configuration
# =============================================================================

target_include_directories(Fernanda PRIVATE
    src
    submodules/Coco/Coco/include
    external/bit7z/include
)

target_compile_definitions(Fernanda PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

# --- bit7z (pre-built static library, Windows only) ---
# TODO: Replace with add_subdirectory() for cross-platform builds

add_library(bit7z STATIC IMPORTED)
set_target_properties(bit7z PROPERTIES
    IMPORTED_LOCATION_DEBUG
        "${CMAKE_CURRENT_SOURCE_DIR}/external/bit7z/lib/x64/Debug/bit7z.lib"
    IMPORTED_LOCATION_RELEASE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/bit7z/lib/x64/Release/bit7z.lib"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)
target_link_libraries(Fernanda PRIVATE bit7z)

# --- Qt linking ---

target_link_libraries(Fernanda PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::Svg
    Qt6::Xml
    Qt6::PdfWidgets
)

# --- Compiler options (MSVC) ---

if(MSVC)
    target_compile_options(Fernanda PRIVATE
        /W3 /MP /sdl /permissive- /bigobj
    )
    target_compile_options(Fernanda PRIVATE
        $<$<CONFIG:Release>:/GL /Gy /Oi>
    )
    target_link_options(Fernanda PRIVATE
        $<$<CONFIG:Release>:/OPT:REF /OPT:ICF /LTCG>
    )
endif()

# =============================================================================
# Translations
# =============================================================================

set(FERNANDA_TS_FILES
    translations/Translation_en.ts
    translations/Translation_es.ts
)

qt_add_lupdate(Fernanda
    TS_FILES translations/Translation_en.ts
    OPTIONS -pluralonly
)

qt_add_lupdate(Fernanda
    TS_FILES translations/Translation_es.ts
)

qt_add_lrelease(Fernanda
    TS_FILES ${FERNANDA_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

# =============================================================================
# Pre-build and install
# =============================================================================

add_custom_command(TARGET Fernanda PRE_BUILD
    COMMAND $<$<CONFIG:Release>:powershell>
            $<$<CONFIG:Release>:-ExecutionPolicy>
            $<$<CONFIG:Release>:Bypass>
            $<$<CONFIG:Release>:-File>
            $<$<CONFIG:Release>:"${CMAKE_CURRENT_SOURCE_DIR}/packaging/VSPreBuildGenVersionFullStringTxt.ps1">
            $<$<CONFIG:Release>:-OutputDir>
            $<$<CONFIG:Release>:"$<TARGET_FILE_DIR:Fernanda>">
    COMMENT "Generating version string (Release only)"
)

install(TARGETS Fernanda RUNTIME DESTINATION bin)
install(FILES ${QM_FILES} DESTINATION bin)
install(FILES $<TARGET_FILE_DIR:Fernanda>/Version.txt DESTINATION bin OPTIONAL)

# Fernanda - A plain text editor for drafting long-form fiction. (At least, that's the plan.)
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
    src/Main.cpp
    src/core/Tr.cpp
    src/services/WindowService.cpp

    src/AboutDialog.cpp
    src/CollapsibleWidget.cpp
    src/FnxModel.cpp
    src/PlainTextEdit.cpp
    src/TabWidget.cpp
    src/TextFileView.cpp
    src/Window.cpp
    src/Workspace.cpp
)

# --- Fernanda headers ---

set(FERNANDA_HEADERS
    src/core/AppDirs.h
    src/core/Application.h
    src/core/BuildMessages.h
    src/core/Debug.h
    src/core/Formatters.h
    src/core/TempDir.h
    src/core/ToString.h
    src/core/Tr.h
    src/core/Version.h
    src/core/XPlatform.h

    src/services/AbstractService.h
    src/services/FileService.h
    src/services/SettingsService.h
    src/services/TieredSettings.h
    src/services/TreeViewService.h
    src/services/ViewService.h
    src/services/WindowService.h

    src/AboutDialog.h
    src/AbstractFileModel.h
    src/AbstractFileView.h
    src/AccordionWidget.h
    src/BetaAlert.h
    src/Bus.h
    src/CollapsibleWidget.h
    src/CollapsibleWidgetHeader.h
    src/ColorBar.h
    src/ColorBarModule.h
    src/ColorBarPanel.h
    src/Commander.h
    src/ControlField.h
    src/DisplaySlider.h
    src/EditorPanel.h
    src/FileMeta.h
    src/FileTypes.h
    src/Fnx.h
    src/FnxModel.h
    src/FnxModelCache.h
    src/FontPanel.h
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
    src/StyleContext.h
    src/StyleModule.h
    src/TabWidget.h
    src/TabWidgetButton.h
    src/TabWidgetCloseButton.h
    src/TabWidgetTabBar.h
    src/TabWidgetUnderlay.h
    src/TextFileModel.h
    src/TextFileView.h
    src/Themes.h
    src/ThemesPanel.h
    src/Timers.h
    src/TrashPrompt.h
    src/TreeView.h
    src/UpdateDialog.h
    src/Window.h
    src/WordCounter.h
    src/WordCounterModule.h
    src/WordCounterPanel.h
    src/Workspace.h
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
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
    src/dialogs/AboutDialog.cpp
    src/fnx/FnxModel.cpp
    src/services/WindowService.cpp
    src/views/TextFileView.cpp
    src/workspaces/Workspace.cpp

    src/CollapsibleWidget.cpp
    src/PlainTextEdit.cpp
    src/TabWidget.cpp
    src/Window.cpp
)

# --- Fernanda headers ---

set(FERNANDA_HEADERS
    src/core/AppDirs.h
    src/core/Application.h
    src/core/BuildMessages.h
    src/core/Debug.h
    src/core/Formatters.h
    src/core/TempDir.h
    src/core/Timers.h
    src/core/ToString.h
    src/core/Tr.h
    src/core/Version.h
    src/core/XPlatform.h

    src/dialogs/AboutDialog.h
    src/dialogs/BetaAlert.h
    src/dialogs/UpdateDialog.h

    src/fnx/Fnx.h
    src/fnx/FnxModel.h
    src/fnx/FnxModelCache.h

    src/services/AbstractService.h
    src/services/FileService.h
    src/services/SettingsService.h
    src/services/TieredSettings.h
    src/services/TreeViewService.h
    src/services/ViewService.h
    src/services/WindowService.h

    src/models/AbstractFileModel.h
    src/models/FileMeta.h
    src/models/ImageFileModel.h
    src/models/PdfFileModel.h
    src/models/TextFileModel.h

    src/modules/ColorBar.h
    src/modules/ColorBarModule.h
    src/modules/Qss.h
    src/modules/StyleContext.h
    src/modules/StyleModule.h
    src/modules/Themes.h
    src/modules/WordCounter.h
    src/modules/WordCounterModule.h

    src/views/AbstractFileView.h
    src/views/ImageFileView.h
    src/views/PdfFileView.h
    src/views/TextFileView.h

    src/workspaces/Bus.h
    src/workspaces/Commander.h
    src/workspaces/NewNotebookPrompt.h
    src/workspaces/Notebook.h
    src/workspaces/Notepad.h
    src/workspaces/NotepadFileSystemModel.h
    src/workspaces/SaveFailMessageBox.h
    src/workspaces/SavePrompt.h
    src/workspaces/TrashPrompt.h
    src/workspaces/Workspace.h

    src/AccordionWidget.h
    src/CollapsibleWidget.h
    src/CollapsibleWidgetHeader.h
    src/ColorBarPanel.h
    src/ControlField.h
    src/DisplaySlider.h
    src/EditorPanel.h
    src/FileTypes.h
    src/FontPanel.h
    src/Ini.h
    src/Io.h
    src/KeyFilters.h
    src/KeyFiltersPanel.h
    src/MagicBytes.h
    src/MenuBuilder.h
    src/MenuShortcuts.h
    src/MenuState.h
    src/PlainTextEdit.h
    src/SelectionHandleOverlay.h
    src/SettingsDialog.h
    src/SettingsPanel.h
    src/TabWidget.h
    src/TabWidgetButton.h
    src/TabWidgetCloseButton.h
    src/TabWidgetTabBar.h
    src/TabWidgetUnderlay.h
    src/ThemesPanel.h
    src/TreeView.h
    src/Window.h
    src/WordCounterPanel.h
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
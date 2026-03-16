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
    src/core/Debug.cpp
    src/core/Tr.cpp
    src/dialogs/AboutDialog.cpp
    src/fnx/FnxModel.cpp
    src/services/WindowService.cpp
    src/ui/PlainTextEdit.cpp
    src/ui/TabWidget.cpp
    src/ui/Window.cpp
    src/views/TextFileView.cpp
    src/workspaces/Workspace.cpp
)

if(APPLE)
    list(APPEND FERNANDA_SOURCES src/core/XPlatform_MacOS.mm)
endif()

# --- Fernanda headers ---

set(FERNANDA_HEADERS
    src/core/AppDirs.h
    src/core/Application.h
    src/core/BuildMessages.h
    src/core/Debug.h
    src/core/FileTypes.h
    src/core/Formatters.h
    src/core/Io.h
    src/core/MagicBytes.h
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

    src/menus/MenuBuilder.h
    src/menus/MenuShortcuts.h
    src/menus/MenuState.h

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

    src/services/AbstractService.h
    src/services/FileService.h
    src/services/ReloadPrompt.h
    src/services/SettingsService.h
    src/services/TieredSettings.h
    src/services/TreeViewService.h
    src/services/ViewService.h
    src/services/WindowService.h

    src/settings/ColorBarPanel.h
    src/settings/EditorPanel.h
    src/settings/FontPanel.h
    src/settings/Ini.h
    src/settings/KeyFiltersPanel.h
    src/settings/SettingsDialog.h
    src/settings/SettingsPanel.h
    src/settings/ThemesPanel.h
    src/settings/WordCounterPanel.h

    src/ui/ControlField.h
    src/ui/DisplaySlider.h
    src/ui/DrawerWidget.h
    src/ui/DrawerWidgetHeader.h
    src/ui/Painting.h
    src/ui/PlainTextEdit.h
    src/ui/SelectionHandleOverlay.h
    src/ui/TabWidget.h
    src/ui/TabWidgetAlertWidget.h
    src/ui/TabWidgetButton.h
    src/ui/TabWidgetCloseButton.h
    src/ui/TabWidgetTabBar.h
    src/ui/TabWidgetUnderlay.h
    src/ui/TreeView.h
    src/ui/Window.h
    src/ui/ZoomControl.h

    src/views/AbstractFileView.h
    src/views/ImageFileView.h
    src/views/KeyFilters.h
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
)

if(WIN32)
    target_compile_definitions(Fernanda PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# --- bit7z (built from source via submodule) ---

set(BIT7Z_AUTO_FORMAT ON CACHE BOOL "Enable auto format detection")
# Add any other bit7z options you need here, e.g.:
# set(BIT7Z_USE_NATIVE_STRING ON CACHE BOOL "")

# --- bit7z (built from source via submodule) ---
# bit7z compiles 7-Zip source into its static library at build time. This should
# match the version of the runtime DLL/SO files bundled in Fernanda's resources.
# Our fork (Fernanda-bit7z) is pointed to 82f3593 (v4.0.11 release commit).

set(BIT7Z_7ZIP_VERSION "25.01" CACHE STRING "7-Zip version to build against")
add_subdirectory(submodules/bit7z)

# --- Linking ---

target_link_libraries(Fernanda PRIVATE
    bit7z64
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::Svg
    Qt6::Xml
    Qt6::PdfWidgets
)

if(UNIX AND NOT APPLE)
    find_package(X11 COMPONENTS xcb)
    if(X11_xcb_FOUND)
        target_link_libraries(Fernanda PRIVATE X11::xcb)
    endif()
elseif(APPLE)
    target_link_libraries(Fernanda PRIVATE
        "-framework AppKit"
        objc
    )
endif()

# --- Compiler options (MSVC) ---

# TODO: Other compilers!

if(MSVC)
    target_compile_options(Fernanda PRIVATE
        /W3 /MP /sdl /permissive- /bigobj /EHsc
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
# Version.txt (Release builds only)
# =============================================================================

file(STRINGS src/core/Version.h _ver_lines)

foreach(_line ${_ver_lines})
    if(_line MATCHES "^#define VERSION_MAJOR +([0-9]+)")
        set(_ver_major ${CMAKE_MATCH_1})
    elseif(_line MATCHES "^#define VERSION_MINOR +([0-9]+)")
        set(_ver_minor ${CMAKE_MATCH_1})
    elseif(_line MATCHES "^#define VERSION_PATCH +([0-9]+)")
        set(_ver_patch ${CMAKE_MATCH_1})
    elseif(_line MATCHES "^#define VERSION_IS_PRERELEASE +([01])")
        set(_ver_is_pre ${CMAKE_MATCH_1})
    elseif(_line MATCHES "^#define VERSION_PRERELEASE_STRING +\"([^\"]+)\"")
        set(_ver_pre ${CMAKE_MATCH_1})
    endif()
endforeach()

set(FERNANDA_VERSION "${_ver_major}.${_ver_minor}.${_ver_patch}")
if(_ver_is_pre STREQUAL "1" AND _ver_pre)
    string(APPEND FERNANDA_VERSION "-${_ver_pre}")
endif()

message(STATUS "Fernanda version: ${FERNANDA_VERSION}")

file(GENERATE
    OUTPUT "$<TARGET_FILE_DIR:Fernanda>/Version.txt"
    CONTENT "${FERNANDA_VERSION}"
    CONDITION $<CONFIG:Release>
)

# =============================================================================
# Install
# =============================================================================

install(TARGETS Fernanda RUNTIME DESTINATION bin)
install(FILES ${QM_FILES} DESTINATION bin)
install(FILES $<TARGET_FILE_DIR:Fernanda>/Version.txt DESTINATION bin OPTIONAL)
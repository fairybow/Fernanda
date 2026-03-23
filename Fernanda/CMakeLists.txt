# Fernanda - A plain text editor for drafting long-form fiction. (At least, that's the plan.)
# https://github.com/fairybow/Fernanda

# TODO: Non-Windows builds not yet tested

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
# Version.h parsing
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
    elseif(_line MATCHES "^#define VERSION_AUTHOR_STRING +\"([^\"]+)\"")
        set(_ver_author ${CMAKE_MATCH_1})
    elseif(_line MATCHES "^#define VERSION_APP_NAME_STRING +\"([^\"]+)\"")
        set(_ver_app_name ${CMAKE_MATCH_1})
    elseif(_line MATCHES "^#define VERSION_COPYRIGHT_STRING +\"([^\"]+)\"")
        set(_ver_copyright ${CMAKE_MATCH_1})
    endif()
endforeach()

set(FERNANDA_VERSION "${_ver_major}.${_ver_minor}.${_ver_patch}")
if(_ver_is_pre STREQUAL "1" AND _ver_pre)
    string(APPEND FERNANDA_VERSION "-${_ver_pre}")
endif()

message(STATUS "Fernanda version: ${FERNANDA_VERSION}")

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
    list(APPEND FERNANDA_SOURCES src/core/XPlatform_macOS.mm)
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
    src/core/Random.h
    src/core/Time.h
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

    src/workspaces/Backup.h
    src/workspaces/Bus.h
    src/workspaces/Commander.h
    src/workspaces/NewNotebookPrompt.h
    src/workspaces/Notebook.h
    src/workspaces/NotebookLockfile.h
    src/workspaces/Notepad.h
    src/workspaces/NotepadFileSystemModel.h
    src/workspaces/SaveFailMessageBox.h
    src/workspaces/SavePrompt.h
    src/workspaces/TrashPrompt.h
    src/workspaces/WorkingDir.h
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
    external/External.qrc
    resources/Icons.qrc
    resources/Themes.qrc
    resources/Ui.qrc
)

if(WIN32)
    list(APPEND FERNANDA_RESOURCES resources/Fernanda.rc)
endif()

# =============================================================================
# Target
# =============================================================================

if(WIN32)
    set(PLATFORM_EXE_FLAGS WIN32)
elseif(APPLE)
    set(PLATFORM_EXE_FLAGS MACOSX_BUNDLE)
endif()

qt_add_executable(Fernanda ${PLATFORM_EXE_FLAGS}
    ${FERNANDA_SOURCES}
    ${FERNANDA_HEADERS}
    ${COCO_SOURCES}
    ${COCO_HEADERS}
    ${FERNANDA_RESOURCES}
)

# --- macOS bundle ---

if(APPLE)
    set(MACOS_ICON ${CMAKE_CURRENT_SOURCE_DIR}/icons/Fernanda.icns)
    set_source_files_properties(${MACOS_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(Fernanda PRIVATE ${MACOS_ICON})

set_target_properties(Fernanda PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "${_ver_app_name}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.${_ver_author}.${_ver_app_name}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${FERNANDA_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${_ver_major}.${_ver_minor}.${_ver_patch}"
        MACOSX_BUNDLE_COPYRIGHT "${_ver_copyright}"
        MACOSX_BUNDLE_ICON_FILE "${_ver_app_name}.icns"
    )
endif()

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

# --- miniz (built from source via submodule) ---

add_subdirectory(submodules/miniz)

# --- Linking ---

target_link_libraries(Fernanda PRIVATE
    miniz
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

# --- Compiler options ---

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
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Fernanda PRIVATE
        -Wall -Wextra -Wpedantic -fexceptions
    )
    target_compile_options(Fernanda PRIVATE
        $<$<CONFIG:Release>:-O2>
    )
    target_compile_options(Fernanda PRIVATE
        $<$<CONFIG:Debug>:-g>
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

file(GENERATE
    OUTPUT "$<TARGET_FILE_DIR:Fernanda>/Version.txt"
    CONTENT "${FERNANDA_VERSION}"
    CONDITION $<CONFIG:Release>
)

# =============================================================================
# Install
# =============================================================================

install(TARGETS Fernanda
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

install(FILES ${QM_FILES} DESTINATION bin)
install(FILES $<TARGET_FILE_DIR:Fernanda>/Version.txt DESTINATION bin OPTIONAL)
# Hearth — a plain-text-first workbench for creative writing
# https://github.com/fairybow/Hearth

cmake_minimum_required(VERSION 3.21)
project(Hearth LANGUAGES CXX C)

set_property(GLOBAL PROPERTY VS_STARTUP_PROJECT Hearth)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Qt auto-processing ---

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Qt modules ---

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Network Widgets Svg Xml PdfWidgets WebEngineWidgets LinguistTools
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

set(HEARTH_VERSION "${_ver_major}.${_ver_minor}.${_ver_patch}")
if(_ver_is_pre STREQUAL "1" AND _ver_pre)
    string(APPEND HEARTH_VERSION "-${_ver_pre}")
endif()

message(STATUS "Hearth version: ${HEARTH_VERSION}")

# =============================================================================
# File lists
# =============================================================================

# --- Hearth sources ---

set(HEARTH_SOURCES
    src/Main.cpp
    src/core/Debug.cpp
    src/core/Tr.cpp
    src/dialogs/AboutDialog.cpp
    src/nbx/NbxModel.cpp
    src/services/ViewService.cpp
    src/services/WindowService.cpp
    src/ui/PlainTextEdit.cpp
    src/ui/TabWidget.cpp
    src/ui/Window.cpp
    src/views/TextFileView.cpp
    src/workspaces/Workspace.cpp
)

if(APPLE)
    list(APPEND HEARTH_SOURCES src/core/XPlatform_macOS.mm)
endif()

# --- Hearth headers ---

set(HEARTH_HEADERS
    src/core/AppDirs.h
    src/core/Application.h
    src/core/BuildMessages.h
    src/core/BundledFonts.h
    src/core/Debug.h
    src/core/Disk.h
    src/core/Files.h
    src/core/Hash.h
    src/core/Io.h
    src/core/LogViewer.h
    src/core/MagicBytes.h
    src/core/Random.h
    src/core/Time.h
    src/core/Tr.h
    src/core/Version.h
    src/core/XPlatform.h

    src/dialogs/AboutDialog.h
    src/dialogs/BetaAlert.h
    src/dialogs/UpdateDialog.h

    src/fmt/Fmt.h
    src/fmt/ToQString.h

    src/menus/MenuBuilder.h
    src/menus/MenuShortcuts.h
    src/menus/MenuState.h

    src/models/AbstractFileModel.h
    src/models/FileMeta.h
    src/models/PdfFileModel.h
    src/models/RawFileModel.h
    src/models/TextFileModel.h

    src/modules/ColorBarModule.h
    src/modules/Qss.h
    src/modules/StyleContext.h
    src/modules/StyleModule.h
    src/modules/Themes.h
    src/modules/WordCounterModule.h

    src/nbx/Nbx.h
    src/nbx/NbxModel.h
    src/nbx/NbxModelCache.h
    src/nbx/NbxModelIcons.h

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

    src/ui/ColorBar.h
    src/ui/ControlField.h
    src/ui/DisplaySlider.h
    src/ui/DrawerWidget.h
    src/ui/DrawerWidgetHeader.h
    src/ui/Icons.h
    src/ui/MultiSwitch.h
    src/ui/Painting.h
    src/ui/PlainTextEdit.h
    src/ui/SelectionHandleOverlay.h
    src/ui/TabSurface.h
    src/ui/TabWidget.h
    src/ui/TabWidgetAlertWidget.h
    src/ui/TabWidgetButton.h
    src/ui/TabWidgetCloseButton.h
    src/ui/TabWidgetTabBar.h
    src/ui/TabWidgetUnderlay.h
    src/ui/TreeView.h
    src/ui/WidgetMask.h
    src/ui/WidgetSnapshotOverlay.h
    src/ui/Window.h
    src/ui/WordCounter.h
    src/ui/ZoomControl.h
    src/ui/ZoomState.h

    src/views/AbstractFileView.h
    src/views/AbstractMarkupFileView.h
    src/views/FountainFileView.h
    src/views/HtmlFileView.h
    src/views/ImageFileView.h
    src/views/ImageGraphicsView.h
    src/views/KeyFilters.h
    src/views/MarkdownFileView.h
    src/views/MarkupWebcode.h
    src/views/PdfFileView.h
    src/views/TextFileView.h
    src/views/WebEnginePage.h
    src/views/WebEngineView.h

    src/workspaces/Backup.h
    src/workspaces/Bus.h
    src/workspaces/Commander.h
    src/workspaces/Docx.h
    src/workspaces/NewNotebookPrompt.h
    src/workspaces/Notebook.h
    src/workspaces/NotebookColorChip.h
    src/workspaces/NotebookImport.h
    src/workspaces/NotebookLockfile.h
    src/workspaces/Notepad.h
    src/workspaces/NotepadFileSystemModel.h
    src/workspaces/NotepadImport.h
    src/workspaces/NotepadRecovery.h
    src/workspaces/Rtf.h
    src/workspaces/SaveFailMessageBox.h
    src/workspaces/SavePrompt.h
    src/workspaces/TrashPrompt.h
    src/workspaces/WorkingDir.h
    src/workspaces/Workspace.h
)

# --- Coco (compiled as part of Hearth) ---

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

set(HEARTH_RESOURCES
    external/External.qrc
    resources/AppIcons.qrc
    resources/Themes.qrc
    resources/Ui.qrc
)

if(WIN32)
    list(APPEND HEARTH_RESOURCES resources/Hearth.rc)
endif()

# =============================================================================
# Target
# =============================================================================

if(WIN32)
    set(PLATFORM_EXE_FLAGS WIN32)
elseif(APPLE)
    set(PLATFORM_EXE_FLAGS MACOSX_BUNDLE)
endif()

qt_add_executable(Hearth ${PLATFORM_EXE_FLAGS}
    ${HEARTH_SOURCES}
    ${HEARTH_HEADERS}
    ${COCO_SOURCES}
    ${COCO_HEADERS}
    ${HEARTH_RESOURCES}
)

# --- macOS bundle ---

if(APPLE)
    set(MACOS_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/app-icons/Hearth.icns)
    set_source_files_properties(${MACOS_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(Hearth PRIVATE ${MACOS_ICON})

set_target_properties(Hearth PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "${_ver_app_name}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.${_ver_author}.${_ver_app_name}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${HEARTH_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${_ver_major}.${_ver_minor}.${_ver_patch}"
        MACOSX_BUNDLE_COPYRIGHT "${_ver_copyright}"
        MACOSX_BUNDLE_ICON_FILE "${_ver_app_name}.icns"
    )
endif()

# =============================================================================
# Build configuration
# =============================================================================

# NB: Even though some modules pull in others, I'd prefer to be explicit and
# always include what's used

# Pass cmake -DHEARTH_USE_SYSTEM_LIBS=ON -B build
option(HEARTH_USE_SYSTEM_LIBS "Use system-installed md4c and miniz" OFF)

if(HEARTH_USE_SYSTEM_LIBS)
    # Hearth's bundled submodules pin md4c 0.5.2 and miniz 3.1.1. md4c's
    # CMake doesn't populate its version in the exported config, so
    # find_package can't verify it. Version omitted here as a workaround.
    # 0.5.2 is the expected version
    find_package(md4c REQUIRED)
    find_package(miniz 3.1.1 REQUIRED)
    set(MD4C_TARGETS md4c::md4c md4c::md4c-html)
    set(MINIZ_TARGETS miniz::miniz)
else()
    set(MD4C_TARGETS md4c md4c-html)
    set(MINIZ_TARGETS miniz)
endif()

# CMake requires absolute paths for include directories when they come from
# generator expressions. The non-conditional paths get resolved automatically,
# but genexpr contents don't get that treatment
target_include_directories(Hearth PRIVATE
    src
    submodules/Coco/Coco/include
    submodules/fountain.h/fountain.h/include
    $<$<NOT:$<BOOL:${HEARTH_USE_SYSTEM_LIBS}>>:${CMAKE_CURRENT_SOURCE_DIR}/submodules/md4c/src>
    $<$<NOT:$<BOOL:${HEARTH_USE_SYSTEM_LIBS}>>:${CMAKE_CURRENT_SOURCE_DIR}/submodules/md4c/md2html>
    $<$<NOT:$<BOOL:${HEARTH_USE_SYSTEM_LIBS}>>:${CMAKE_CURRENT_SOURCE_DIR}/submodules/miniz>
)

if(WIN32)
    target_compile_definitions(Hearth PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

target_compile_definitions(Hearth PRIVATE
    $<$<CONFIG:Debug>:VERSION_DEBUG>
)

# --- miniz (built from source via submodule) ---

add_subdirectory(submodules/fountain.h/fountain.h EXCLUDE_FROM_ALL)

if(NOT HEARTH_USE_SYSTEM_LIBS)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    add_subdirectory(submodules/md4c EXCLUDE_FROM_ALL)
    add_subdirectory(submodules/miniz EXCLUDE_FROM_ALL)
endif()

# --- Linking ---

target_link_libraries(Hearth PRIVATE
    fountain_h
    ${MD4C_TARGETS}
    ${MINIZ_TARGETS}
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::Svg
    Qt6::Xml
    Qt6::PdfWidgets
    Qt6::WebEngineWidgets
)

if(UNIX AND NOT APPLE)
    find_package(X11 COMPONENTS xcb)
    if(X11_xcb_FOUND)
        target_link_libraries(Hearth PRIVATE X11::xcb)
    endif()
elseif(APPLE)
    target_link_libraries(Hearth PRIVATE
        "-framework AppKit"
        objc
    )
endif()

# --- Compiler options ---

if(MSVC)
    target_compile_options(Hearth PRIVATE
        /W3 /MP /sdl /permissive- /bigobj /EHsc /Zc:preprocessor
    )
    target_compile_options(Hearth PRIVATE
        $<$<CONFIG:Release>:/GL /Gy /Oi>
    )
    target_link_options(Hearth PRIVATE
        $<$<CONFIG:Release>:/OPT:REF /OPT:ICF /LTCG>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Hearth PRIVATE
        -Wall -Wextra -Wpedantic -fexceptions
    )
    target_compile_options(Hearth PRIVATE
        $<$<CONFIG:Release>:-O2>
    )
    target_compile_options(Hearth PRIVATE
        $<$<CONFIG:Debug>:-g>
    )
endif()

# =============================================================================
# Translations
# =============================================================================

set(HEARTH_TS_FILES
    translations/Translation_en.ts
    translations/Translation_es.ts
)

qt_add_lupdate(Hearth
    TS_FILES translations/Translation_en.ts
    OPTIONS -pluralonly
)

qt_add_lupdate(Hearth
    TS_FILES translations/Translation_es.ts
)

qt_add_lrelease(Hearth
    TS_FILES ${HEARTH_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

# =============================================================================
# Install
# =============================================================================

install(TARGETS Hearth
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

if(UNIX AND NOT APPLE)
    install(FILES resources/Hearth.desktop
        DESTINATION share/applications
    )

    foreach(SIZE 16 22 24 32 48 64 128 256 512)
        set(_icon "${CMAKE_CURRENT_SOURCE_DIR}/resources/app-icons/Hearth-${SIZE}.png")
        if(EXISTS "${_icon}")
            install(FILES ${_icon}
                DESTINATION share/icons/hicolor/${SIZE}x${SIZE}/apps
                RENAME Hearth.png
            )
        endif()
    endforeach()

    install(FILES resources/app-icons/proof/Hearth.svg
        DESTINATION share/icons/hicolor/scalable/apps
    )

    install(FILES ${QM_FILES}
        DESTINATION share/Hearth/translations
    )
else()
    install(FILES ${QM_FILES} DESTINATION bin)
endif()
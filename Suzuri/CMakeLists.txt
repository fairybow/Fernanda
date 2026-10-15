# Suzuri — A plain-text editor for creative writing
# Copyright (C) 2026 fairybow
#
# This program is free software, redistributable and/or modifiable under the
# terms of the GNU GPL v3. It's distributed in the hope that it will be useful
# but without any warranty (even the implied warranty of merchantability or
# fitness for a particular purpose)
#
# See the LICENSE file or visit <https://www.gnu.org/licenses/>

cmake_minimum_required(VERSION 3.21)
project(Suzuri LANGUAGES CXX C)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Suzuri)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# NB: Not used yet, but will be most likely - keep
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Qt auto-processing ---

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Qt modules ---

find_package(Qt6 6.7 REQUIRED COMPONENTS
    Core Gui Network PdfWidgets Svg Widgets LinguistTools
)

# NB: Also includes GNUInstallDirs (CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_DATADIR)
# and sets an $ORIGIN-relative install RPATH on Linux
qt_standard_project_setup()

# --- Coco (submodule) ---

set(COCO_WITH_NETWORK ON)
set(COCO_WITH_XML OFF)
set(COCO_WITH_ASSERTIONS "AUTO")
set(COCO_WITH_DIAGNOSTICS "AUTO")

add_subdirectory(submodules/Coco/Coco)

# --- Hunspell (submodule) ---

# Hunspell has no CMake build of its own, so its library is compiled here from
# its sources. HUNSPELL_STATIC stops its headers marking symbols for a DLL.
# Its headers are SYSTEM so they raise no warnings in Suzuri's code, and its
# own warnings are off: they aren't ours to fix. None of it is Qt code, so the
# Qt code generators are off for it
set(_hunspell_dir submodules/hunspell/src/hunspell)

add_library(hunspell STATIC
    ${_hunspell_dir}/affentry.cxx
    ${_hunspell_dir}/affixmgr.cxx
    ${_hunspell_dir}/csutil.cxx
    ${_hunspell_dir}/filemgr.cxx
    ${_hunspell_dir}/hashmgr.cxx
    ${_hunspell_dir}/hunspell.cxx
    ${_hunspell_dir}/hunspelltrace.cxx
    ${_hunspell_dir}/hunzip.cxx
    ${_hunspell_dir}/phonet.cxx
    ${_hunspell_dir}/replist.cxx
    ${_hunspell_dir}/suggestmgr.cxx
)

set_target_properties(hunspell PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

target_include_directories(hunspell SYSTEM PUBLIC ${_hunspell_dir})
target_compile_definitions(hunspell PUBLIC HUNSPELL_STATIC)

if(MSVC)
    target_compile_options(hunspell PRIVATE /w)
else()
    target_compile_options(hunspell PRIVATE -w)
endif()

# =============================================================================
# Parsing
# =============================================================================

set(_ver_header src/core/Version.h)
set(_pub_header src/core/Publication.h)

# file(STRINGS) creates no dependency; without this, editing these headers
# leaves the parsed values (and the bundle and .desktop metadata built from
# them) stale until the next manual reconfigure
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${_ver_header}
    ${_pub_header}
)

# NB: Checks use `NOT DEFINED`, never `NOT ${var}`: if() treats values like 0,
# OFF, and N as false, so a correctly parsed "0" would fail a truthiness check.
# The regexes use `+`, so a match can't produce an empty value

# --- Version.h ---

file(STRINGS ${_ver_header} _ver_lines)

foreach(_line ${_ver_lines})
    if(_line MATCHES "^#define VERSION_MAJOR +([0-9]+)")
        set(_ver_major "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^#define VERSION_MINOR +([0-9]+)")
        set(_ver_minor "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^#define VERSION_PATCH +([0-9]+)")
        set(_ver_patch "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^#define VERSION_IS_PRERELEASE +([01])")
        set(_ver_is_pre "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^#define VERSION_PRERELEASE_STRING +\"([^\"]+)\"")
        set(_ver_pre "${CMAKE_MATCH_1}")
    endif()
endforeach()

foreach(_var _ver_major _ver_minor _ver_patch _ver_is_pre)
    if(NOT DEFINED ${_var})
        message(FATAL_ERROR "Failed to parse ${_var} from ${_ver_header}")
    endif()
endforeach()

# NB: Apple requires dot-separated integers for both bundle version keys, so
# the bundle gets this and never the prerelease label
set(APP_VERSION_NUMERIC "${_ver_major}.${_ver_minor}.${_ver_patch}")
set(APP_VERSION "${APP_VERSION_NUMERIC}")

if(_ver_is_pre STREQUAL "1")
    if(NOT DEFINED _ver_pre)
        message(FATAL_ERROR
            "VERSION_IS_PRERELEASE is 1 but VERSION_PRERELEASE_STRING failed "
            "to parse from ${_ver_header}"
        )
    endif()

    string(APPEND APP_VERSION "-${_ver_pre}")
endif()

message(STATUS "Version ${APP_VERSION}")

# --- Publication.h ---

file(STRINGS ${_pub_header} _pub_lines)

foreach(_line ${_pub_lines})
    if(_line MATCHES "^#define PUB_AUTHOR_STRING +\"([^\"]+)\"")
        set(_pub_author "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^#define PUB_APP_NAME_STRING +\"([^\"]+)\"")
        set(_pub_app_name "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^#define PUB_APP_ID_STRING +\"([^\"]+)\"")
        set(_pub_app_id "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "^#define PUB_COPYRIGHT_STRING +\"([^\"]+)\"")
        set(_pub_copyright "${CMAKE_MATCH_1}")
    endif()
endforeach()

foreach(_var _pub_author _pub_app_name _pub_app_id _pub_copyright)
    if(NOT DEFINED ${_var})
        message(FATAL_ERROR "Failed to parse ${_var} from ${_pub_header}")
    endif()
endforeach()

message(STATUS "${_pub_copyright}")
message(STATUS "App ID ${_pub_app_id}")

# =============================================================================
# File lists
# =============================================================================

# --- Sources ---

set(APP_SOURCES
    src/Main.cpp
)

# --- Headers ---

set(APP_HEADERS
    src/core/spell/Misspelling.h
    src/core/spell/SpellChecker.h
    src/core/spell/SpellCheckers.h
    src/core/spell/SpellWords.h
    src/core/spell/WordList.h

    src/core/ActionIds.h
    src/core/AppActions.h
    src/core/AppConfig.h
    src/core/AppDirs.h
    src/core/BundledFonts.h
    src/core/Clargs.h
    src/core/CMakeMacroGuard.h
    src/core/CoreConstants.h
    src/core/FileRef.h
    src/core/FileTypes.h
    src/core/Io.h
    src/core/JsonIo.h
    src/core/Publication.h
    src/core/RecentFiles.h
    src/core/TextCounts.h
    src/core/UpdateCheck.h
    src/core/Updater.h
    src/core/Vault.h
    src/core/VaultConfig.h
    src/core/VaultDotDir.h
    src/core/VaultEntry.h
    src/core/VaultTreeModel.h
    src/core/VaultTreeNode.h
    src/core/Version.h
    src/core/WorkspaceKeys.h

    src/models/AbstractFileModel.h
    src/models/ImageFileModel.h
    src/models/PdfFileModel.h
    src/models/PrimeDocument.h
    src/models/TextFileModel.h

    src/ui/settings/FontFamilyBox.h
    src/ui/settings/SettingRow.h
    src/ui/settings/SettingsAppearancePage.h
    src/ui/settings/SettingsDialog.h
    src/ui/settings/SettingsEditorPage.h
    src/ui/settings/SettingsGeneralPage.h
    src/ui/settings/SettingsPage.h
    src/ui/settings/SettingsStatusBarPage.h

    src/ui/sidebar/Drawer.h
    src/ui/sidebar/RenameDialog.h
    src/ui/sidebar/SettingsButton.h
    src/ui/sidebar/Sidebar.h
    src/ui/sidebar/VaultEntryDragMime.h
    src/ui/sidebar/VaultFileTree.h
    src/ui/sidebar/VaultSwitcher.h
    src/ui/sidebar/VaultTreeItemDelegate.h
    src/ui/sidebar/VaultTreeView.h

    src/ui/tabs/NewTabButton.h
    src/ui/tabs/NewTabPage.h
    src/ui/tabs/PageIcon.h
    src/ui/tabs/PagePin.h
    src/ui/tabs/TabBar.h
    src/ui/tabs/TabCloseButton.h
    src/ui/tabs/TabPaneLeaf.h
    src/ui/tabs/TabPaneTree.h

    src/ui/widgets/ButtonWash.h
    src/ui/widgets/DisplaySlider.h
    src/ui/widgets/Glyph.h
    src/ui/widgets/GlyphButton.h
    src/ui/widgets/RowBadges.h
    src/ui/widgets/ToggleSwitch.h
    src/ui/widgets/UserTextEntry.h

    src/ui/AboutPanel.h
    src/ui/BaseWindow.h
    src/ui/CursorPosition.h
    src/ui/FileSwitcher.h
    src/ui/LicensesDialog.h
    src/ui/LossyOpenPrompt.h
    src/ui/ManageVaults.h
    src/ui/NewVaultDialog.h
    src/ui/OpenMode.h
    src/ui/PopoutWindow.h
    src/ui/Style.h
    src/ui/UiConstants.h
    src/ui/UiUtility.h
    src/ui/UpdateNotice.h
    src/ui/VaultWindow.h
    src/ui/WordCounter.h
    src/ui/WorkspaceFile.h

    src/views/AbstractFileView.h
    src/views/FindBar.h
    src/views/ImageFileView.h
    src/views/PdfFileView.h
    src/views/SelectionHandles.h
    src/views/TextEditor.h
    src/views/TextFileView.h
    src/views/TextSearch.h
    src/views/ViewConstants.h
    src/views/ZoomControl.h
    src/views/ZoomState.h

    src/App.h
    src/LogView.h
)

# --- Resources ---

set(APP_RESOURCES
    resources/Dictionaries.qrc
    resources/Fonts.qrc
    resources/Icons.qrc
    resources/Licenses.qrc
    resources/Lucide.qrc
)

if(WIN32)
    list(APPEND APP_RESOURCES
        resources/Suzuri.rc
    )
elseif(APPLE)
    list(APPEND APP_RESOURCES
        resources/icons/Suzuri.icns
    )

    # Copied into Suzuri.app/Contents/Resources (see MACOSX_BUNDLE_ICON_FILE)
    set_source_files_properties(resources/icons/Suzuri.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
endif()

# =============================================================================
# Target
# =============================================================================

# NB: Each flag is ignored on the platforms it doesn't apply to
qt_add_executable(Suzuri WIN32 MACOSX_BUNDLE
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${APP_RESOURCES}
)

# --- macOS bundle ---

# NB: Unset, Qt fills in com.yourcompany.Suzuri and version 1.0.0. Qt's
# Info.plist template has no document types, so no file associations
if(APPLE)
    set_target_properties(Suzuri PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "${_pub_app_name}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${_pub_app_id}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION_NUMERIC}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION_NUMERIC}"
        MACOSX_BUNDLE_COPYRIGHT "${_pub_copyright}"
        MACOSX_BUNDLE_ICON_FILE Suzuri.icns
    )
endif()

# =============================================================================
# Build configuration
# =============================================================================

# NB: Even though some modules pull in others, I'd prefer to be explicit and
# always include what's used

target_include_directories(Suzuri PRIVATE
    src
)

if(WIN32)
    target_compile_definitions(Suzuri PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

target_compile_definitions(Suzuri PRIVATE
    APP_DEBUG=$<IF:$<CONFIG:Debug>,1,0>
)

# --- Linking ---

target_link_libraries(Suzuri PRIVATE
    Coco::Coco
    hunspell
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::PdfWidgets
    Qt6::Svg
    Qt6::Widgets
)

# --- Compiler options ---

if(MSVC)
    target_compile_options(Suzuri PRIVATE
        /W3 /MP /sdl /permissive- /bigobj /EHsc /Zc:preprocessor
    )
    target_compile_options(Suzuri PRIVATE
        $<$<CONFIG:Release>:/GL /Gy /Oi>
    )
    target_link_options(Suzuri PRIVATE
        $<$<CONFIG:Release>:/OPT:REF /OPT:ICF /LTCG>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # NB: Qt's include directories arrive as system includes, so these don't
    # report warnings from Qt's headers
    target_compile_options(Suzuri PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# =============================================================================
# Translations
# =============================================================================

# WIP

set(APP_TS_FILES
    translations/Translation_en.ts
    translations/Translation_es.ts
)

qt_add_lupdate(Suzuri
    TS_FILES translations/Translation_en.ts
    OPTIONS -pluralonly
)

qt_add_lupdate(Suzuri
    TS_FILES translations/Translation_es.ts
)

qt_add_lrelease(Suzuri
    TS_FILES ${APP_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

# Embedded, so every platform loads from the same path (e.g.
# `:/i18n/Translation_en.qm`) and nothing needs installing. lrelease writes the
# .qm files flat into the binary dir, which BASE strips from the alias
qt_add_resources(Suzuri "translations"
    PREFIX "/i18n"
    BASE ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${QM_FILES}
)

# =============================================================================
# Install
# =============================================================================

install(TARGETS Suzuri
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

if(UNIX AND NOT APPLE)
    # --- Desktop entry ---

    # Named for the app ID so it matches setDesktopFileName (Wayland app_id)
    set(_desktop_file ${CMAKE_CURRENT_BINARY_DIR}/${_pub_app_id}.desktop)

    configure_file(resources/linux/Suzuri.desktop.in ${_desktop_file} @ONLY)

    install(FILES ${_desktop_file}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )

    # --- Icons (hicolor theme tops out at 512; 1024/2048 are qrc-only) ---

    foreach(_size 16 22 24 32 48 64 128 256 512)
        install(FILES resources/icons/Suzuri-${_size}.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}x${_size}/apps
            RENAME ${_pub_app_id}.png
        )
    endforeach()
endif()

# =============================================================================
# Tests
# =============================================================================

option(APP_BUILD_TESTS "Build the tests in tests/" ON)

if(APP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

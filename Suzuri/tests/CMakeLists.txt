# Suzuri — A plain-text editor for creative writing
# Copyright (C) 2026 fairybow
#
# This program is free software, redistributable and/or modifiable under the
# terms of the GNU GPL v3. It's distributed in the hope that it will be useful
# but without any warranty (even the implied warranty of merchantability or
# fitness for a particular purpose)
#
# See the LICENSE file or visit <https://www.gnu.org/licenses/>

# Qt Test, one executable per test class. Each is a single .cpp named for its
# class, and includes the app's headers straight from src/.

find_package(Qt6 REQUIRED COMPONENTS Test)

# What any target here needs to compile the app's headers: the same include
# root, the same APP_DEBUG definition, Coco, Hunspell, the same Qt modules,
# and the same compiler options.
function(app_configure_test_target name)
    target_include_directories(${name} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )

    if(WIN32)
        target_compile_definitions(${name} PRIVATE
            NOMINMAX
            WIN32_LEAN_AND_MEAN
        )
    endif()

    target_compile_definitions(${name} PRIVATE
        APP_DEBUG=$<IF:$<CONFIG:Debug>,1,0>
    )

    target_link_libraries(${name} PRIVATE
        Coco::Coco
        hunspell
        Qt6::Core
        Qt6::Gui
        Qt6::PdfWidgets
        Qt6::Widgets
        Qt6::Test
    )

    if(MSVC)
        # NB: /Zc:preprocessor is required by Coco's ASSERT (__VA_OPT__)
        target_compile_options(${name} PRIVATE
            /W3 /MP /sdl /permissive- /bigobj /EHsc /Zc:preprocessor
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${name} PRIVATE
            -Wall -Wextra -Wpedantic
        )
    endif()
endfunction()

# --- Meta-object code for the app's QObject classes ---

# The app is header-only, but a class with Q_OBJECT also needs the code moc
# generates for it, and including the header doesn't provide that. AUTOMOC
# generates it for each header listed here, once, and every test links the
# result. A test of a QObject class not listed fails to LINK, naming the
# class's staticMetaObject: add its header, and the header of each QObject
# base class.
add_library(AppTestMocs OBJECT
    ${PROJECT_SOURCE_DIR}/src/core/Vault.h
    ${PROJECT_SOURCE_DIR}/src/core/VaultTreeModel.h
    ${PROJECT_SOURCE_DIR}/src/models/AbstractFileModel.h
    ${PROJECT_SOURCE_DIR}/src/models/ImageFileModel.h
    ${PROJECT_SOURCE_DIR}/src/models/PdfFileModel.h
    ${PROJECT_SOURCE_DIR}/src/models/PrimeDocument.h
    ${PROJECT_SOURCE_DIR}/src/models/TextFileModel.h
)

app_configure_test_target(AppTestMocs)

# --- Tests ---

# Builds <name>.cpp as a test executable and registers it with CTest.
function(app_add_test name)
    qt_add_executable(${name} ${name}.cpp)

    app_configure_test_target(${name})
    target_link_libraries(${name} PRIVATE AppTestMocs)

    add_test(NAME ${name} COMMAND ${name})

    # No display is needed (or present, on a build server)
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endfunction()

app_add_test(FileTypesTest)
app_add_test(MisspellingTest)
app_add_test(PrimeDocumentTest)
app_add_test(RecentFilesTest)
app_add_test(SpellCheckerTest)
app_add_test(SpellCheckersTest)
app_add_test(SpellWordsTest)
app_add_test(TextCountsTest)
app_add_test(TextFileModelTest)
app_add_test(TextSearchTest)
app_add_test(TranslationsTest)
app_add_test(TypingHelpersTest)
app_add_test(UpdateCheckTest)
app_add_test(VaultTest)
app_add_test(VaultTimedTest)
app_add_test(VaultTreeModelTest)
app_add_test(WordListTest)

# The dictionary that ships with the app, compiled into the one test that
# installs it
target_sources(SpellCheckersTest PRIVATE
    ${PROJECT_SOURCE_DIR}/resources/Dictionaries.qrc
)

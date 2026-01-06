cmake_minimum_required(VERSION 3.16)

project(Fernanda VERSION 0.99.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Xml Svg LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Xml Svg LinguistTools)
add_subdirectory(Fernanda/submodules/bit7z)

set(TS_FILES 
    Fernanda/translations/Translation_de.ts
    Fernanda/translations/Translation_es.ts  
    Fernanda/translations/Translation_fr.ts  
    Fernanda/translations/Translation_it.ts  
    Fernanda/translations/Translation_ja.ts  
    Fernanda/translations/Translation_zh.ts
)

set(PROJECT_SOURCES
    Fernanda/src/AboutDialog.cpp
    Fernanda/src/FnxModel.cpp
    Fernanda/src/Main.cpp
    Fernanda/src/old_FnxModel.cpp
    Fernanda/src/TextFileView.cpp
    Fernanda/src/Tr.cpp
    Fernanda/src/Window.cpp
    Fernanda/src/WindowService.cpp
    Fernanda/src/Workspace.cpp
    Fernanda/submodules/Coco/Coco/src/Fx.cpp
    Fernanda/submodules/Coco/Coco/src/Path.cpp
    Fernanda/submodules/Coco/Coco/src/PathUtil.cpp
    Fernanda/submodules/Coco/Coco/src/StartCop.cpp
    Fernanda/submodules/Coco/Coco/src/TextIo.cpp
    
    Fernanda/src/AccordionWidget.h
    Fernanda/src/BuildMessages.h
    Fernanda/src/FileService.h
    Fernanda/src/AbstractFileModel.h
    Fernanda/src/FileMeta.h
    Fernanda/src/TextFileModel.h
    Fernanda/src/AbstractFileView.h
    Fernanda/src/TextFileView.h
    Fernanda/src/NoOpFileView.h
    Fernanda/src/TabWidget.h
    Fernanda/src/ColorBarModule.h
    Fernanda/src/ColorBar.h
    Fernanda/src/AbstractService.h
    Fernanda/src/TabWidgetButton.h
    Fernanda/src/TabWidgetCloseButton.h
    Fernanda/src/TabWidgetTabBar.h
    Fernanda/src/TabWidgetUnderlay.h
    Fernanda/src/Commander.h
    Fernanda/src/CollapsibleWidget.h
    Fernanda/src/CollapsibleWidgetHeader.h
    Fernanda/src/Debug.h
    Fernanda/src/Timers.h
    Fernanda/src/DisplaySlider.h
    Fernanda/src/Bus.h
    Fernanda/src/Constants.h
    Fernanda/src/FileTypes.h
    Fernanda/src/NoOpFileModel.h
    Fernanda/src/AboutDialog.h
    Fernanda/src/FontSelector.h
    Fernanda/src/Fnx.h
    Fernanda/src/FnxModel.h
    Fernanda/src/FnxModelCache.h
    Fernanda/src/Notebook.h
    Fernanda/src/Notepad.h
    Fernanda/src/KeyFilter.h
    Fernanda/src/MenuBuilder.h
    Fernanda/src/MenuShortcuts.h
    Fernanda/src/MenuState.h
    Fernanda/src/NewNotebookPrompt.h
    Fernanda/src/TreeView.h
    Fernanda/src/old_FnxModel.h
    Fernanda/src/SaveFailMessageBox.h
    Fernanda/src/SavePrompt.h
    Fernanda/src/Formatters.h
    Fernanda/src/Settings.h
    Fernanda/src/Ini.h
    Fernanda/src/SettingsService.h
    Fernanda/src/SettingsDialog.h
    Fernanda/src/TempDir.h
    Fernanda/src/Io.h
    Fernanda/src/ToString.h
    Fernanda/src/Tr.h
    Fernanda/src/TreeViewService.h
    Fernanda/src/AppDirs.h
    Fernanda/src/Application.h
    Fernanda/src/TrashPrompt.h
    Fernanda/src/Version.h
    Fernanda/src/Workspace.h
    Fernanda/src/ViewService.h
    Fernanda/src/WindowService.h
    Fernanda/src/Window.h
    Fernanda/src/WordCounterModule.h
    Fernanda/src/XPlatform.h
    Fernanda/submodules/Coco/Coco/include/Coco/Bool.h
    Fernanda/submodules/Coco/Coco/include/Coco/Concepts.h
    Fernanda/submodules/Coco/Coco/include/Coco/Debug.h
    Fernanda/submodules/Coco/Coco/include/Coco/Fx.h
    Fernanda/submodules/Coco/Coco/include/Coco/Global.h
    Fernanda/submodules/Coco/Coco/include/Coco/Layout.h
    Fernanda/submodules/Coco/Coco/include/Coco/Log.h
    Fernanda/submodules/Coco/Coco/include/Coco/Macros.h
    Fernanda/submodules/Coco/Coco/include/Coco/Path.h
    Fernanda/submodules/Coco/Coco/include/Coco/PathUtil.h
    Fernanda/submodules/Coco/Coco/include/Coco/Private.h
    Fernanda/submodules/Coco/Coco/include/Coco/TextIo.h
    Fernanda/submodules/Coco/Coco/include/Coco/Utility.h
    Fernanda/submodules/Coco/Coco/include/Coco/StartCop.h
   
)



if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(fernanda
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        Fernanda/resources/Icons.qrc
        Fernanda/resources/Ui.qrc
        Fernanda/external/External.qrc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET fernanda2 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
    # qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    # add_custom_target(translations ALL
    #     DEPENDS ${QM_FILES}
    # )

    qt_add_lrelease(translations
        TS_FILES ${TS_FILES}
        QM_FILES_OUTPUT_VARIABLE QM_FILES
    )

    set(QRC_FILE "${CMAKE_BINARY_DIR}/translations.qrc")
    
    file(WRITE ${QRC_FILE}
        "<RCC>
        <qresource prefix=\"/i18n\">
        ")
    
    foreach(qm ${QM_FILES})
        file(APPEND ${QRC_FILE}
            "    <file>${qm}</file>\n")
    endforeach()
    
    file(APPEND ${QRC_FILE}
        "  </qresource>
        </RCC>
        ")

    qt_add_resources(PROJECT_SOURCES ${QRC_FILE})
#    add_dependencies(fernanda translations)
    
else()
    if(ANDROID)
        add_library(fernanda SHARED
            ${PROJECT_SOURCES}
        )
    # Define properties for Android with Qt 5 after find_package() calls as:
    #    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    add_executable(fernanda
        ${PROJECT_SOURCES}
    )
endif()
endif()

target_link_libraries(fernanda PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Network 
    Qt${QT_VERSION_MAJOR}::Xml 
    Qt${QT_VERSION_MAJOR}::Svg
    bit7z
)
target_include_directories(fernanda PRIVATE 
    Fernanda/submodules/Coco/Coco/include
    Fernanda/external/bit7z/include
)

target_sources(fernanda PRIVATE ${ui_res} ${icons_res})
# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.fernanda)
endif()
set_target_properties(fernanda PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS fernanda
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(fernanda)
endif()

if(WIN32)

    # ---- windeployqt ----
    get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake}" DIRECTORY)

    find_program(QT_DEPLOYQT_EXECUTABLE
        NAMES windeployqt6 windeployqt
        PATHS "${_qt_bin_dir}"
    )

    if(NOT QT_DEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "windeployqt not found")
    endif()

    # ---- Inno Setup ----
    find_program(INNO_SETUP_COMPILER
        NAMES ISCC.exe
        HINTS
            "$ENV{ProgramFiles\(x86\)}/Inno Setup 6"
            "$ENV{ProgramFiles}/Inno Setup 6"
        DOC "Inno Setup Compiler"
    )

    if(NOT INNO_SETUP_COMPILER)
        message(FATAL_ERROR "Inno Setup not found")
    endif()

    # ---- Paths ----
    set(QT_DEPLOY_DIR "${CMAKE_SOURCE_DIR}/Fernanda/installer/temp")
    set(ISS "${CMAKE_SOURCE_DIR}/Fernanda/installer/WindowsInstaller.iss")

    set(README "${CMAKE_SOURCE_DIR}/README.md")
    set(LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
    set(ADDITIONAL_TERMS "${CMAKE_SOURCE_DIR}/ADDITIONAL_TERMS")
    set(INSTALLER_NAME "FernandaInstaller")

    set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Fernanda/installer/output/Windows x64")

    # ---- Deploy Qt after build ----
    add_custom_command(TARGET fernanda POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${QT_DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${QT_DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:fernanda>" "${QT_DEPLOY_DIR}"

        COMMAND "${QT_DEPLOYQT_EXECUTABLE}"
                "$<TARGET_FILE:fernanda>"
                --release
                --no-translations
                --skip-plugin-types generic,networkinformation,tls
                --exclude-plugins qgif,qjpeg
                --no-system-d3d-compiler
                --no-opengl-sw
                --dir "${QT_DEPLOY_DIR}"

        COMMENT "Deploying Qt runtime"
    )

    # ---- Installer ----
    add_custom_target(installer
        COMMAND "${INNO_SETUP_COMPILER}"
            /DAppVersion=${PROJECT_VERSION}
            /DInstallerName=${INSTALLER_NAME}
            /DReadmePath="${README}"
            /DLicensePath="${LICENSE}"
            /DAdditionalTermsPath="${ADDITIONAL_TERMS}"
            /DOutputDir="${OUTPUT_DIR}"
            "${ISS}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Building Inno Setup installer"
    )

    add_dependencies(installer fernanda)

endif()

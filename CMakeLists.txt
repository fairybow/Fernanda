cmake_minimum_required(VERSION 3.16)

project(Fernanda VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Network PrintSupport WebChannel WebEngineCore WebEngineWidgets Widgets Xml)

qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(FetchContent)
FetchContent_Declare(bit7z
	GIT_REPOSITORY https://github.com/rikyoz/bit7z.git
	GIT_TAG        v4.0.0-rc
)
FetchContent_MakeAvailable(bit7z)

qt_add_executable(Fernanda
    
    source/Archiver.h
    source/ColorBar.h
    source/Delegate.h
    source/Dom.h
    source/Editor.h
    source/Icon.h
    source/Index.h
    source/Indicator.h
    source/Io.h
    source/KeyFilter.h
    source/Layout.h
    source/MainWindow.h
    source/Pane.h
    source/Path.h
    source/PlainTextEdit.h
    source/Popup.h
    source/Preview.h
    source/Resource.h
    source/Sample.h
    source/Splitter.h
    source/SplitterHandle.h
    source/StartCop.h
    source/StatusBarButton.h
    source/Story.h
    source/Style.h
    source/Text.h
    source/Tool.h
    source/UserData.h
    source/Version.h
    
    source/Archiver.cpp
    source/ColorBar.cpp
    source/Dom.cpp
    source/Editor.cpp
    source/Indicator.cpp
    source/KeyFilter.cpp
    source/Main.cpp
    source/MainWindow.cpp
    source/Pane.cpp
    source/Path.cpp
    source/PlainTextEdit.cpp
    source/Popup.cpp
    source/Preview.cpp
    source/Splitter.cpp
    source/Story.cpp
    source/Text.cpp
    source/Tool.cpp
    source/UserData.cpp
    
    resources/fonts.qrc
    resources/icons.qrc
    resources/lib.qrc
    resources/preview.qrc
    resources/resource.rc
    resources/sample.qrc
    resources/themes.qrc
)

target_include_directories(Fernanda PRIVATE include)
target_include_directories(Fernanda PRIVATE source)
target_link_libraries(Fernanda PRIVATE Qt6::Network)
target_link_libraries(Fernanda PRIVATE Qt6::PrintSupport)
target_link_libraries(Fernanda PRIVATE Qt6::WebChannel)
target_link_libraries(Fernanda PRIVATE Qt6::WebEngineCore)
target_link_libraries(Fernanda PRIVATE Qt6::WebEngineWidgets)
target_link_libraries(Fernanda PRIVATE Qt6::Widgets)
target_link_libraries(Fernanda PRIVATE Qt6::Xml)

target_link_libraries(Fernanda PRIVATE bit7z64)

install(TARGETS Fernanda RUNTIME)

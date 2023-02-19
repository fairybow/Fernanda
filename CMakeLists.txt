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
    
    src/Archiver.h
    src/ColorBar.h
    src/Delegate.h
    src/Dom.h
    src/Editor.h
    src/Icon.h
    src/Index.h
    src/Indicator.h
    src/Io.h
    src/KeyFilter.h
    src/Layout.h
    src/MainWindow.h
    src/Pane.h
    src/Path.h
    src/PlainTextEdit.h
    src/Popup.h
    src/Preview.h
    src/Resource.h
    src/Sample.h
    src/Splitter.h
    src/SplitterHandle.h
    src/StartCop.h
    src/StatusBarButton.h
    src/Story.h
    src/Style.h
    src/Text.h
    src/Tool.h
    src/UserData.h
    src/Version.h
    
    src/Archiver.cpp
    src/ColorBar.cpp
    src/Dom.cpp
    src/Editor.cpp
    src/Indicator.cpp
    src/KeyFilter.cpp
    src/Main.cpp
    src/MainWindow.cpp
    src/Pane.cpp
    src/Path.cpp
    src/PlainTextEdit.cpp
    src/Popup.cpp
    src/Preview.cpp
    src/Splitter.cpp
    src/Story.cpp
    src/Text.cpp
    src/Tool.cpp
    src/UserData.cpp
    
    res/fonts.qrc
    res/icons.qrc
    res/lib.qrc
    res/preview.qrc
    res/resource.rc
    res/sample.qrc
    res/themes.qrc
)

target_include_directories(Fernanda PRIVATE include)
target_include_directories(Fernanda PRIVATE src)
target_link_libraries(Fernanda PRIVATE Qt6::Network)
target_link_libraries(Fernanda PRIVATE Qt6::PrintSupport)
target_link_libraries(Fernanda PRIVATE Qt6::WebChannel)
target_link_libraries(Fernanda PRIVATE Qt6::WebEngineCore)
target_link_libraries(Fernanda PRIVATE Qt6::WebEngineWidgets)
target_link_libraries(Fernanda PRIVATE Qt6::Widgets)
target_link_libraries(Fernanda PRIVATE Qt6::Xml)

target_link_libraries(Fernanda PRIVATE bit7z64)

install(TARGETS Fernanda RUNTIME)

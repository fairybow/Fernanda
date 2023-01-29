cmake_minimum_required(VERSION 3.16)

project(fernanda VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Network PrintSupport Widgets Xml)

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

qt_add_executable(fernanda
    
    src/archiver.h
    src/colorbar.h
    src/delegate.h
    src/dom.h
    src/editor.h
    src/fernanda.h
    src/icon.h
    src/index.h
    src/indicator.h
    src/io.h
    src/keyfilter.h
    src/layout.h
    src/pane.h
    src/path.h
    src/plaintextedit.h
    src/popup.h
    src/resource.h
    src/sample.h
    src/splitter.h
    src/startcop.h
    src/story.h
    src/style.h
    src/text.h
    src/tool.h
    src/userdata.h
    src/version.h
    
    src/archiver.cpp
    src/colorbar.cpp
    src/dom.cpp
    src/editor.cpp
    src/fernanda.cpp
    src/indicator.cpp
    src/keyfilter.cpp
    src/main.cpp
    src/pane.cpp
    src/plaintextedit.cpp
    src/popup.cpp
    src/story.cpp
    src/text.cpp
    src/tool.cpp
    src/userdata.cpp
    
    res/fonts.qrc
    res/icons.qrc
    res/lib.qrc
    res/resource.rc
    res/sample.qrc
    res/themes.qrc
)

target_include_directories(fernanda PRIVATE include)
target_include_directories(fernanda PRIVATE src)
target_link_libraries(fernanda PRIVATE Qt6::Network)
target_link_libraries(fernanda PRIVATE Qt6::PrintSupport)
target_link_libraries(fernanda PRIVATE Qt6::Widgets)
target_link_libraries(fernanda PRIVATE Qt6::Xml)

target_link_libraries(fernanda PRIVATE bit7z64)

install(TARGETS fernanda RUNTIME)

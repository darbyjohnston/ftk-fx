set(HEADERS
    App.h
    CacheBar.h
    MainWindow.h
    ParametersPanel.h
    SceneModel.h
    TimelineBar.h
    Viewport.h)

set(SOURCE
    App.cpp
    CacheBar.cpp
    MainWindow.cpp
    ParametersPanel.cpp
    SceneModel.cpp
    TimelineBar.cpp
    Viewport.cpp)

add_library(fxApp ${HEADERS} ${SOURCE})

target_include_directories(fxApp
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(fxApp PUBLIC fxSim ftkUI ftkGL)

set_target_properties(fxApp PROPERTIES FOLDER lib)

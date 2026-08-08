set(HEADERS
    App.h
    CacheBar.h
    Capture.h
    DiagPanel.h
    IPanel.h
    MainWindow.h
    Panels.h
    ParametersPanel.h
    SceneModel.h
    TimelineBar.h
    ViewOptions.h
    Viewport.h
    Views.h)

set(SOURCE
    App.cpp
    CacheBar.cpp
    Capture.cpp
    DiagPanel.cpp
    IPanel.cpp
    MainWindow.cpp
    Panels.cpp
    ParametersPanel.cpp
    SceneModel.cpp
    TimelineBar.cpp
    ViewOptions.cpp
    Viewport.cpp
    Views.cpp)

add_library(fxApp ${HEADERS} ${SOURCE})

target_include_directories(fxApp
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(fxApp PUBLIC fxSim ftkUI ftkGL)

set_target_properties(fxApp PROPERTIES FOLDER lib)

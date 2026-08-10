set(HEADERS
    App.h
    CacheBar.h
    Capture.h
    DiagPanel.h
    IPanel.h
    MainWindow.h
    Pane.h
    CurveEditor.h
    CurveEditorPrivate.h
    Icons.h
    ParameterList.h
    PaneOptions.h
    PanePlaceholder.h
    Panels.h
    Panes.h
    ParametersPanel.h
    SceneModel.h
    TimelineBar.h
    Viewport.h)

set(SOURCE
    App.cpp
    CacheBar.cpp
    Capture.cpp
    DiagPanel.cpp
    IPanel.cpp
    MainWindow.cpp
    Pane.cpp
    CurveEditor.cpp
    CurveGraph.cpp
    Icons.cpp
    ParameterList.cpp
    PaneOptions.cpp
    PanePlaceholder.cpp
    Panels.cpp
    Panes.cpp
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

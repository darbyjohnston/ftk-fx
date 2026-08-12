set(HEADERS
    App.h
    CacheBar.h
    Capture.h
    DiagPanel.h
    IPanel.h
    MainWindow.h
    Editor.h
    CurveEditor.h
    CurveEditorPrivate.h
    Icons.h
    ParameterList.h
    EditorOptions.h
    EditorPlaceholder.h
    Panels.h
    Editors.h
    ParametersPanel.h
    SystemsPanel.h
    SystemsPanelPrivate.h
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
    Editor.cpp
    CurveEditor.cpp
    CurveGraph.cpp
    Icons.cpp
    ParameterList.cpp
    EditorOptions.cpp
    EditorPlaceholder.cpp
    Panels.cpp
    Editors.cpp
    ParametersPanel.cpp
    SystemRow.cpp
    SystemsPanel.cpp
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

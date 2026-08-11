set(HEADERS
    Scene.h
    System.h
    Transform.h)

set(SOURCE
    Scene.cpp
    System.cpp
    Transform.cpp)

add_library(fxSim ${HEADERS} ${SOURCE})

target_include_directories(fxSim
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(fxSim PUBLIC fxCore)

set_target_properties(fxSim PROPERTIES FOLDER lib)

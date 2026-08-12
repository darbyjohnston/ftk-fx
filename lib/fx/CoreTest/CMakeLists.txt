set(HEADERS
    CacheTest.h
    CurveTest.h
    PoolTest.h
    SceneTest.h
    SystemTest.h
    TransformTest.h)

set(SOURCE
    CacheTest.cpp
    CurveTest.cpp
    PoolTest.cpp
    SceneTest.cpp
    SystemTest.cpp
    TransformTest.cpp)

add_library(fxCoreTest ${HEADERS} ${SOURCE})

target_include_directories(fxCoreTest
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(fxCoreTest PUBLIC fxSim ftkTestLib)

set_target_properties(fxCoreTest PROPERTIES FOLDER lib)

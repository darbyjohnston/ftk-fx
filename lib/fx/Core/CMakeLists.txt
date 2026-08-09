set(HEADERS
    Cache.h
    Curve.h
    Frame.h
    Parameter.h
    Pool.h
    Serialize.h
    Rand.h
    Version.h)

set(SOURCE
    Cache.cpp
    Curve.cpp
    Frame.cpp
    Parameter.cpp
    Pool.cpp
    Serialize.cpp
    Version.cpp)

add_library(fxCore ${HEADERS} ${SOURCE})

target_include_directories(fxCore
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(fxCore PUBLIC ftkCore)

set_target_properties(fxCore PROPERTIES FOLDER lib)

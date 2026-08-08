add_executable(fx-test fx-test.h fx-test.cpp)

target_include_directories(fx-test
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(fx-test fxCoreTest)

set_target_properties(fx-test PROPERTIES FOLDER tests)

add_test(NAME fx-test COMMAND fx-test)

# feather-tk and tlRender each find zlib for themselves; the imported target
# an inner find_package() makes does not reach out here, so this one does too.
find_package(ZLIB REQUIRED)

set(HEADERS)
set(SOURCE)

# The icons this application adds to the ones feather-tk ships. Each is
# compiled into the binary by ftk-resource, the same way feather-tk and
# tlRender carry theirs, so that an installed ftk-fx is one file and an icon
# cannot go missing between the build and the run.
set(RESOURCES
    etc/Icons/Key.svg
    etc/Icons/LayoutFour.svg
    etc/Icons/LayoutSingle.svg
    etc/Icons/LayoutThree.svg
    etc/Icons/LayoutTwo.svg
    etc/Icons/Rotate.svg
    etc/Icons/Scale.svg
    etc/Icons/SystemAdd.svg
    etc/Icons/SystemDuplicate.svg
    etc/Icons/SystemRemove.svg
    etc/Icons/Translate.svg)
foreach(RESOURCE ${RESOURCES})
    get_filename_component(RESOURCE_BASE ${RESOURCE} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_BASE}.cpp
        COMMAND ftk-resource ${PROJECT_SOURCE_DIR}/${RESOURCE} ${RESOURCE_BASE} fx_resource
        DEPENDS ${PROJECT_SOURCE_DIR}/${RESOURCE})
    list(APPEND SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_BASE}.cpp)
endforeach()

add_library(fxResource ${HEADERS} ${SOURCE})

target_link_libraries(fxResource ZLIB::ZLIB)

set_target_properties(fxResource PROPERTIES FOLDER lib)

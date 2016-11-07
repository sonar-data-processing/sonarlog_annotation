cmake_minimum_required (VERSION 2.8)
project (sonarlog_annotation)

include ( "${WORKSPACE_DIR}/base/cmake/Dependencies.cmake"  )
include ( FindPkgConfig  )

add_project_dependency (base              ${WORKSPACE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_project_dependency (rock_util         ${WORKSPACE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_project_dependency (sonar_processing  ${WORKSPACE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_project_dependency (image_picker_tool ${WORKSPACE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# setup_headers ( sonar_processing ${sonar_processing_SOURCE_DIR} )
setup_headers ( ${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/src" )

find_package ( Boost COMPONENTS system filesystem program_options iostreams REQUIRED )
find_package ( Eigen3 REQUIRED )
find_package ( OpenCV REQUIRED )
find_package ( Qt4 REQUIRED )

include( ${QT_USE_FILE} )
add_definitions(${QT_DEFINITIONS})

pkg_check_modules(pocolog_cpp pocolog_cpp)

set(
    sonarlog_annotation_HEADERS_MOC
    src/AnnotationWindow.hpp
)

qt4_wrap_cpp( sonarlog_annotation_MOC_CPP ${sonarlog_annotation_HEADERS_MOC} )

set (
    ${PROJECT_NAME}_INCLUDE_DIR
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/include
    CACHE PATH "Sonar log annotation headers"
    FORCE
)

set (
    LIBS
    rock_util
    base
    image_picker_tool
    ${pocolog_cpp_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OpenCV_LIBS}
    ${QT_LIBRARIES}
)


include_directories (
    ${sonarlog_annotation_INCLUDE_DIR}
    ${base_INCLUDE_DIR}
    ${rock_util_INCLUDE_DIR}
    ${gnuplot_INCLUDE_DIR}
    ${sonar_processing_INCLUDE_DIR}
    ${ROCK_ROBOTICS_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
)

link_directories (
    ${ROCK_ROBOTICS_LIBRARY_DIR}
)


add_library (
    annotation_filereader STATIC
    src/AnnotationFileReader.cpp
)

target_link_libraries (
    annotation_filereader
    ${OpenCV_LIBS}
)

add_executable (
    sonarlog_annotation
    src/main.cpp
    src/AnnotationWindow.cpp
    ${sonarlog_annotation_MOC_CPP}
    ${sonar_processing_SOURCES}
)

target_link_libraries (
    sonarlog_annotation
    ${LIBS}
    annotation_filereader
)

install(
    TARGETS sonarlog_annotation sonarlog_annotation
    DESTINATION ${CMAKE_SOURCE_DIR}/bin
)

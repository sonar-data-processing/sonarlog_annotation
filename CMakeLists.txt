cmake_minimum_required (VERSION 2.8)
project (sonarlog_annotation)

include ( FindPkgConfig  )

find_package ( Boost COMPONENTS system filesystem program_options iostreams REQUIRED )
find_package ( Eigen3 REQUIRED )
find_package ( OpenCV REQUIRED )
find_package ( Qt4 REQUIRED )

file ( GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp )
file ( GLOB HEADERS ${PROJECT_SOURCE_DIR}/src/*.hpp )

include( ${QT_USE_FILE} )
add_definitions(${QT_DEFINITIONS})

pkg_check_modules(pocolog_cpp pocolog_cpp)

set(
    ${PROJECT_NAME}_HEADERS_MOC
    src/AnnotationWindow.hpp
)

qt4_wrap_cpp( sonarlog_annotation_MOC_CPP ${sonarlog_annotation_HEADERS_MOC} )

include_directories (
    ${CMAKE_INSTALL_PREFIX}/include/
    ${CMAKE_INSTALL_PREFIX}/include/sonar_toolkit
    ${EIGEN3_INCLUDE_DIR}
)

link_directories (
    ${CMAKE_INSTALL_PREFIX}/lib/
    ${CMAKE_INSTALL_PREFIX}/lib/sonar_toolkit
)

add_library (
    annotation_filereader SHARED
    src/AnnotationFileReader.cpp
)

target_link_libraries (
    annotation_filereader
    ${OpenCV_LIBS}
)

add_executable (
    sonarlog-annotation
    src/main.cpp
    src/AnnotationWindow.cpp
    ${sonarlog_annotation_MOC_CPP}
)

target_link_libraries (
    sonarlog-annotation
    annotation_filereader
    sonar_processing
    rock_util
    sonar_util
    image_picker_tool
    ${QT_LIBRARIES}
    ${pocolog_cpp_LIBRARIES}
)

install(
    FILES ${HEADERS}
    DESTINATION include/sonar_toolkit/${PROJECT_NAME}
)

install(
    TARGETS annotation_filereader
    DESTINATION lib/sonar_toolkit
)

install(
    TARGETS sonarlog-annotation
    DESTINATION bin
)

cmake_minimum_required(VERSION 3.14)


set(PROJECT_NAME "bayan")

project(${PROJECT_NAME} LANGUAGES CXX)
set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})
set(PROJECT_VERSION_PATCH ${PATCH_VERSION})
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
#set(CMAKE_CXX_FLAGS "-Wall -Wextra -O1")
#set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")


#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Boost 1.30 REQUIRED COMPONENTS program_options system filesystem)

add_executable(${PROJECT_NAME}
    main.cpp
    lib.hpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR})

#configure_file(ip_filter.tsv ${CMAKE_CURRENT_BINARY_DIR}/ip_filter.tsv COPYONLY)

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_VERBATIM_VARIABLES ON)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION "Configure test_ci_cd and launch")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Maintainer <mm@example.com>")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/Debian/postinst")

set(CPACK_DEBIAN_PACKAGE_DEPENDS libgtest-dev libgmock-dev)

target_link_libraries(
    ${PROJECT_NAME} PUBLIC
#    Qt${QT_VERSION_MAJOR}::Core
    profiler
    pthread # required
    ${Boost_LIBRARIES}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
#install(FILES ip_filter.tsv DESTINATION bin)


set(CPACK_GENERATOR DEB)

include(CPack)


add_subdirectory(tests)

enable_testing()


add_test(NAME test_main_google COMMAND test_main_google)

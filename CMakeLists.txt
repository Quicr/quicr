cmake_minimum_required(VERSION 3.10)

project( quicr
 VERSION 0.1
 LANGUAGES C CXX
 )

###
### Global Config
###
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -pedantic -Wextra)
elseif(MSVC)
    add_compile_options(/W4 /WX)
endif()

###
### Dependencies
###
find_package(Threads REQUIRED) 


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

###
### Library Source
###

###
### Library Config
###
set( LIBRARY_NAME quicr )


file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh") 
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

 
add_library( quicr ${LIB_HEADERS} ${LIB_SOURCES} )

if(WIN32)
    target_link_libraries( quicr PRIVATE wsock32 ws2_32 )
endif()

target_link_libraries( quicr PRIVATE Threads::Threads)


target_include_directories(${LIBRARY_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${LIBRARY_NAME}>
    $<INSTALL_INTERFACE:include/${LIBRARY_NAME}>
)

target_include_directories(${LIBRARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)


###
### Applications
###

add_subdirectory(cmd)
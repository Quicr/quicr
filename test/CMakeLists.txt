set(TEST_APP_NAME "${LIBRARY_NAME}_test")

# Dependencies
find_package(doctest REQUIRED)

# Test Binary
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(${TEST_APP_NAME} ${TEST_SOURCES})
add_dependencies(${TEST_APP_NAME} ${LIBRARY_NAME})
target_link_libraries(${TEST_APP_NAME} ${LIBRARY_NAME} gsl doctest::doctest OpenSSL::Crypto)

# Enable CTest
include(doctest)
doctest_discover_tests(${TEST_APP_NAME})
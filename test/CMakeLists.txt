# Macro to create individual test executables
macro(add_littorrent_test test_name)
    set(source_files ${ARGN})  # Capture additional arguments as source files
    
    MESSAGE("Test name: ${test_name}")
    MESSAGE("Source files: ${source_files}")
    
    # Create executable with test file and additional sources
    add_executable(${test_name} 
        ${test_name}.cpp
        ${source_files}
    )
    
    # Link against GTest and project dependencies
    target_link_libraries(${test_name}
        GTest::gtest_main
        GTest::gmock_main
    )
    
    # Include directories for tests
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/LitTorrent
    )
endmacro()

# Define test cases with their dependencies
add_littorrent_test(BEncodedValue_test
    ${CMAKE_SOURCE_DIR}/src/LitTorrent/BEncoding/BEncodingImpl.cpp
    ${CMAKE_SOURCE_DIR}/src/LitTorrent/BEncoding/BEncoding.cpp
)

add_littorrent_test(BEncoding_test
    ${CMAKE_SOURCE_DIR}/src/LitTorrent/BEncoding/BEncodingImpl.cpp
    ${CMAKE_SOURCE_DIR}/src/LitTorrent/BEncoding/BEncoding.cpp
)

add_littorrent_test(HTTPUtils_test
    ${CMAKE_SOURCE_DIR}/src/Utils/HTTPUtils.cpp
)


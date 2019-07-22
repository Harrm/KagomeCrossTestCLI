cmake_minimum_required(VERSION 3.14)
project(KagomeCrossTestCLI)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.70 REQUIRED COMPONENTS program_options)
find_package(OpenSSL REQUIRED)
find_package(leveldb REQUIRED)
find_package(kagome)

add_executable(KagomeCrossTestCLI main.cpp)
message(STATUS ${kagome_INCLUDE_DIRS})
target_include_directories(KagomeCrossTestCLI PUBLIC ${kagome_INCLUDE_DIRS})
target_link_libraries(KagomeCrossTestCLI Boost::program_options ${kagome_LIBRARIES})

if (TESTING)
    enable_testing()

    set(EXECUTABLE ${CMAKE_BINARY_DIR}/KagomeCrossTestCLI)
    add_test(ScaleTest1 ${EXECUTABLE} scale-codec encode -i 123456)
    add_test(ScaleTest2 ${EXECUTABLE} scale-codec encode -i "a b c d")
    add_test(ScaleTest3 ${EXECUTABLE} scale-codec encode --input "Hello, world")
endif ()

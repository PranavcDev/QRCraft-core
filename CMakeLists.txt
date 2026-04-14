cmake_minimum_required(VERSION 3.16)
project(qr_generator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()


add_executable(qr_generator
    src/main.cpp
    src/ModeEncoder.h
    src/NumericEncoder.h
    src/AlphanumericEncoder.h
    src/ByteEncoder.h
    src/CodewordUtils.h
    src/DataInterleaving.h
    src/ErrorCorrection.h
    src/MatrixUtils.h
    src/PaddingUtils.h
    src/VersionUtils.h
    src/Renderer.h
    src/VersionBlocks.h
    src/stb_image_write.h
)

target_link_libraries(qr_generator PRIVATE)

# Test executable
add_executable(test_ecc tests/test_ecc.cpp src/ErrorCorrection.h)

add_test(NAME ecc_test COMMAND test_ecc)


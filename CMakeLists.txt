cmake_minimum_required(VERSION 3.16)
project(qr_generator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)




add_executable(qr_generator
    src/main.cpp
    src/ModeEncoder.hpp
    src/NumericEncoder.hpp
    src/AlphanumericEncoder.hpp
    src/ByteEncoder.hpp
    src/CodewordUtils.hpp
    src/DataInterleaving.hpp
    src/ErrorCorrection.hpp
    src/MatrixUtils.hpp
    src/PaddingUtils.hpp
    src/VersionUtils.hpp
    src/Renderer.hpp
    src/VersionBlocks.hpp
    src/stb_image_write.hpp
)

target_link_libraries(qr_generator PRIVATE)




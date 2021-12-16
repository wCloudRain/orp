# CMakeLists.txt example for STXXL

project(my-project)
cmake_minimum_required(VERSION 2.8)

# build a program and link it with STXXL.
add_executable(project example/main.cpp include/murmurhash3.cpp include/murmurhash3.h utils/permutation.h utils/server.h headers/waksman.h alg/bitonic.cpp headers/bitonic.h alg/melbshuffle.cpp headers/melbshuffle.h headers/ORP.h alg/waksman.cpp alg/bucket.cpp headers/bucket.h)


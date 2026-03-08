add_executable(phase3 main.cpp)
target_compile_options(phase3 PRIVATE -O3 -march=native -ftree-vectorize)
target_link_libraries(phase3 PRIVATE OpenMP::OpenMP_CXX)

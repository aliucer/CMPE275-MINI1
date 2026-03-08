add_executable(phase2 main.cpp)
target_compile_options(phase2 PRIVATE -O3 -march=native)
target_link_libraries(phase2 PRIVATE OpenMP::OpenMP_CXX)

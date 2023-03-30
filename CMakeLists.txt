cmake_minimum_required(VERSION 3.0)

project(bwa-mem3)
add_subdirectory(ext/safestringlib)

set(CMAKE_CXX_STANDARD 11)

include_directories(src)
include_directories(ext/safestringlib/include)

add_compile_options(-mavx512bw 
                    -DENABLE_PREFETCH 
                    -DDEB=0 -DRDT=0 -DMAXI=0 -DNEW=1 -DSORT_PAIRS=0
                    -DPAIRED_END=1 -DMAINY=0 -DSAIS=1)

aux_source_directory(src SRC)
list(REMOVE_ITEM SRC src/runsimd.cpp)
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} pthread m z safestring_static rt)


install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin 
)

cmake_minimum_required(VERSION 3.0)

project(bwa-mem2-shm)
add_subdirectory(ext/safestringlib)

set(CMAKE_CXX_STANDARD 11)

include_directories(src)
include_directories(ext/safestringlib/include)

aux_source_directory(src SRC)
list(REMOVE_ITEM SRC src/runsimd.cpp)

set(common_options ${COMPILE_OPTIONS} -DENABLE_PREFETCH 
                   -DDEB=0 -DRDT=0 -DMAXI=0 -DNEW=1 -DSORT_PAIRS=0
                   -DPAIRED_END=1 -DMAINY=0 -DSAIS=1)
set(archs sse4.1 sse4.2 avx avx2 avx512bw avx512f)

foreach(x IN LISTS abc)
    message(${x})
endforeach()

foreach(arch IN LISTS archs)
    add_compile_options(${common_options} -m${arch})
    add_executable(${PROJECT_NAME}.${arch} ${SRC})
    target_link_libraries(${PROJECT_NAME}.${arch} pthread m z safestring_static rt)
endforeach()
add_executable(${PROJECT_NAME} src/runsimd.cpp)


set(install_targets ${PROJECT_NAME})
foreach(arch IN LISTS archs)
    list(APPEND install_targets ${PROJECT_NAME}.${arch})
endforeach()

install(
    TARGETS ${install_targets}
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin 
)

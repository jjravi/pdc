# DYnamic Plugins are compiled separately from PDC.

project(pdc_plugins C CXX CUDA)

set(CMAKE_CUDA_ARCHITECTURES 60 61 62 70 72 75 80 86)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --extended-lambda --expt-relaxed-constexpr -Wno-deprecated-declarations")
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CXX_STANDARD 14)

# find_package(HDF5 NO_MODULE NAMES hdf5 COMPONENTS C shared REQUIRED)
find_package(HDF5 NO_MODULE NAMES hdf5 COMPONENTS C shared REQUIRED PATHS $ENV{HDF5_DIR})
message(STATUS "HDF5_FOUND: ${HDF5_FOUND}")
message(STATUS "HDF5_VERSION: ${HDF5_VERSION}")
message(STATUS "HDF5_INCLUDE_DIR: ${HDF5_INCLUDE_DIR}")
message(STATUS "HDF5_LIBRARIES: ${HDF5_LIBRARIES}")

set(PDC_DIR $ENV{PDC_DIR})
message(STATUS "PDC_DIR: ${PDC_DIR}")

add_subdirectory(external/SZ3 EXCLUDE_FROM_ALL)
add_subdirectory(external/cuSZ EXCLUDE_FROM_ALL)

include_directories(
  external/SZ3/include/SZ3/
  external/cuSZ/src/
  ${PDC_DIR}/../
  ${PDC_DIR}/src/utils/include
  ${PDC_DIR}/src/api/include
  $ENV{BLOSC_DIR}/include
  $ENV{HOME}/include
  ${HDF5_INCLUDE_DIR}
  /opt/nvidia/nsight-systems/2022.4.1/target-linux-x64/nvtx/include
)

link_directories(${PDC_DIR}/lib)

# enable_language(CXX)
find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})
# add_executable(vpicio_compress vpicio_compress.cc)
# target_link_libraries(vpicio_compress pdc MPI::MPI_C MPI::MPI_CXX)
# add_executable(nyx_sdrbench nyx_sdrbench.cc)
# target_link_libraries(nyx_sdrbench pdc MPI::MPI_C MPI::MPI_CXX)
#
# # *******************************************
# # Create a transform library which contains:
# #  1. compression/depression functions
# #  2. datatype conversion
# # *******************************************
# OPTION(GEN_TRANSFORM_LIB "Create a transform library" ON)
# if(GEN_TRANSFORM_LIB)
#   FIND_LIBRARY(BLOSC_LIBRARY blosc $ENV{BLOSC_DIR}/lib)
#   set(PDC_TRANSFORMS_SRCS pdc_transforms_lib.c)
#   add_library(pdctransforms  ${PDC_TRANSFORMS_SRCS})
#   target_link_libraries(pdctransforms pdc ${BLOSC_LIBRARY})
#   # target_link_libraries(neon-read3d HDF5::HDF5)
#   # target_link_libraries(neon-read3d ${HDF5_LIBRARIES})
#   target_link_libraries(neon-read3d hdf5-shared)
#   target_link_libraries(s3d-read3d hdf5-shared)
#   target_link_libraries(openmsi-read3d hdf5-shared)
#   target_link_libraries(nyx-read3d hdf5-shared)
# endif()

############################################################################
# PDC Transform Plugins naming convention:
# libpdc_transform_<name>.so
############################################################################

add_library(pdc_server_transform_test SHARED server_plugin_test.cc)
target_include_directories(pdc_server_transform_test PUBLIC ${PDC_DIR}/include)
target_link_libraries(pdc_server_transform_test pdc MPI::MPI_C MPI::MPI_CXX)
target_link_libraries(pdc_server_transform_test "-Wl,--no-undefined")

add_library(pdc_transform_test SHARED plugin_test.cc)
target_include_directories(pdc_transform_test PUBLIC ${PDC_DIR}/include/)
target_link_libraries(pdc_transform_test pdc)

add_library(pdc_transform_sz SHARED plugin_sz.cc)
target_include_directories(pdc_transform_sz PUBLIC ${PDC_DIR}/include/)
target_link_libraries(pdc_transform_sz pdc)
target_link_libraries(pdc_transform_sz SZ3)
target_link_libraries(pdc_transform_sz MPI::MPI_C MPI::MPI_CXX)

add_library(pdc_transform_cusz SHARED plugin_cusz.cu)
target_include_directories(pdc_transform_cusz PUBLIC ${PDC_DIR}/include/)
target_include_directories(pdc_transform_cusz PUBLIC ../../external/cuSZ/src)
target_link_libraries(pdc_transform_cusz pdc)
target_link_libraries(pdc_transform_cusz cusz compress argp huff sp pq) 
# target_link_libraries(pdc_transform_cusz -lcuda -lcuspare)
target_link_libraries(pdc_transform_cusz -lcuda)
# target_link_libraries(pdc_transform_cusz -fPIC)
target_link_libraries(pdc_transform_cusz MPI::MPI_C MPI::MPI_CXX)
set_target_properties(pdc_transform_cusz PROPERTIES CUDA_SEPARABLE_COMPILATION ON)


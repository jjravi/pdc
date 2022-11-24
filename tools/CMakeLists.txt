cmake_minimum_required (VERSION 2.8.12)

project(PDC_VOL C)

# /home/jjravi/hpc-io/pdc/build/install/share/cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../build/install/share/cmake)
message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
include_directories(../build/)

# data6
include_directories(/opt/nvidia/nsight-systems/2021.2.4/target-linux-x64/nvtx/include)

# jjravi
include_directories(/opt/nvidia/nsight-systems/2022.2.1/target-linux-x64/nvtx/include)

include_directories(/opt/nvidia/nsight-systems/2022.4.1/target-linux-x64/nvtx/include)


include_directories(
  ..//src/server/include
  ..//src/server/pdc_server_region
  ..//src/server/pdc_server_region/include
  ..//src/server/pdc_server_analysis/include
  ..//src/server/pdc_server_transform/include
  ..//src/server/dablooms
  ..//src/api/include
  ..//src/api/pdc_obj/include
  ..//src/api/pdc_region/include
  ..//src/api/pdc_query/include
  ..//src/api/pdc_transform/include
  ..//src/api/pdc_transform_old/include
  ..//src/api/pdc_analysis/include
  ..//src/api/profiling/include
  ..//src/utils/include
  ..//src/api/pdc_persist/include
)

include_directories(
  ${PDC_EXT_INCLUDE_DEPENDENCIES}
)

find_package(PDC REQUIRED ${CMAKE_CURRENT_SOURCE_DIR}/../build/)
if(PDC_FOUND)
    #message(STATUS "PDC include directory: ${PDC_INCLUDE_DIR}")
  set(PDC_EXT_INCLUDE_DEPENDENCIES ${PDC_INCLUDE_DIR}
    ${PDC_EXT_INCLUDE_DEPENDENCIES}
    )
  set(PDC_EXT_LIB_DEPENDENCIES pdc ${PDC_EXT_LIB_DEPENDENCIES})
endif()

find_package(HDF5 NO_MODULE NAMES hdf5 COMPONENTS C shared REQUIRED PATHS $ENV{HDF5_DIR})
message(STATUS "HDF5_FOUND: ${HDF5_FOUND}")
message(STATUS "HDF5_VERSION: ${HDF5_VERSION}")
message(STATUS "HDF5_INCLUDE_DIR: ${HDF5_INCLUDE_DIR}")
message(STATUS "HDF5_LIBRARIES: ${HDF5_LIBRARIES}")
include_directories(${HDF5_INCLUDE_DIR})

# Mercury
find_package(MERCURY REQUIRED)
if(MERCURY_FOUND)
  message(STATUS "mercury dir ${MERCURY_DIR}")
  # set(PDC_EXT_INCLUDE_DEPENDENCIES ${MERCURY_INCLUDE_DIR}
  #   ${PDC_EXT_INCLUDE_DEPENDENCIES}
  #   )
  # set(PDC_EXT_LIB_DEPENDENCIES mercury ${PDC_EXT_LIB_DEPENDENCIES})
endif()

add_definitions(-DENABLE_MPI=1)
add_library(cjson cjson/cJSON.c)

set(PROGRAMS
  pdc_ls
  # pdc_import
  # pdc_export
  )

foreach(program ${PROGRAMS})
  add_executable(${program} ${program}.c)
  # target_link_libraries(${program} ${PDC_EXT_LIB_DEPENDENCIES})
  target_link_libraries(${program} mercury)
  # target_link_libraries(${program} MPI::MPI_C)
  target_link_libraries(${program} hdf5-shared)
  target_link_libraries(${program} cjson)
  # target_link_libraries(${program} pdc)
endforeach(program)


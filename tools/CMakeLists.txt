cmake_minimum_required (VERSION 2.8.12)

project(PDC_VOL C)

include_directories(../build/)

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

find_package(PDC REQUIRED)
if(PDC_FOUND)
    #message(STATUS "PDC include directory: ${PDC_INCLUDE_DIR}")
  set(PDC_EXT_INCLUDE_DEPENDENCIES ${PDC_INCLUDE_DIR}
    ${PDC_EXT_INCLUDE_DEPENDENCIES}
    )
  set(PDC_EXT_LIB_DEPENDENCIES pdc ${PDC_EXT_LIB_DEPENDENCIES})
endif()

#HDF5
option(USE_SYSTEM_HDF5 "Use system-installed HDF5." ON)
  if(USE_SYSTEM_HDF5)
  find_package(HDF5 NO_MODULE NAMES hdf5 COMPONENTS C shared)
  if(HDF5_FOUND)
    set(HDF5_C_SHARED_LIBRARY hdf5-shared)
#    if(NOT TARGET ${HDF5_C_SHARED_LIBRARY})
#      message(FATAL_ERROR "Could not find hdf5 shared target, please make "
#"sure that HDF5 has ben compiled with shared libraries enabled.")
#    endif()
    set(PDC_EXT_INCLUDE_DEPENDENCIES
        ${PDC_EXT_INCLUDE_DEPENDENCIES}
        ${HDF5_INCLUDE_DIR}
      )
    set(PDC_EXT_LIB_DEPENDENCIES
      ${PDC_EXT_LIB_DEPENDENCIES}
      ${HDF5_C_SHARED_LIBRARY}
    )
  else()
    # Allow for HDF5 autotools builds
    find_package(HDF5 MODULE REQUIRED)
    if(HDF5_FOUND)
      set(PDC_EXT_INCLUDE_DEPENDENCIES
        ${PDC_EXT_INCLUDE_DEPENDENCIES}
        ${HDF5_INCLUDE_DIRS}
      )
      set(PDC_EXT_LIB_DEPENDENCIES
        ${PDC_EXT_LIB_DEPENDENCIES}
        ${HDF5_LIBRARIES}
      )
    else()
      message(FATAL_ERROR "Could not find HDF5, please check HDF5_DIR.")
    endif()
  endif()
endif()

add_definitions(-DENABLE_MPI=1)
add_library(cjson cjson/cJSON.c)

set(PROGRAMS
  pdc_ls
  pdc_import
  pdc_export
  )

foreach(program ${PROGRAMS})
  add_executable(${program} ${program}.c)
  target_link_libraries(${program} ${PDC_EXT_LIB_DEPENDENCIES})
  target_link_libraries(${program} cjson)
endforeach(program)


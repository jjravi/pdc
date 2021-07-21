find_package(MERCURY REQUIRED)

if(PDC_ENABLE_FASTBIT)
    add_definitions(-DENABLE_FASTBIT=1)
    find_path(FASTBIT_INCLUDE_DIR fastbit/iapi.h)
    find_library(FASTBIT_LIBRARY fastbit $ENV{HOME}/cori/fastbit-2.0.3/install)
endif()

if(PDC_ENABLE_ROCKSDB)
    add_definitions(-DENABLE_ROCKSDB=1)
    find_path(ROCKSDB_INCLUDE_DIR include/db.h)
    find_library(ROCKSDB_LIBRARY rocksdb /global/cfs/cdirs/m1248/pdc/rocksdb/)
    find_library(BZ2_LIBRARY bz2 /global/cfs/cdirs/m1248/pdc/rocksdb/)
    find_library(LZ4_LIBRARY lz4 /global/cfs/cdirs/m1248/pdc/rocksdb/)
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PDC_INCLUDES_BUILD_TIME}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/dablooms
  ${MERCURY_INCLUDE_DIR}
  ${FASTBIT_INCLUDE_DIR}
  ${ROCKSDB_INCLUDE_DIR}
)

add_definitions( -DIS_PDC_SERVER=1 )

add_executable(pdc_server.exe 
               pdc_server.c
               pdc_server_data.c
               pdc_server_metadata.c
               pdc_server_analysis.c
               ../api/pdc_client_server_common.c
               ../api/pdc_analysis_common.c
               ../api/pdc_transforms_common.c
               dablooms/pdc_dablooms.c
               dablooms/pdc_murmur.c
               pdc_hash-table.c
               ../api/pdc_hist_pkg.c
)

target_link_libraries(pdc_server.exe mercury pdcprof -lm -ldl ${PDC_EXT_LIB_DEPENDENCIES})
if(PDC_ENABLE_FASTBIT)
    message(STATUS "Enabled fastbit")
    target_link_libraries(pdc_server.exe ${FASTBIT_LIBRARY}/libfastbit.so)
elseif(PDC_ENABLE_ROCKSDB)
    message(STATUS "Enabled RocksDB")
    target_link_libraries(pdc_server.exe ${ROCKSDB_LIBRARY})
    target_link_libraries(pdc_server.exe ${BZ2_LIBRARY})
    target_link_libraries(pdc_server.exe ${LZ4_LIBRARY})
endif()

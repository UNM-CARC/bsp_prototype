project( bsp_prototype LANGUAGES CXX C )
cmake_minimum_required( VERSION 3.2 )

find_package( MPI REQUIRED )
message( STATUS "MPI_CXX_FOUND = ${MPI_CXX_FOUND}" )
if( MPI_C_FOUND AND NOT TARGET MPI::MPI_C )
  # synthesize an import target
  add_library( MPI::MPI_C INTERFACE IMPORTED )
  target_link_libraries( MPI::MPI_C INTERFACE ${MPI_C_LIBRARIES} )
  target_include_directories( MPI::MPI_C INTERFACE ${MPI_C_LIBRARIES} )
  target_compile_options( MPI::MPI_C INTERFACE ${MPI_C_COMPILE_OPTIONS} )
  target_compile_definitions( MPI::MPI_C INTERFACE ${MPI_C_COMPILE_DEFINITIONS} )
endif()
if( MPI_CXX_FOUND AND NOT TARGET MPI::MPI_CXX )
  # synthesize an import target
  add_library( MPI::MPI_CXX INTERFACE IMPORTED )
  target_link_libraries( MPI::MPI_CXX INTERFACE ${MPI_CXX_LIBRARIES} )
  target_include_directories( MPI::MPI_CXX INTERFACE ${MPI_CXX_LIBRARIES} )
  target_compile_options( MPI::MPI_CXX INTERFACE ${MPI_CXX_COMPILE_OPTIONS} )
  target_compile_definitions( MPI::MPI_CXX INTERFACE ${MPI_CXX_COMPILE_DEFINITIONS} )
endif()


# Look for SPRNG
SET(TRY_INCLUDE_PATHS
  /usr/local/include
  /usr/local/include/sprng
  /usr/include
  /usr/include/sprng
  /sw/include
  /sw/include/sprng
  /opt/local/include
  /opt/local/include/sprng
  ${CMAKE_SOURCE_DIR}/include
  )

set( TRY_LINK_PATHS
  /usr/lib
  /usr/local/lib
  /sw/lib
  /opt/local/lib
  ${CMAKE_SOURCE_DIR}/lib
  )

find_library( SPRNG_LIBRARY sprng PATHS ${TRY_LINK_PATHS} $ENV{SPRNG_ROOT}/lib )
find_path( SPRNG_INCLUDE_DIR sprng.h PATHS ${TRY_INCLUDE_PATHS} $ENV{SPRNG_ROOT}/include )

IF(SPRNG_INCLUDE_DIR AND SPRNG_LIBRARY)
  set(SPRNG_FOUND 1 CACHE BOOL "SPRNG found")
  message(STATUS "SPRNG found (SPRNG_INCLUDE_DIR=${SPRNG_INCLUDE_DIR}, SPRNG_LIBRARY=${SPRNG_LIBRARY})")
ELSE()
  set(SPRNG_FOUND 0 CACHE BOOL "SPRNG not found")
  message( STATUS "SPRNG not found" )
ENDIF()

add_library( SPRNG::sprng INTERFACE IMPORTED )
target_include_directories( SPRNG::sprng INTERFACE ${SPRNG_INCLUDE_DIR} )
target_link_libraries( SPRNG::sprng INTERFACE ${SPRNG_LIBRARY} )

# Look for GSL (a lot less complicated because FindGSL.cmake exists
find_package( GSL REQUIRED )

# Look for RabbitMQ
# right now this depends on someone having done an install from their librabbitmq tree
#  so librabbitmq.pc gets put somewhere pkg-config can find it
#  If that install doesn't end up in a standard location, add the install root to
#  CMAKE_PREFIX_PATH and this should find it
include( FindPkgConfig )
pkg_search_module( rabbitmq librabbitmq IMPORTED_TARGET GLOBAL REQUIRED )
message( STATUS "RabbitMQ found: ${rabbitmq_LIBRARIES}" )

# We found everything, set up the build
set( SOURCES bsp_prototype.cc amqp_producer.c )
add_executable( bsp_prototype ${SOURCES} )
set_target_properties( bsp_prototype PROPERTIES LINKER_LANGUAGE CXX )
target_link_libraries( bsp_prototype GSL::gsl GSL::gslcblas SPRNG::sprng PkgConfig::rabbitmq MPI::MPI_CXX )

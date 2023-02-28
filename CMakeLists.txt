cmake_minimum_required(VERSION 3.16)

project(ChimeraTK-ControlSystemAdapter)

include(cmake/add_dependency.cmake)

set(${PROJECT_NAME}_MAJOR_VERSION 02)
set(${PROJECT_NAME}_MINOR_VERSION 05)
set(${PROJECT_NAME}_PATCH_VERSION 01)
include(cmake/set_version_numbers.cmake)

include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)
include(cmake/enable_code_coverage_report.cmake)

include(cmake/format_options.cmake)

#boost as such is required for shared pointers e.g.
FIND_PACKAGE(Boost 1.58 REQUIRED)
#boost chrono is needed for measuring time intervals.
FIND_PACKAGE(Boost REQUIRED COMPONENTS chrono)
#boost system is needed by boost::lockfree.
FIND_PACKAGE(Boost REQUIRED COMPONENTS system)
#boost thread is needed for synchronization and multi-threading
FIND_PACKAGE(Boost REQUIRED COMPONENTS thread)
#boost system is needed by some tests
FIND_PACKAGE(Boost REQUIRED COMPONENTS filesystem)

#the unit test component is optional
option(BUILD_TESTS "Build test programs" ON)
IF(BUILD_TESTS)
  FIND_PACKAGE(Boost COMPONENTS unit_test_framework REQUIRED)
  ENABLE_TESTING()
ENDIF()

#turn on the rpath so dependencies are found properly
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# DeviceAccess is needed due to the common interface classes
# no longer use add_dependency by directly find_package because we don't want pulled in options become PUBLIC
FIND_PACKAGE(ChimeraTK-DeviceAccess 03.02 REQUIRED)

# libXML++ is used for parsing/writing XML files
FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED IMPORTED_TARGET ${LIBXML++_VERSION})
PKG_CHECK_MODULES(glib REQUIRED IMPORTED_TARGET glib-2.0)

# libatomic is required on some patforms (and doesn't hurt on the others)
find_library(atomic-library NAMES atomic libatomic.so libatomic.so.1)
# On some platforms, libatomic does not exist. In order to avoid an error when
# we refer to the "atomic-library" variable on these platforms, we set it to the
# empty string if it is not defined.
if(NOT atomic-library)
  set(atomic-library "")
endif()

aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

#Create the executables for automated unit testing.
#Currently we assume that they are all tests contained in one file, so
#each file gives a new executable. This section has to be adapted if this should change.
if(BUILD_TESTS)
  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/src testSources)
  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/auxsrc testAuxSources)
  foreach( testSourceFile ${testSources})
    #NAME_WE means the base name without path and (longest) extension
    get_filename_component(executableName ${testSourceFile} NAME_WE)
    add_executable(${executableName} ${testSourceFile} ${testAuxSources})
    
    #Includes inside the library are local includes, so we give the full path to the include directory
    target_include_directories(${executableName} PRIVATE ${CMAKE_SOURCE_DIR}/include/ChimeraTK/ControlSystemAdapter)
    target_include_directories(${executableName} PRIVATE ${CMAKE_SOURCE_DIR}/tests/include/ChimeraTK/ControlSystemAdapter/Testing)
    #The ReferenceTestApplication is written to work with the control system adapter as installed, so
    #we add that path as system just for the tests.
    target_include_directories(${executableName} SYSTEM BEFORE PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${executableName} PRIVATE ${atomic-library} ${PROJECT_NAME}
        Boost::chrono Boost::system Boost::thread Boost::filesystem
        ChimeraTK::ChimeraTK-DeviceAccess pthread)
    add_test(${executableName} ${executableName})
  endforeach( testSourceFile )

  FILE(COPY tests/decoratorTest.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/changedType.persist DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/changedVectorSize.persist DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/renamedVariable.persist DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/testMappedImage.dmap tests/testMappedImage.map DESTINATION ${PROJECT_BINARY_DIR})
endif()

add_library(${PROJECT_NAME} SHARED ${library_sources} )
#Includes inside the library are local includes, so we give the full path to the include directory
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include/ChimeraTK/ControlSystemAdapter)
target_include_directories(${PROJECT_NAME} PUBLIC 
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
    # relative to ${CMAKE_INSTALL_PREFIX} but don't explicitly mention it, to make result relocatable
    "$<INSTALL_INTERFACE:include>")

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::LibXML++ PkgConfig::glib)
target_link_libraries(${PROJECT_NAME} PUBLIC ${atomic-library} 
    Boost::chrono Boost::system Boost::thread 
    ChimeraTK::ChimeraTK-DeviceAccess)

#Install the library
install(TARGETS ${PROJECT_NAME}
            EXPORT ${PROJECT_NAME}Targets
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
)

#all include files go into include
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include
        FILES_MATCHING
            PATTERN "ChimeraTK/*.h"
            PATTERN "ChimeraTK/ControlSystemAdapter/*.h")

#We additionally install the reference test application, it's header only
install(DIRECTORY ${CMAKE_SOURCE_DIR}/tests/include/  DESTINATION include
        FILES_MATCHING
            PATTERN "ChimeraTK/ControlSystemAdapter/Testing/*.h")

# Note, variables below are provided only for compatibility; we prefer cmake exports.
#Create the config files by filling the correct variables into the template (*.cmake.in).
#All variables @VARIABLE@ are replaced with the current values, but they have to be set first....
#Only the reference header is needed. The other variables are for dependencies of this library,
#but there are none.
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include ${Boost_INCLUDE_DIRS}")
set(${PROJECT_NAME}_LIBRARIES "${atomic-library} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${ChimeraTK-DeviceAccess_LIBRARIES}")
set(${PROJECT_NAME}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib ${Boost_LIBRARY_DIRS} ${ChimeraTK-DeviceAccess_LIBRARY_DIRS}")
set(${PROJECT_NAME}_CXX_FLAGS "${ChimeraTK-DeviceAccess_CXX_FLAGS}")
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
set(${PROJECT_NAME}_LINKER_FLAGS "-Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib ${ChimeraTK-DeviceAccess_LINKER_FLAGS}")
else()
set(${PROJECT_NAME}_LINKER_FLAGS "-Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib,--enable-new-dtags ${ChimeraTK-DeviceAccess_LINKER_FLAGS}")
endif()
list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "Boost COMPONENTS system thread filesystem date_time atomic chrono")

# we support our cmake EXPORTS as imported targets
set(PROVIDES_EXPORTED_TARGETS 1)
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)


## The instructions to create a debian package are loaded from a separate file

#include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/enable_doxygen_documentation.cmake)

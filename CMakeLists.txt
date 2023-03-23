cmake_minimum_required(VERSION 3.16)

project(ChimeraTK-ControlSystemAdapter)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 02)
set(${PROJECT_NAME}_MINOR_VERSION 06)
set(${PROJECT_NAME}_PATCH_VERSION 00)
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
  FIND_PACKAGE(Boost REQUIRED COMPONENTS unit_test_framework)
  ENABLE_TESTING()
ENDIF()

# do not remove runtime paths of the library when installing (helps for unsually located implicit dependencies)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# DeviceAccess is needed due to the common interface classes
# no longer use add_dependency by directly find_package because we don't want pulled in options become PUBLIC
FIND_PACKAGE(ChimeraTK-DeviceAccess 03.09 REQUIRED)

# libXML++ is used for parsing/writing XML files
FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED IMPORTED_TARGET ${LIBXML++_VERSION})

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
    target_link_libraries(${executableName} 
        # most of the time we set dependencies for tests to PRIVATE; however since ReferenceTestApplication
        # gets installed and is header-only, in this case we put PUBLIC
        PUBLIC ${PROJECT_NAME}
        # not yet implicit from CSAdapter
        PUBLIC Boost::filesystem pthread)
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
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::LibXML++)
target_link_libraries(${PROJECT_NAME} 
    PUBLIC ${atomic-library} 
    Boost::chrono Boost::system Boost::thread 
    ChimeraTK::ChimeraTK-DeviceAccess)

#Install the library
# this defines architecture-dependent ${CMAKE_INSTALL_LIBDIR}
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
            EXPORT ${PROJECT_NAME}Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
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

# we need the public dependencies so create_cmake_config_files can find them as implicit dependencies
list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "Boost COMPONENTS system thread filesystem date_time chrono")
list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "ChimeraTK-DeviceAccess")
# we support our cmake EXPORTS as imported targets
set(PROVIDES_EXPORTED_TARGETS 1)
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)


## The instructions to create a debian package are loaded from a separate file

#include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/enable_doxygen_documentation.cmake)

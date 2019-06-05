cmake_minimum_required(VERSION 3.0.2)

project(ChimeraTK-ControlSystemAdapter)

include(cmake/add_dependency.cmake)

set(${PROJECT_NAME}_MAJOR_VERSION 01)
set(${PROJECT_NAME}_MINOR_VERSION 04)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)

#boost as such is required for shared pointers e.g.
FIND_PACKAGE(Boost 1.58 REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

#boost chrono is needed for measuring time intervals.
FIND_PACKAGE(Boost REQUIRED COMPONENTS chrono)

#boost system is needed by boost::lockfree.
FIND_PACKAGE(Boost REQUIRED COMPONENTS system)

#boost thread is needed for synchronization and multi-threading
FIND_PACKAGE(Boost REQUIRED COMPONENTS thread)

#boost system is needed by some tests
FIND_PACKAGE(Boost REQUIRED COMPONENTS filesystem)

#the unit test component is optional
FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
IF(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  set(TESTING_IS_ENABLED "true")
  ENABLE_TESTING()
ELSE(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  message("Boost unit_test_framework not found, disabling testing")
ENDIF(Boost_UNIT_TEST_FRAMEWORK_FOUND)

#turn on the rpath so dependencies are found properly
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# DeviceAccess is needed due to the common interface classes
add_dependency(ChimeraTK-DeviceAccess 01.02 REQUIRED)

# libXML++ is used for parsing/writing XML files
FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED ${LIBXML++_VERSION})
PKG_CHECK_MODULES(glib REQUIRED glib-2.0)
include_directories(SYSTEM "${LibXML++_INCLUDEDIR}/${LIBXML++_VERSION}" ${glib_INCLUDE_DIRS} ${LibXML++_INCLUDE_DIRS})

# libatomic is required on some patforms (and doesn't hurt on the others)
find_library(atomic-library NAMES atomic libatomic.so libatomic.so.1)
# On some platforms, libatomic does not exist. In order to avoid an error when
# we refer to the "atomic-library" variable on these platforms, we set it to the
# empty string if it is not defined.
if(NOT atomic-library)
  set(atomic-library "")
endif()

#Includes inside the library are local includes, so we give the full path to the include directory
include_directories(${CMAKE_SOURCE_DIR}/include/ChimeraTK/ControlSystemAdapter)
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

#Create the executables for automated unit testing.
#Currently we assume that they are all tests contained in one file, so
#each file gives a new executable. This section has to be adapted if this should change.
if(TESTING_IS_ENABLED)
  #include_directories(${CMAKE_SOURCE_DIR}/tests/include)
  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/src testSources)
  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/auxsrc testAuxSources)
  foreach( testSourceFile ${testSources})
    #NAME_WE means the base name without path and (longest) extension
    get_filename_component(executableName ${testSourceFile} NAME_WE)
    add_executable(${executableName} ${testSourceFile} ${testAuxSources})
    target_include_directories(${executableName} PRIVATE ${CMAKE_SOURCE_DIR}/tests/include/ChimeraTK/ControlSystemAdapter/Testing)
    #The ReferenceTestApplication is written to work with the control system adapter as installed, so
    #we add that path as system just for the tests.
    target_include_directories(${executableName} SYSTEM BEFORE PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${executableName} ${atomic-library} ${PROJECT_NAME} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${ChimeraTK-DeviceAccess_LIBRARIES} pthread)
    set_target_properties(${executableName} PROPERTIES LINK_FLAGS ${Boost_LINK_FLAGS} ${ChimeraTK-DeviceAccess_LINK_FLAGS})
    add_test(${executableName} ${executableName})
  endforeach( testSourceFile )

  #The make coverage command is only available in debug mode
  IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    configure_file(cmake/Makefile.coverage.in
      ${PROJECT_BINARY_DIR}/Makefile.coverage @ONLY)
    add_custom_target(coverage
      make -f Makefile.coverage
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating test coverage documentation" VERBATIM
      )
  ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

  FILE(COPY tests/decoratorTest.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/changedType.persist DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/changedVectorSize.persist DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/renamedVariable.persist DESTINATION ${PROJECT_BINARY_DIR})
endif(TESTING_IS_ENABLED)

add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME} ${atomic-library} ${LibXML++_LDFLAGS} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${ChimeraTK-DeviceAccess_LIBRARIES} ${LibXML++_LIBRARIES} ${glib_LIBRARIES})

#Install the library
install(TARGETS ${PROJECT_NAME}
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

#Create the config files by filling the correct variables into the template (*.cmake.in).
#All variables @VARIABLE@ are replaced with the current values, but they have to be set first....
#Only the reference header is needed. The other variables are for dependencies of this library,
#but there are none.
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include ${Boost_INCLUDE_DIRS}")
set(${PROJECT_NAME}_LIBRARIES "${PROJECT_NAME} ${atomic-library} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${ChimeraTK-DeviceAccess_LIBRARIES}")
set(${PROJECT_NAME}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib ${Boost_LIBRARY_DIRS} ${ChimeraTK-DeviceAccess_LIBRARY_DIRS}")
set(${PROJECT_NAME}_CXX_FLAGS "${ChimeraTK-DeviceAccess_CXX_FLAGS}")
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
set(${PROJECT_NAME}_LINKER_FLAGS "-Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib ${ChimeraTK-DeviceAccess_LINKER_FLAGS}")
else()
set(${PROJECT_NAME}_LINKER_FLAGS "-Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib,--enable-new-dtags ${ChimeraTK-DeviceAccess_LINKER_FLAGS}")
endif()
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)


## The instructions to create a debian package are loaded from a separate file

#include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/enable_doxygen_documentation.cmake)


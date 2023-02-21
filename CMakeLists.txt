cmake_minimum_required(VERSION 3.0.2)
cmake_policy(SET CMP0028 NEW) # disables a warning

project(ChimeraTK-ControlSystemAdapter)

include(cmake/add_dependency.cmake)

set(${PROJECT_NAME}_MAJOR_VERSION 02)
set(${PROJECT_NAME}_MINOR_VERSION 05)
set(${PROJECT_NAME}_PATCH_VERSION 01)
include(cmake/set_version_numbers.cmake)

include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)
include(cmake/enable_code_coverage_report.cmake)

#boost as such is required for shared pointers e.g.
FIND_PACKAGE(Boost 1.58 REQUIRED)
# these are becoming PUBLIC (TODO - check)
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
option(BUILD_TESTS "Build test programs" ON)
IF(BUILD_TESTS)
  FIND_PACKAGE(Boost COMPONENTS unit_test_framework REQUIRED)
  ENABLE_TESTING()
ENDIF()

#turn on the rpath so dependencies are found properly
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# DeviceAccess is needed due to the common interface classes
# TODO check what this pulls in for our exported target
add_dependency(ChimeraTK-DeviceAccess 03.02 REQUIRED)

# libXML++ is used for parsing/writing XML files
FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED ${LIBXML++_VERSION})
PKG_CHECK_MODULES(glib REQUIRED glib-2.0)
set(privateSystemIncludes ${glib_INCLUDE_DIRS} ${LibXML++_INCLUDE_DIRS})

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
    
    target_include_directories(${executableName} SYSTEM PRIVATE ${privateSystemIncludes})
    #Includes inside the library are local includes, so we give the full path to the include directory
    # TODO check - maybe following line not neccesary
    target_include_directories(${executableName} PRIVATE ${CMAKE_SOURCE_DIR}/include/ChimeraTK/ControlSystemAdapter)
    target_include_directories(${executableName} PRIVATE ${CMAKE_SOURCE_DIR}/tests/include/ChimeraTK/ControlSystemAdapter/Testing)
    #The ReferenceTestApplication is written to work with the control system adapter as installed, so
    #we add that path as system just for the tests.
    target_include_directories(${executableName} SYSTEM BEFORE PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${executableName} ${atomic-library} ${PROJECT_NAME} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${ChimeraTK-DeviceAccess_LIBRARIES} pthread)
    set_target_properties(${executableName} PROPERTIES LINK_FLAGS "-Wl,-rpath,${PROJECT_BINARY_DIR} ${Boost_LINK_FLAGS} ${ChimeraTK-DeviceAccess_LINK_FLAGS}")
    add_test(${executableName} ${executableName})
  endforeach( testSourceFile )

  FILE(COPY tests/decoratorTest.map DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/changedType.persist DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/changedVectorSize.persist DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/renamedVariable.persist DESTINATION ${PROJECT_BINARY_DIR})
  FILE(COPY tests/testMappedImage.dmap tests/testMappedImage.map DESTINATION ${PROJECT_BINARY_DIR})
endif()

add_library(${PROJECT_NAME} SHARED ${library_sources} )
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${privateSystemIncludes})
#Includes inside the library are local includes, so we give the full path to the include directory
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include/ChimeraTK/ControlSystemAdapter)
# TODO check targets output for this
target_include_directories(${PROJECT_NAME} PUBLIC 
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>")

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LibXML++_LDFLAGS} ${LibXML++_LIBRARIES} ${glib_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${atomic-library} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${ChimeraTK-DeviceAccess_LIBRARIES})

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
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)


## The instructions to create a debian package are loaded from a separate file

#include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/enable_doxygen_documentation.cmake)



# Below, I try out things in order to move to imported targets
# Once sufficiently generic, might move this to project-template

# set up namespaced alias
# TODO - discuss naming. Should "ChimeraTK" appear inside namespace?
# TODO - find out how name spacing should be used.
#  - docu says, imported targets should be namespaced.
#  - but it seems, file names and arguments to find_package do not use namespaces
#add_library(ChimeraTK::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# add include directories
# system includes, dependency includes, own includes
# dependency includes: I think we can skip them if via imported target ?
# system includes: PRIVATE, if consuming code also wants these, it should set it up by itself
#target_include_directories(${PROJECT_NAME2}
#                           PRIVATE
#                           ${Boost_INCLUDE_DIRS} ${glib_INCLUDE_DIRS} ${LibXML++_INCLUDE_DIRS})

# For the exported target, all that we need is the own includes. Includes from our dependencies are added automatically, if they are handled via imported target
# TODO - check exported target, does list include all the clutter from above?
#  YES! Include dirs, all linker flags in main targets file, library location in targets-debug config file.
# TODO - what does INSTALL_INTERFACE logic mean?
# own includes: PUBLIC because consuming code might also use these
#target_include_directories(${PROJECT_NAME}
#                           PUBLIC
#                           "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ChimeraTK/ControlSystemAdapter>"
#                           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ChimeraTK/ControlSystemAdapter>")

#include(GNUInstallDirs) # defines CMAKE_INSTALL_LIBDIR etc

# install the target and create export-set
# actually I'm here only interested in export-set, rest should be already handled
#install(TARGETS ${PROJECT_NAME}
#        EXPORT ${PROJECT_NAME}Targets
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)

# install header files
#install(FILES Addition.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# generate and install export file
# TODO discuss - should we care about CONFIGURATIONS? They are only relevant if we intend to install e.g. Debug and Release into the same dir.
# If it's clear that in production, only one (Release) config will be needed, we don't need them.
#    install(EXPORT ${PROJECT_NAME}Targets
#            FILE ${PROJECT_NAME}Targets.cmake
#            NAMESPACE ChimeraTK::
#            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
#    )

## include CMakePackageConfigHelpers macro
#include(CMakePackageConfigHelpers)

## generate the version file for the config file
#write_basic_package_version_file(
## TODO check - we might have a problem with overwitten files
  #"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}2Version.cmake"
## TODO - which version from set_version_numbers makes sense here?
## for now, just take full version major.minor.patch
  #VERSION "${PROJECT_NAME}_VERSION"
  ## TODO - look up whether that applies to us
  #COMPATIBILITY AnyNewerVersion
#)

# create config file
#configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
# TODO check - we might have a problem with overwitten files. 
#  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}2Config.cmake"
#  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
#  NO_CHECK_REQUIRED_COMPONENTS_MACRO
#)

# install config files
#install(FILES
#          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
#        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
#)

# TODO - in the end, we will need something that is backwards compatible, i.e. requires the CMakeLists.txt
# of actual procets are not edited, except for project-template part.
# Realistically this is only achievable for consumers, but not for exports.




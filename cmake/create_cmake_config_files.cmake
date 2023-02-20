#######################################################################################################################
# create_cmake_config_files.cmake
#
# Create the Find${PROJECT_NAME}.cmake cmake macro and the ${PROJECT_NAME}-config shell script and installs them.
#
# Expects the following input variables:
#   ${PROJECT_NAME}_SOVERSION - version of the .so library file (or just MAJOR.MINOR without the patch level)
#   ${PROJECT_NAME}_INCLUDE_DIRS - list include directories needed when compiling against this project
#   ${PROJECT_NAME}_LIBRARY_DIRS - list of library directories needed when linking against this project
#   ${PROJECT_NAME}_LIBRARIES - list of additional libraries needed when linking against this project. The library
#                               provided by the project will be added automatically
#   ${PROJECT_NAME}_CXX_FLAGS - list of additional C++ compiler flags needed when compiling against this project
#   ${PROJECT_NAME}_LINKER_FLAGS - list of additional linker flags needed when linking against this project
#   ${PROJECT_NAME}_MEXFLAGS - (optional) mex compiler flags
#
#######################################################################################################################

#######################################################################################################################
#
# IMPORTANT NOTE:
#
# DO NOT MODIFY THIS FILE inside a project. Instead update the project-template repository and pull the change from
# there. Make sure to keep the file generic, since it will be used by other projects, too.
#
# If you have modified this file inside a project despite this warning, make sure to cherry-pick all your changes
# into the project-template repository immediately.
#
#######################################################################################################################

# create variables for standard makefiles and pkgconfig
set(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "${${PROJECT_NAME}_CXX_FLAGS}")

string(REPLACE " " ";" LIST "${${PROJECT_NAME}_INCLUDE_DIRS}")
foreach(INCLUDE_DIR ${LIST})
  set(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "${${PROJECT_NAME}_CXX_FLAGS_MAKEFILE} -I${INCLUDE_DIR}")
endforeach()

set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS} ${${PROJECT_NAME}_LINK_FLAGS}")

string(REPLACE " " ";" LIST "${${PROJECT_NAME}_LIBRARY_DIRS}")
foreach(LIBRARY_DIR ${LIST})
  set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} -L${LIBRARY_DIR}")
endforeach()

string(REPLACE " " ";" LIST "${PROJECT_NAME} ${${PROJECT_NAME}_LIBRARIES}")
foreach(LIBRARY ${LIST})
  if(LIBRARY MATCHES "/")         # library name contains slashes: link against the a file path name
    set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} ${LIBRARY}")
  elseif(LIBRARY MATCHES "^-l")   # library name does not contain slashes but already the -l option: directly quote it
    set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} ${LIBRARY}")
  elseif(LIBRARY MATCHES "::")  # library name is an exported target - we need to resolve it for Makefiles
    get_property(lib_loc TARGET ${LIBRARY} PROPERTY LOCATION)
    string(APPEND ${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE " ${lib_loc}")
  else()                          # link against library with -l option
    set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} -l${LIBRARY}")
  endif()
endforeach()

set(${PROJECT_NAME}_PUBLIC_DEPENDENCIES_L "")
foreach(DEPENDENCY ${${PROJECT_NAME}_PUBLIC_DEPENDENCIES})
    string(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES_L "find_package(${DEPENDENCY} REQUIRED)\n")
endforeach()

if(TARGET ${PROJECT_NAME})
  set(${PROJECT_NAME}_HAS_LIBRARY 1)
else()
  set(${PROJECT_NAME}_HAS_LIBRARY 0)
endif()

# we have nested @-statements, so we have to parse twice:

# create the cmake find_package configuration file
set(PACKAGE_INIT "@PACKAGE_INIT@") # replacement handled later, so leave untouched here
cmake_policy(SET CMP0053 NEW) # less warnings about irrelevant stuff in comments
configure_file(cmake/PROJECT_NAMEConfig.cmake.in.in "${PROJECT_BINARY_DIR}/cmake/Config.cmake.in" @ONLY)
#configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)
configure_file(cmake/PROJECT_NAMEConfigVersion.cmake.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" @ONLY)

# create the shell script for standard make files
configure_file(cmake/PROJECT_NAME-config.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config" @ONLY)

# create the pkgconfig file
configure_file(cmake/PROJECT_NAME.pc.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}.pc.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}.pc.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc" @ONLY)


# install cmake find_package configuration file
#install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#  DESTINATION lib/cmake/${PROJECT_NAME} COMPONENT dev)
#install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
#  DESTINATION lib/cmake/${PROJECT_NAME} COMPONENT dev)

# install same cmake configuration file another time into the Modules cmake subdirectory for compatibility reasons
# TODO discuss whether necc. - it seems it breaks things now!
#install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#  DESTINATION share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules RENAME Find${PROJECT_NAME}.cmake COMPONENT dev)

# install script for Makefiles
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config DESTINATION bin COMPONENT dev)

# install configuration file for pkgconfig
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION share/pkgconfig COMPONENT dev)


# Below comes stuff for the new auto-generated Target config by cmake, for imported targets
# TODO - make the output merge with that defined above

# set up namespaced alias
# TODO - discuss naming. Should "ChimeraTK" appear inside namespace?
# TODO - find out how name spacing should be used.
#  - docu says, imported targets should be namespaced.
#  - but it seems, file names and arguments to find_package do not use namespaces
add_library(ChimeraTK::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

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

include(GNUInstallDirs) # defines CMAKE_INSTALL_LIBDIR etc

# generate and install export file
# TODO discuss - should we care about CONFIGURATIONS? They are only relevant if we intend to install e.g. Debug and Release into the same dir.
# If it's clear that in production, only one (Release) config will be needed, we don't need them.
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ChimeraTK::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

# include CMakePackageConfigHelpers macro
include(CMakePackageConfigHelpers)

# TODO - check how this merges with our version defs
# generate the version file for the config file
#write_basic_package_version_file(
# TODO check - we might have a problem with overwitten files
#  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Version.cmake"
# TODO - which version from set_version_numbers makes sense here?
# for now, just take full version major.minor.patch
#  VERSION "${PROJECT_NAME}_VERSION"
  # TODO - look up whether that applies to us
#  COMPATIBILITY AnyNewerVersion
#)

# create config file
configure_package_config_file("${PROJECT_BINARY_DIR}/cmake/Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

message("install config files to DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

# install cmake find_package configuration file
install(FILES
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
        COMPONENT dev
)

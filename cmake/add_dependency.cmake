#######################################################################################################################
# add_dependency.cmake
#
# Define macro to search for a dependency and add it to our project. This works only with dependency using the
# project-template!
#
# The macro is called "add_dependency" and takes 3 arguments:
#   - name of the dependency project as passed to FIND_PACKAGE (e.g. "ChimeraTK-DeviceAccess")
#   - required version as passed to FIND_PACKAGE
#   - a list of components used by this project including the REQUIRED keyword etc.
#
# The macro will call FIND_PACKAGE, add the returned include directories to the system list, add the library directories
# to the list of link directories, and update the CMAKE_CXX_FLAGS variable.
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

FUNCTION(add_dependency dependency_project_name required_version)
  # collect additional arguments and put into the list of components
  SET(components "")
  foreach(arg IN LISTS ARGN)
    SET(components ${components} ${arg})
  endforeach()
  FIND_PACKAGE(${dependency_project_name} ${required_version} COMPONENTS ${components})
  # putting include_directories here is BAD because it propages everything into the PUBLIC flags for all targets which are exported.
  # include_directories(SYSTEM ${${dependency_project_name}_INCLUDE_DIRS} ${${dependency_project_name}_INCLUDE_DIR})
  
  # link_directories is BAD because it propages everything into the PUBLIC flags for all targets which are exported.
  #link_directories(${${dependency_project_name}_LIBRARY_DIRS})
  #link_directories(${${dependency_project_name}_LIBRARY_DIR})
  # If build of a depending target fails because we remove this, it was not done correctly in the first place.
  # The depending target should either make use of the (modern) exported target, or use (legacy) ${dependency_project_name}_LIBRARY_DIRS
  
  #set the cxx flags used in the child project
  # CMAKE_CXX_FLAGS is space-separated
  string(REPLACE ";" " " cmakeExtraCxxFlags "${${dependency_project_name}_CXX_FLAGS}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cmakeExtraCxxFlags}" PARENT_SCOPE)
  message("new CXX_FLAGS (${dependency_project_name}): ${cmakeExtraCxxFlags}")
  #set all the flags we found also in the parent scope, so the child can hand them over to the grand children
  # <>_LIBRARIES is semicolon seperated in order that it can be used with target_link_libraries()
  string(REPLACE " " ";" ${dependency_project_name}_LIBRARIES "${${dependency_project_name}_LIBRARIES}")
  SET(${dependency_project_name}_LIBRARIES ${${dependency_project_name}_LIBRARIES} PARENT_SCOPE)
  # <>_LIBRARY_DIRS is semicolon seperated in order that it can be used with target_link_directories()
  string(REPLACE " " ";" ${dependency_project_name}_LIBRARY_DIRS "${${dependency_project_name}_LIBRARY_DIRS}")
  SET(${dependency_project_name}_LIBRARY_DIRS "${${dependency_project_name}_LIBRARY_DIRS};${${dependency_project_name}_LIBRARY_DIR}" PARENT_SCOPE)
  # <>_LINKER_FLAGS is semicolon seperated in order that it can be used with target_link_options()
  string(REPLACE " " ";" ${dependency_project_name}_LINKER_FLAGS "${${dependency_project_name}_LINKER_FLAGS}")
  SET(${dependency_project_name}_LINKER_FLAGS "${${dependency_project_name}_LINKER_FLAGS};${${dependency_project_name}_LINK_FLAGS}" PARENT_SCOPE)
  SET(${dependency_project_name}_LINK_FLAGS "${${dependency_project_name}_LINKER_FLAGS};${${dependency_project_name}_LINK_FLAGS}" PARENT_SCOPE)
  # <>_CXX_FLAGS is semicolon seperated in order that it can be used with target_compile_options
  string(REPLACE " " ";" ${dependency_project_name}_CXX_FLAGS "${${dependency_project_name}_CXX_FLAGS}")
  SET(${dependency_project_name}_CXX_FLAGS "${${dependency_project_name}_CXX_FLAGS}" PARENT_SCOPE)
  SET(${dependency_project_name}_FOUND ${${dependency_project_name}_FOUND} PARENT_SCOPE)
  SET(${dependency_project_name}_VERSION ${${dependency_project_name}_VERSION} PARENT_SCOPE)
  # <>_INCLUDE_DIRS is semicolon seperated in order that it can be used with target_include_directories
  string(REPLACE " " ";" ${dependency_project_name}_INCLUDE_DIRS "${${dependency_project_name}_INCLUDE_DIRS}")
  SET(${dependency_project_name}_INCLUDE_DIRS ${${dependency_project_name}_INCLUDE_DIRS} PARENT_SCOPE)
  SET(${dependency_project_name}_PREFIX ${${dependency_project_name}_PREFIX} PARENT_SCOPE)
ENDFUNCTION(add_dependency)


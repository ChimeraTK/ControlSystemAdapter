cmake_minimum_required(VERSION 2.8.0)

project(ControlSystemAdapter)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 00)
set(${PROJECT_NAME}_PATCH_VERSION 00)
set(${PROJECT_NAME}_VERSION
  ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})
 
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -fPIC -Wextra -Wshadow
#-Weffc++ -ansi -pedantic -Wuninitialized -std=c++11")
#skip -Weffc++ for the moment. It gives too many false positives
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -Wuninitialized -std=c++11")

#use -DCMAKE_BUILD_TYPE=Debug in your cmake command to turn on the coverage option
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")

#boost as such is required for shared pointers e.g.
FIND_PACKAGE(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

#boost chrono is needed for measuring time intervals.
FIND_PACKAGE(Boost REQUIRED COMPONENTS chrono)

#boost system is needed by boost::lockfree.
FIND_PACKAGE(Boost REQUIRED COMPONENTS system)

#boost thread is needed for synchronization and multi-threading
FIND_PACKAGE(Boost REQUIRED COMPONENTS thread)

#the unit test component is optional
FIND_PACKAGE(Boost COMPONENTS unit_test_framework)
IF(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  set(TESTING_IS_ENABLED "true")
  ENABLE_TESTING()
ELSE(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  message("Boost unit_test_framework not found, disabling testing")
ENDIF(Boost_UNIT_TEST_FRAMEWORK_FOUND)

include_directories(${CMAKE_SOURCE_DIR}/include)
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
    target_include_directories(${executableName} PRIVATE ${CMAKE_SOURCE_DIR}/tests/include)
    #no target library yet
    # Linking the boost libraries and using their included counterparts at the
    # same time causes problems.
    #target_link_libraries(${executableName} ${PROJECT_NAME} ${Boost_LIBRARIES})
    target_link_libraries(${executableName} ${PROJECT_NAME} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})
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

endif(TESTING_IS_ENABLED)

add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME} ${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})

#change the install prefix to the source directory in case the user has not specified a destination 
#i. e. CMAKE_INSTALL_PREFIX is not set manually
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "Install directory
                 prefix" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

#Install the library
install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib
)

#all include files go into include/PROJECT_NAME
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/${PROJECT_NAME}
          FILES_MATCHING PATTERN "*.h" 
	  PATTERN ".svn" EXCLUDE
	  PATTERN "${PROJECT_NAME}" EXCLUDE)

#We additionally install the independent reference test application, it's header only
install(FILES ${CMAKE_SOURCE_DIR}/tests/include/IndependentTestCore.h  DESTINATION include/${PROJECT_NAME}/testing)	

#Create the config files by filling the correct variables into the template (*.cmake.in).
#All variables @VARIABLE@ are replaced with the current values, but they have to be set first....
#Only the reference header is needed. The other variables are for dependencies of this library,
#but there are none.
set(${PROJECT_NAME}_REFERENCE_HEADER "DeviceProcessVariable.h")
set(${PROJECT_NAME}_CONFIG_INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
set(${PROJECT_NAME}_CONFIG_LIBRARIES "${Boost_CHRONO_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY}")
set(${PROJECT_NAME}_CONFIG_LIBRARY_DIRS "${Boost_LIBRARY_DIRS}")

include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)

#Finally prepare a config file for inclusion in regular Makefiles
configure_file(${CMAKE_SOURCE_DIR}/cmake/PROJECT_NAME.CONFIG.in
	    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.CONFIG @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.CONFIG DESTINATION .)

set(${PROJECT_NAME}_CPPFLAGS "-I${CMAKE_INSTALL_PREFIX}/include -std=c++11")
set(${PROJECT_NAME}_LDFLAGS "-L${CMAKE_INSTALL_PREFIX}/lib -l${PROJECT_NAME} -Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib,--enable-new-dtags")
configure_file(cmake/PROJECT_NAME-config.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config" @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config DESTINATION bin COMPONENT dev)

## The instructions to create a debian package are loaded from a separate file
#include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)



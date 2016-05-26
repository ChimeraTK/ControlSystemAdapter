#check that all config variables which have to be replaced are set
foreach( CONFIG_VARIABLE CONFIG_INCLUDE_DIRS 
                         REFERENCE_HEADER
			 CONFIG_LIBRARIES
			 CONFIG_LIBRARY_DIRS )
  if(NOT ${PROJECT_NAME}_${CONFIG_VARIABLE})
    message(FATAL_ERROR "${PROJECT_NAME}_${CONFIG_VARIABLE} not set in CMakeListst.txt")
  endif(NOT ${PROJECT_NAME}_${CONFIG_VARIABLE})
endforeach(CONFIG_VARIABLE)

#We have nested @-statements, so we have to parse twice
configure_file(cmake/PROJECT_NAMEConfig.cmake.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)

configure_file(cmake/PROJECT_NAMEConfigVersion.cmake.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" @ONLY)

configure_file(cmake/FindPROJECT_NAME.cmake.in
  "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake" @ONLY)

# Install the ..Config.cmake and ..ConfigVersion.cmake
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" 
  "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake"
  DESTINATION share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules COMPONENT dev)

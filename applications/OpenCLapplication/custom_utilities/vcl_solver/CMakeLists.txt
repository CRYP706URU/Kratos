set(CMAKE_INCLUDE_CURRENT_DIR ON)

message("**** configuring Test ViennaCL solver ****")

## generate variables with the sources
set( VIENNACL_TEST_SOURCES    
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
)

###############################################################
## define library Kratos which defines the basic python interface
add_executable(vcl_solver ${VIENNACL_TEST_SOURCES})
target_link_libraries(vcl_solver KratosCore ${OPENCL_LIBRARIES} rt )
install(TARGETS vcl_solver DESTINATION applications/OpenCLapplication/custom_utilities/vcl_solver )

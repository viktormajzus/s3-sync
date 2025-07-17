# CMakeList.txt : CMake project for s3-sync, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (s3-sync "s3-sync.cpp" "s3-sync.h" "AWSManager.cpp" "AWSManager.h" "CLI.cpp" "CLI.h" "Error.h" "Error.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET s3-sync PROPERTY CXX_STANDARD 23)
endif()

find_package(AWSSDK REQUIRED COMPONENTS s3)

target_link_libraries(s3-sync PRIVATE 
	aws-cpp-sdk-s3
	aws-cpp-sdk-core
)

# TODO: Add tests and install targets if needed.

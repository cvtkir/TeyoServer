# CMakeList.txt : CMake project for TeyoServer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.21)

# Enable Hot Reload for MSVC compilers if supported.
#if (POLICY CMP0141)
#  cmake_policy(SET CMP0141 NEW)
#  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
#endif()

project ("TeyoServer")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source to this project's executable.
add_executable (TeyoServer ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(libpqxx REQUIRED)



target_link_libraries(TeyoServer 
	PRIVATE
    Boost::boost
    libpqxx::pqxx
    nlohmann_json::nlohmann_json
)

#if (CMAKE_VERSION VERSION_GREATER 3.12)
#  set_property(TARGET TeyoServer PROPERTY CXX_STANDARD 20)
#endif()

# TODO: Add tests and install targets if needed.

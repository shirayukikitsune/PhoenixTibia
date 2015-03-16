cmake_minimum_required(VERSION 3.1)
project (PHOENIX_TIBIA_SERVER)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

add_subdirectory(PhoenixLibrary)
add_subdirectory(PhoenixTibiaServer)
#add_subdirectory(Plugin_Interserver)
#add_subdirectory(Plugin_InterserverClient)
#add_subdirectory(Plugin_LoginService)

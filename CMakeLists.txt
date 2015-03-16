cmake_minimum_required(VERSION 3.1)
project (PHOENIX_TIBIA_SERVER)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.55.0 REQUIRED COMPONENTS system filesystem)

find_package(LibXml2 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})
include_directories(${LibXml2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LibXml2_LIBRARIES})

add_subdirectory(PhoenixLibrary)
add_subdirectory(PhoenixTibiaServer)
#add_subdirectory(Plugin_Interserver)
#add_subdirectory(Plugin_InterserverClient)
#add_subdirectory(Plugin_LoginService)

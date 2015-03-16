add_library (PhoenixLibrary
    Component.cpp
    ComponentManager.cpp
    LoggerComponent.cpp
    LuaNetworkService.cpp
    NetworkConnection.cpp
    NetworkListener.cpp
    NetworkManager.cpp
    NetworkService.cpp
    Plugin.cpp
    PluginComponent.cpp
    Script.cpp
    ScriptComponent.cpp
    Settings.cpp
    Tools.cpp)

target_include_directories (PhoenixLibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

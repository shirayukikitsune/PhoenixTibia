-- Load general settings
dofile('config.lua')

-- The file to get plugin info from. This is relative to <dataDirectory>/plugins/
pluginsFile = "interserver_plugins.xml"

-- The number of threads to create. If zero, this value will be set to the number of logical processors
workerCount = 1

-- The file that logging information should be outputted to
loggerFile = "phoenixserver-interserver-" .. os.date('%Y%m%d%H%M%S') .. ".log"

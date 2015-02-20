-- The folder that stores all of this service's data. May be absolute or relative to the executable
dataDirectory = "data"
-- The file to get plugin info from. This is relative to <dataDirectory>/plugins/
-- NOTE: this is for the general case. This might be overriden by each service configuration
pluginsFile = "plugins.xml"

-- Same as above, but for scripts
scriptsFile = "scripts.xml"

-- The number of threads to create. If zero, this value will be set to the number of logical processors
workerCount = 2

-- The amount of information to be logged. This should be one of these values: 0 - None, 1 - Fatal, 2 - Error, 3 - Warning, 4 - Information, 5 - Debug
loggerLevel = 5
-- The file that logging information should be outputted to
loggerFile = "phoenixserver-" .. os.date('%Y%m%d%H%M%S') .. ".log"

-- Address which the interserver service will run
interserver_address = "::1"
interserver_port = 7878
interserver_key = 'nonsense'
interserver_algorithm = 'sha-256'
interserver_username = 'root'
interserver_password = '!alksvm%@SA561'

locale = "ja-jp"

client_version = 1053

-- Load general settings
dofile('config.lua')

-- The file to get plugin info from. This is relative to <dataDirectory>/plugins/
pluginsFile = "login_plugins.xml"

-- The file that logging information should be outputted to
loggerFile = "phoenixserver-login-" .. os.date('%Y%m%d%H%M%S') .. ".log"


login_address = '127.0.0.1'
login_port = 7171

local useSHA1 = true
local redisParams = {
    host = '192.168.1.151',
    port = 6379,
}

client = nil
service = nil
callbacks = {}

function hashPassword(pass)
    if useSHA1 == false then
        return pass
    end

    return sha1.hex(pass)
end

function onBeforeNetworkStart()
    if interserverclient == nil then
        print('LUA Account service requires the interserver client to be present.')
        if service ~= nil then
            service = nil
        end
    end

	if service ~= nil then
		network.register(service)

        interserverclient.addCapability({
            name = 'account',
            serviceAddress = service.address,
            servicePort = service.port
        })
	end
end

function accountHandler(inpacket, outpacket)
    local account = {}

    local read = function (data, packet)
        data.success = packet:popU8()
        data.accountName = packet:popString()
        data.password = packet:popString()
    end
    local write = function (data, packet)
        packet:pushU8(data.success)
        packet:pushString(data.accountName)
        packet:pushString(data.password)
    end

    read(account, inpacket)

    account.success = 0

    accountHashed = hashPassword(account.accountName)

    if client ~= nil then
        local accountKey = 'account:' .. accountHashed
        if client:exists(accountKey) == true then
            local dbAccount = client:hgetall(accountKey)

            if dbAccount.password == hashPassword(account.password) then
                account.success = 1
            end
        end
    else
        accountFile = io.open('accounts/'..accountHashed)

        if accountFile ~= nil then
            local password = accountFile:read('l')

            if hashPassword(account.password) == password then
                account.success = 1
            end
        end
    end
    write(account, outpacket)
end

function onServerReady()
    if service ~= nil then
        interserverclient.registerPacketSerializableHandler('Account', accountHandler)
    end
end

function onLoaded()
	service = networkservice.new('lua-account')
	service.address = '::1'
	service.port = 6666
	service.needChecksum = false
	service.canHandle = function (connection, packet)
		return false
	end

	callbacks['OnBeforeNetworkStart'] = manager.register(onBeforeNetworkStart, 'OnBeforeNetworkStart')
    callbacks['OnServerReady'] = manager.register(onServerReady, 'OnServerReady')

    print ('Connecting to REDIS')
    local redis = require('redis')
    client = redis.connect(redisParams)
    if client:select(0) == true then
        print ('REDIS connection success!')
    end
end

function onUnloaded()
	if service ~= nil then
		service:unregister()
        service = nil
	end
end

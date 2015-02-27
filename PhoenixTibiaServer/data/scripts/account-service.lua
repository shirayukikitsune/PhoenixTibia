local useSHA1 = true

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
    accountFile = io.open('accounts/'..accountHashed)

    if accountFile ~= nil then
        local password = accountFile:read('l')

        if hashPassword(account.password) == password then
            account.success = 1
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
end

function onUnloaded()
	if service ~= nil then
		service:unregister()
        service = nil
	end
end

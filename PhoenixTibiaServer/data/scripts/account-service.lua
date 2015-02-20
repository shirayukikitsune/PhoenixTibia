service = nil
callbacks = {}

function onBeforeNetworkStart()
	if service ~= nil then
		network.register(service)
	end
end

function onServerReady()
    if interserverclient == nil then
        print('LUA Account service requires the interserver client to be present.')
        if service ~= nil then
            service:unregister()
            service = nil
        end
    end

    if service ~= nil then
        interserverclient.addCapability({
            name = 'account',
            serviceAddress = service.address,
            servicePort = service.port
        })

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

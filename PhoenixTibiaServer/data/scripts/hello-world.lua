a = 2
callbacks = {}

function onBeforeNetworkStart()
	print('Network Ready from LUA!', a)
end

function onServerReady()
	print('Server Ready from LUA!')
end

function onLoaded()
	print('Hello, world!', settings.get('pluginsFile'))

	callbacks['onBeforeNetworkStart'] = manager.register(onBeforeNetworkStart, 'onBeforeNetworkStart')
	callbacks['onServerReady'] = manager.register(onServerReady, 'onServerReady') -- this should be deleted :)
end

function onUnloaded()
	print('Leaving LUA :(')

	for k,v in pairs(callbacks) do
		if v ~= nil and k ~= nil then
			manager.unregister(v, k)
		end
	end
end

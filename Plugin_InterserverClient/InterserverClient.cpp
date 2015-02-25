#include "InterserverClient.h"

#include "Packet.h"
#include "Script.h"
#include "Settings.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include <openssl/evp.h>
#include <openssl/hmac.h>

extern std::weak_ptr<Settings> g_settings;
extern std::weak_ptr<Script> g_script;
extern std::shared_ptr<InterserverClient> g_client;

using boost::asio::ip::tcp;

Capability lua_tocapability(lua_State *L, int index)
{
	Capability out;

	lua_pushstring(L, "name");
	lua_gettable(L, index);
	out.name = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "serviceAddress");
	lua_gettable(L, index);
	if (lua_isstring(L, -1)) {
		out.serviceEndpoint.address(boost::asio::ip::address::from_string(lua_tostring(L, -1)));
	}
	lua_pop(L, 1);

	lua_pushstring(L, "servicePort");
	lua_gettable(L, index);
	if (lua_isnumber(L, -1)) {
		out.serviceEndpoint.port((unsigned short)lua_tointeger(L, -1));
	}
	lua_pop(L, 1);

	return out;
}

int interserverclient_addCapability(lua_State *L)
{
	g_client->addCapability(lua_tocapability(L, 1));

	return 0;
}

int interserverclient_removeCapability(lua_State *L)
{
	g_client->removeCapability(lua_tocapability(L, 1));

	return 0;
}

void createInterserverclientTable(lua_State *L, const char* fieldname, const char* subfield)
{
	int start = lua_gettop(L);
	// push manager table
	lua_getglobal(L, "interserverclient");
	// try to get the function table
	lua_pushstring(L, fieldname);
	lua_rawget(L, start + 1);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		// Create the key
		lua_newtable(L);
		lua_setfield(L, start + 1, fieldname);
		// push the value to the stack
		lua_getfield(L, start + 1, fieldname);
	}
	lua_pushstring(L, subfield);
	lua_pushvalue(L, start);
	lua_rawset(L, start + 2);
	lua_pop(L, lua_gettop(L) - start);
}

auto getFunction = [](lua_State *L, int index, const char *type, const char *sub) {
	int start = lua_gettop(L);
	lua_getglobal(L, "interserverclient");
	lua_pushstring(L, type);
	lua_rawget(L, -2);
	if (lua_istable(L, -1)) {
		lua_pushstring(L, sub);
		lua_rawget(L, -2);

		lua_insert(L, start + 1);
	}
	lua_pop(L, lua_gettop(L) - start - 1);
};

int interserverclient_requestNotify(lua_State *L)
{
	Capability cap = lua_tocapability(L, 1);
	lua_remove(L, 1);
	createInterserverclientTable(L, "notify", cap.name.c_str());
	
	auto notifyCallback = [L, cap](bool status) {
		getFunction(L, 1, "notify", cap.name.c_str());
		lua_pushboolean(L, status);

		lua_pcall(L, 1, 0, 0); 
	};

	g_client->requestNotify(cap, notifyCallback);

	return 0;
}

int interserverclient_requestPacketSerializable(lua_State *L)
{
	Capability cap = lua_tocapability(L, 1);
	lua_remove(L, 1);
	std::string className = lua_tostring(L, 1);
	lua_remove(L, 1);
	PacketSerializable *data = lua_topacketserializable(L, 1);
	lua_remove(L, 1);
	createInterserverclientTable(L, "request", className.c_str());

	auto callback = [L, className](PacketPtr packet) {
		getFunction(L, 1, "request", className.c_str());

		lua_pushpacket(L, packet.get());

		lua_pcall(L, 1, 0, 0);
	};

	g_client->requestPacketSerializable(cap, className, *data, callback);

	return 0;
}

int interserverclient_registerPacketSerializableHandler(lua_State *L)
{
	std::string className = lua_tostring(L, 1);
	lua_remove(L, 1);
	createInterserverclientTable(L, "handler", className.c_str());

	auto callback = [L, className](PacketPtr inpacket, PacketPtr outpacket) {
		getFunction(L, 1, "handler", className.c_str());

		lua_pushpacket(L, inpacket.get());
		lua_pushpacket(L, outpacket.get());

		if (lua_pcall(L, 2, 0, 0))
		{
			std::cout << lua_tostring(L, -1) << std::endl;
		}

	};

	g_client->registerPacketSerializableHandler(className, callback);

	return 0;
}

const luaL_Reg interserverClientLib[] = {
	{ "addCapability", interserverclient_addCapability },
	{ "removeCapability", interserverclient_removeCapability },
	{ "requestNotify", interserverclient_requestNotify },
	{ "requestPacketSerializable", interserverclient_requestPacketSerializable },
	{ "registerPacketSerializableHandler", interserverclient_registerPacketSerializableHandler },
	{ NULL, NULL }
};

int interserverclient_registerLib(lua_State *L)
{
	if (auto script = g_script.lock()) {
		luaL_newlib(script->getEnv(), interserverClientLib);
		return 1;
	}

	return 0;
}

InterserverClient::InterserverClient(boost::asio::io_service &ioService)
	: NetworkConnection(ioService), m_keepalive(ioService)
{
}


InterserverClient::~InterserverClient()
{
}


void InterserverClient::initialize()
{
	if (auto script = g_script.lock()) {
		luaL_requiref(script->getEnv(), "interserverclient", interserverclient_registerLib, 1);
		lua_pop(script->getEnv(), 1);
	}
}

void InterserverClient::startConnect()
{
	// Connection closed, reschedule reconnection in 5 seconds
	m_keepalive.expires_from_now(boost::posix_time::seconds(5));
	m_keepalive.async_wait([this](boost::system::error_code ec) {
		if (!ec) {
			this->connect();
		}
	});
}

void InterserverClient::connect()
{
	tcp::resolver resolver(socket().get_io_service());
	boost::system::error_code ec;
	if (auto settings = g_settings.lock()) {
		auto iEndpoint = resolver.resolve({ settings->getString("interserver_address"), settings->getString("interserver_port") }, ec);
		std::string username = encipher(settings->getString("interserver_username")), password = encipher(settings->getString("interserver_password"));

		if (!ec) {
			boost::asio::async_connect(socket(), iEndpoint, [this, username, password](boost::system::error_code ec, tcp::resolver::iterator i) {
				if (!ec) {
					ConnectionSuccess();

					PacketPtr packet(new Packet);

					packet->push<uint8_t>(0xF1)
						.push<uint16_t>(protocolVersion)
						.push<std::string>(username)
						.push<std::string>(password);

					send(packet, [this](boost::system::error_code ec, size_t) {
						if (ec) std::cout << "InterserverClient::async_write error: " << ec.message() << std::endl;
						else {
							keepalive();

							sendCapabilityList();

							receive();
						}
					});
				}
				else {
					ConnectionFailed();
					std::cout << "InterserverClient::async_connect error: " << ec.message() << std::endl;
					startConnect();
				}
			});
		}
		else {
			ConnectionFailed();
			std::cout << "InterserverClient::resolve error: " << ec.message() << std::endl;
			startConnect();
		}
	}
}

void InterserverClient::close()
{
	m_keepalive.cancel();
	m_keepalive.expires_from_now(boost::posix_time::seconds(0));

	NetworkConnection::close();
}

void InterserverClient::addCapability(const Capability &capability)
{
	auto iCapability = m_capabilities.find(capability);
	if (iCapability != m_capabilities.end()) iCapability->second++;
	else {
		m_capabilities[capability] = 1;

		// If connected, send to the server that we now accept a new capability
		if (socket().is_open()) {
			PacketPtr packet(new Packet);

			packet->push<uint16_t>(0x0001).push<PacketSerializable>(capability);

			send(packet);
		}
	}
}

void InterserverClient::removeCapability(const Capability &capability)
{
	auto iCapability = m_capabilities.find(capability);

	if (iCapability != m_capabilities.end()) {
		if (--iCapability->second == 0) {
			m_capabilities.erase(iCapability);

			// If connected, send to the server that we do not accept anymore a capability
			if (socket().is_open()) {
				PacketPtr packet(new Packet);

				packet->push<uint16_t>(0x0002).push<PacketSerializable>(capability);

				send(packet);
			}
		}
	}
}

void InterserverClient::requestNotify(const Capability &capability, notification_t::function_type &&callback)
{
	PacketPtr packet(new Packet);
	packet->push<uint16_t>(0x0005).push<PacketSerializable>(capability);
	send(packet);

	m_notifications[capability].push(callback);
}

void InterserverClient::requestPacketSerializable(const Capability &capability, const std::string &className, const PacketSerializable& data, requestpacket_t callback)
{
	static uint32_t id = 0;

	PacketPtr packet(new Packet);
	packet->push<uint16_t>(0x0007)
		.push<PacketSerializable>(capability)
		.push<uint8_t>((uint8_t)RelayOperation::RequestPacketSerializable)
		.push<uint32_t>(++id)
		.push<std::string>(className)
		.push<PacketSerializable>(data);

	m_relays[id] = callback;

	send(packet);
}

void InterserverClient::registerPacketSerializableHandler(const std::string& className, requestpackethandler_t handler)
{
	m_handlers[className] = handler;
}

void InterserverClient::sendCapabilityList()
{
	if (!m_capabilities.empty()) {
		PacketPtr packet(new Packet);

		packet->push<uint16_t>(0x0003).push<uint16_t>(m_capabilities.size());

		for (auto &i : m_capabilities) {
			packet->push<PacketSerializable>(i.first);
		}

		send(packet);
	}
}

void InterserverClient::keepalive()
{
	m_keepalive.expires_from_now(boost::posix_time::seconds(4));
	m_keepalive.async_wait([this](boost::system::error_code ec) {
		if (!ec) {
			setTimeout(NetworkConnection::readTimeout);

			PacketPtr packet(new Packet);
			
			packet->push<uint16_t>(0x0000);

			send(packet, [this](boost::system::error_code ec, size_t) {
				if (ec) {
					std::cout << "InterserverClient::keepalive error: " << ec.message() << std::endl;
					this->close();
					this->startConnect();
				}
				else this->keepalive();
			});
		}
		else this->keepalive();
	});
}

void InterserverClient::receive()
{
	beginReading([this](PacketPtr packet, boost::system::error_code e) {
		if (!e) {
			uint32_t checksum = packet->pop<uint32_t>();
			if (this->getLastChecksum() != checksum)
				return;

			uint16_t handle = packet->pop<uint16_t>();

			switch (handle) {
			case 0x0006:
			{
				Capability cap;
				packet->get(cap);
				bool state = packet->pop<bool>();

				auto range = m_notifications.equal_range(cap);
				for (auto cb = range.first; cb != range.second; ++cb) {
					cb->second(state);
				}

				break;
			}
			case 0x0007:
			{
				uint32_t serverId = packet->pop<uint32_t>();
				RelayOperation operation = (RelayOperation)packet->pop<uint8_t>();
				switch (operation) {
				case RelayOperation::RequestPacketSerializable: {
					uint32_t clientId = packet->pop<uint32_t>();
					std::string className = packet->pop<std::string>();

					auto handler = m_handlers.find(className);
					if (handler == m_handlers.end())
						break;

					PacketPtr outPacket(new Packet);
					outPacket->push<uint16_t>(0x0008)
						.push<uint32_t>(serverId)
						.push<uint8_t>((uint8_t)RelayOperation::RequestPacketSerializable)
						.push<uint32_t>(clientId);
					handler->second(packet, outPacket);
					send(outPacket);

					m_handlers.erase(handler);
				}
				}

				break;
			}
			case 0x0008:
			{
				RelayOperation operation = (RelayOperation)packet->pop<uint8_t>();
				switch (operation) {
				case RelayOperation::RequestPacketSerializable: {
					uint32_t clientId = packet->pop<uint32_t>();

					auto relay = m_relays.find(clientId);
					if (relay != m_relays.end())
						relay->second(packet);
					break;

					m_relays.erase(relay);
				}
				}
			}
			}

			receive();
			return;
		}

		close();
	});
}
  
std::string InterserverClient::encipher(const std::string &what)
{
	std::string key, algo;
	if (auto settings = g_settings.lock()) {
		key = settings->getString("interserver_key");
		algo = settings->getString("interserver_algorithm");
	}
	else return what;

	unsigned char *digest = new unsigned char[EVP_MAX_MD_SIZE];
	unsigned int len = EVP_MAX_MD_SIZE;

	HMAC_CTX hmac;
	HMAC_CTX_init(&hmac);
#pragma region HMAC initializers
#ifndef OPENSSL_NO_SHA
	if (algo.compare("sha") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_sha(), NULL);
	else if (algo.compare("sha1") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_sha1(), NULL);
#else
	if (false) {}
#endif
#ifndef OPENSSL_NO_SHA256
	else if (algo.compare("sha-224") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_sha224(), NULL);
	else if (algo.compare("sha-256") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_sha256(), NULL);
#endif
#ifndef OPENSSL_NO_SHA512
	else if (algo.compare("sha-384") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_sha384(), NULL);
	else if (algo.compare("sha-512") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_sha512(), NULL);
#endif
#ifndef OPENSSL_NO_WHIRLPOOL
	else if (algo.compare("whirlpool") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_whirlpool(), NULL);
#endif
#ifndef OPENSSL_NO_MD4
	else if (algo.compare("md4") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_md4(), NULL);
#endif
#ifndef OPENSSL_NO_MD5
	else if (algo.compare("md5") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_md5(), NULL);
#endif
#ifndef OPENSSL_NO_RIPEMD
	else if (algo.compare("ripemd160") == 0)
		HMAC_Init_ex(&hmac, key.c_str(), key.length(), EVP_ripemd160(), NULL);
#endif
	else {
		HMAC_CTX_cleanup(&hmac);
		std::cout << "InterserverClient::encipher: invalid algorithm! using plaintext!" << std::endl;
		return what;
	}
#pragma endregion
	HMAC_Update(&hmac, (const unsigned char*)what.c_str(), what.length());
	HMAC_Final(&hmac, digest, &len);
	HMAC_CTX_cleanup(&hmac);

	char *buffer = new char[len * sizeof (char)* 2 + 1];

	for (unsigned int i = 0; i < len; i++)
		sprintf(buffer + i, "%02x", digest[i]);

	std::string digeststr(buffer);
	delete[] buffer;
	return digeststr;
}

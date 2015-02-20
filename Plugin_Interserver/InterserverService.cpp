#include "InterserverService.h"
#include "Settings.h"
#include "Packet.h"
#include "NetworkConnection.h"
#include "LoggerComponent.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include <openssl/engine.h>
#include <openssl/hmac.h>

extern std::weak_ptr<Settings> g_settings;
extern LoggerComponent *g_logger;

std::mt19937 g_random(0);

InterserverService::InterserverService()
{
	// Try to reseed the RNG
	try {
		std::random_device rd;
		g_random.seed(rd());
	}
	catch (std::exception) {
		g_random.seed(std::chrono::system_clock::now().time_since_epoch().count());
	}

	ENGINE_load_builtin_engines();
	ENGINE_register_all_complete();

	// Register keepalive handler
	m_handlers[0] = [](NetworkConnectionPtr c, PacketPtr p) {
		std::stringstream ss;
		ss << "Heartbeat received from " << c->socket().remote_endpoint().address().to_string() << ":" << c->socket().remote_endpoint().port();
		if (g_logger) g_logger->log(LogLevel::Debug, ss.str());

		return true;
	};

	// Register add capability handler
	m_handlers[1] = [this](NetworkConnectionPtr c, PacketPtr p) {
		Capability capability;
		p->get(capability);
		capability.connection = c;

		registerCapability(capability);

		return true;
	};

	// Register remove capability handler
	m_handlers[2] = [this](NetworkConnectionPtr c, PacketPtr p) {
		Capability capability;
		p->get(capability);
		capability.connection = c;

		auto i = m_capabilities.equal_range(capability.name);
		if (i.first != i.second) {
			auto remote = c->socket().remote_endpoint();
			for (auto iCap = i.first; iCap != i.second; ++iCap) {
				auto ep = iCap->second.serviceEndpoint;
				if (ep.port() == capability.serviceEndpoint.port() && ep.address().to_string().compare(capability.serviceEndpoint.address().to_string()) == 0) {
					m_capabilities.erase(iCap);

					std::stringstream ss;
					ss << "Removing capability " << capability.name << " from " << ep.address().to_string() << ":" << ep.port();
					if (g_logger) g_logger->log(LogLevel::Information, ss.str());

					return true;
				}
			}
		}

		return true;
	};

	// Register capability list handler
	m_handlers[3] = [this](NetworkConnectionPtr c, PacketPtr p) {
		auto size = p->pop<uint16_t>();

		for (uint16_t i = 0; i < size; i++) {
			Capability capability;
			p->get(capability);
			capability.connection = c;

			registerCapability(capability);
		}

		return true;
	};

	// Register relay message handler
	m_handlers[4] = [this](NetworkConnectionPtr c, PacketPtr p) {
		auto capability = p->pop<std::string>();

		auto i = m_capabilities.equal_range(capability);
		PacketPtr response(new Packet);
		response->push<uint16_t>(0x0004);

		if (i.first != i.second) {
			auto len = p->size() - p->pos();
			uint8_t* b = new uint8_t[len];
			p->get(len, b);
			
			for (auto connection = i.first; connection != i.second; ++connection) {
				if (auto con = connection->second.connection.lock()) {
					PacketPtr sendPacket(new Packet);
					sendPacket->push<uint16_t>(0x0005).copy(b, len);
					con->send(sendPacket);
				}
			}

			response->push<bool>(true);

			delete[] b;
		}
		else {
			response->push<bool>(false);
		}

		c->send(response);

		return true;
	};

	// Register notify on capability registered
	m_handlers[6] = [this](NetworkConnectionPtr c, PacketPtr p) {
		Capability capability;
		p->get(capability);

		m_capabilityNotify.emplace(capability.name, capability);

		return true;
	};

	// Register relay & notify answer handler
	m_handlers[7] = [this](NetworkConnectionPtr c, PacketPtr p) {
		Capability capability;
		p->get(capability);

		auto i = m_capabilities.equal_range(capability.name);
		for (auto con = i.first; con != i.second; ++con) {
			auto connection = con->second.connection.lock();
			if (connection && connection->socket().is_open()) {
				while (true) {
					uint32_t requestIndex = g_random();

					if (m_relay.find(requestIndex) != m_relay.end()) continue;

					m_relay.emplace(requestIndex, c);

					auto len = p->size() - p->pos();
					uint8_t* b = new uint8_t[len];
					p->get(len, b);
					PacketPtr out(new Packet);
					out->push<uint16_t>(0x0007).push<uint32_t>(requestIndex).copy(b, len);
					delete[] b;
					connection->send(out);

					break;
				}
			}
		}
		
		return true;
	};

	m_handlers[8] = [this](NetworkConnectionPtr c, PacketPtr p) {
		uint32_t index = p->pop<uint32_t>();

		auto requester = m_relay.find(index);
		if (requester == m_relay.end()) return true;

		auto len = p->size() - p->pos();
		if (len > 0) {
			uint8_t* b = new uint8_t[len];
			p->get(len, b);
			PacketPtr out(new Packet);
			out->push<uint16_t>(0x0008).copy(b, len);
			delete[] b;
			requester->second->send(out);
		}

		m_relay.erase(requester);

		return true;
	};
}


InterserverService::~InterserverService()
{
	ENGINE_cleanup();
}

unsigned short InterserverService::getBindPort()
{
	if (auto settings = g_settings.lock())
		return (unsigned short)settings->getUnsigned("interserver_port");

	return 0U;
}

std::string InterserverService::getBindAddress()
{
	if (auto settings = g_settings.lock())
		return settings->getString("interserver_address");

	return "localhost";
}

bool InterserverService::canHandle(NetworkConnectionPtr connection, PacketPtr packet)
{
	return packet->peek<uint8_t>() == 0xF1;
}

bool InterserverService::handleFirst(NetworkConnectionPtr connection, PacketPtr packet)
{
	std::stringstream ss;

	// Verify protocol version
	uint16_t protocolVersion = packet->pop<uint16_t>();

	if (protocolVersion < InterserverService::minProtocolVersion || protocolVersion > InterserverService::maxProtocolVersion) {
		ss << "Interserver: Received invalid protocol version from " << connection->socket().remote_endpoint().address().to_string();
		if (g_logger) g_logger->log(LogLevel::Information, ss.str());

		return false;
	}

	// Request authentication
	std::string receivedUsername = packet->pop<std::string>();
	std::string receivedPassword = packet->pop<std::string>();

	// Verify data
	std::string username, password;
	if (auto settings = g_settings.lock()) {
		username = encipher(settings->getString("interserver_username"));
		password = encipher(settings->getString("interserver_password"));
	}

	if (username.compare(receivedUsername) != 0 || password.compare(receivedPassword) != 0) {
		ss << "Interserver: Failed connection attempt from " << connection->socket().remote_endpoint().address().to_string();
		if (g_logger) g_logger->log(LogLevel::Warning, ss.str());
	}

	ss << "InterserverService: Connection success from " << connection->socket().remote_endpoint().address().to_string();
	if (g_logger) g_logger->log(LogLevel::Information, ss.str());
	std::cout << ">>> " << ss.str() << std::endl;

	return true;
}

bool InterserverService::handle(NetworkConnectionPtr connection, PacketPtr packet)
{
	uint16_t code = packet->peek<uint16_t>();

	// Check if packet handler is registered
	for (auto &handler : m_handlers) {
		if (handler.first == code) {
			// Handler found, skip the handler code and return whatever the handler throws.
			packet->skip(sizeof (uint16_t));
			return handler.second(connection, packet);
		}
	}

	// No handler found -- fake request or plugin needed, we should disconnect.
	return false;
}

void InterserverService::addHandler(uint16_t code, InterserverService::PacketHandler handler)
{
	if (code >= 0x0010) // Codes 0x0000~0x000F are reserved for interserver communication
		m_handlers[code] = handler;
}

void InterserverService::removeHandler(uint16_t code)
{
	if (code < 0x0010)
		return;

	auto i = m_handlers.find(code);

	if (i != m_handlers.end())
		m_handlers.erase(i);
}

void InterserverService::removeConnection(NetworkConnectionPtr connection)
{
	auto remote = connection->socket().remote_endpoint();
	for (auto i = m_capabilities.begin(); i != m_capabilities.end(); ++i) {
		if (auto con = i->second.connection.lock()) {
			auto ep = con->socket().remote_endpoint();
			if (ep.port() == remote.port() && ep.address().to_string().compare(remote.address().to_string()) == 0) {
				m_capabilities.erase(i);
				i = m_capabilities.begin();
			}
		}
		else {
			m_capabilities.erase(i);
			i = m_capabilities.begin();
		}
	}

	std::cout << ">>> InterserverService: Connection closed from " << connection->socket().remote_endpoint().address().to_string() << std::endl;
}

std::string InterserverService::encipher(const std::string &what)
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
		std::cout << "InterserverService::encipher: invalid algorithm! using plaintext!" << std::endl;
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

void InterserverService::registerCapability(const Capability &capability)
{
	std::stringstream ss;
	ss << "Adding capability " << capability.name << " to " << capability.serviceEndpoint.address().to_string() << ":" << capability.serviceEndpoint.port();
	if (g_logger) g_logger->log(LogLevel::Information, ss.str());

	auto i = this->m_capabilities.equal_range(capability.name);
	if (i.first == i.second) {
		m_capabilities.emplace(capability.name, capability);
	}
	else {
		bool found = false;
		for (auto iCap = i.first; iCap != i.second; ++iCap) {
			auto ep = iCap->second.serviceEndpoint;
			if (ep.port() == capability.serviceEndpoint.port() && ep.address().to_string().compare(capability.serviceEndpoint.address().to_string()) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			m_capabilities.emplace(capability.name, capability);
		}
		else if (g_logger) {
			ss.str("");
			ss << capability.serviceEndpoint.address().to_string() << ":" << capability.serviceEndpoint.port() << " already have capability " << capability.name << " . ignoring ...";
			g_logger->log(LogLevel::Information, ss.str());
			return;
		}
	}
	
	// Notify that a capability was registered
	i = m_capabilityNotify.equal_range(capability.name);
	for (auto iCap = i.first; iCap != i.second; ++iCap) {
		if (auto connection = iCap->second.connection.lock()) {
			PacketPtr packet(new Packet);
			packet->push<uint8_t>(5).push<bool>(true).push<PacketSerializable>(capability);
			connection->send(packet);
		}
	}
}

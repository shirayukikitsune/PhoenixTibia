#include "InterserverClient.h"
#include "Settings.h"
#include "Packet.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include <openssl/evp.h>
#include <openssl/hmac.h>

extern std::weak_ptr<Settings> g_settings;

using boost::asio::ip::tcp;

InterserverClient::InterserverClient(boost::asio::io_service &ioService)
: NetworkConnection(ioService), m_keepalive(ioService)
{
}


InterserverClient::~InterserverClient()
{
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

					packet->push<uint8_t>(0xF1).push<std::string>(username).push<std::string>(password);

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

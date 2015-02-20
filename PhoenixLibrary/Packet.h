#pragma once

#include "Tools.h"

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

class Packet;

class PacketSerializable
{
public:
	/**
	 * Read this struct from a Packet
	 */
	virtual void read(Packet &packet) { }

	/**
	 * Write this struct into a Packet
	 */
	virtual void write(Packet &packet) const { }
};

class Packet
{
public:
	enum { maxPacketSize = 0x7fff, headerSize = 2 };
	typedef std::vector<uint8_t> buffer_t;

	Packet() {
		reset();
	}
	Packet(Packet *copy) {
		m_buffer.resize(copy->m_buffer.capacity());
		std::copy(copy->m_buffer.begin(), copy->m_buffer.end(), m_buffer.begin());
		m_start = copy->m_start;
		m_position = copy->m_position;
		m_length = copy->m_length;
	}
	~Packet() { }

	Packet& reset() {
		m_start = 8;
		m_position = 8;
		m_length = 0;

		m_buffer.resize(512, 0);

		return *this;
	}

	template <typename T>
	Packet& push(const T &value) {
		while (m_position + sizeof(T) > m_buffer.size()) {
			m_buffer.resize(m_buffer.size() + 512, 0);
		}
		*(T*)&m_buffer[m_position] = value;
		m_position += sizeof (T);
		m_length += sizeof (T);

		return *this;
	}

	template <>
	Packet& push<std::string>(const std::string &str) {
		push<uint16_t>((uint16_t)str.length());

		while (m_position + str.length() > m_buffer.size()) {
			m_buffer.resize(m_buffer.size() + 512, 0);
		}

		std::memcpy(&m_buffer[m_position], str.c_str(), str.length());
		m_position += str.length();
		m_length += str.length();

		return *this;
	}

	template <>
	Packet& push<PacketSerializable>(const PacketSerializable &s) {
		s.write(*this);

		return *this;
	}

	Packet& push(const PacketSerializable *s) {
		s->write(*this);

		return *this;
	}

	Packet& copy(const uint8_t* buffer, size_t len) {
		while (m_position + len > m_buffer.size()) {
			m_buffer.resize(m_buffer.size() + 512);
		}

		std::memcpy(&m_buffer[m_position], buffer, len);
		m_position += len;
		m_length += len;

		return *this;
	}

	template <typename T>
	T peek() {
		return *(T*)&m_buffer[m_position];
	}

	template <>
	std::string peek<std::string>() {
		uint16_t len = pop<uint16_t>();
		std::string out((char*)&m_buffer[m_position], len);
		m_position -= sizeof uint16_t;
		return out;
	}

	template <typename T>
	T pop() {
		T value = peek<T>();
		m_position += sizeof (T);

		return value;
	}

	template <>
	std::string pop<std::string>() {
		std::string::size_type s = pop<uint16_t>();
		std::string value((char*)&m_buffer[m_position], s);
		m_position += s;
		return value;
	}

	Packet& get(PacketSerializable &s) {
		s.read(*this);

		return *this;
	}

	Packet& get(PacketSerializable *s) {
		s->read(*this);

		return *this;
	}

	Packet& get(size_t len, uint8_t* value) {
		std::memcpy(value, &m_buffer[m_position], len);
		m_position += len;

		return *this;
	}

	buffer_t::pointer data() {
		return m_buffer.data();
	}

	Packet& skip(size_t len) {
		m_position += len;

		return *this;
	}

	Packet& pad(size_t len) {
		m_length += len;

		return skip(len);
	}

	buffer_t::size_type capacity() {
		return maxPacketSize;
	}

	buffer_t::size_type size() {
		return m_length;
	}

	Packet& size(buffer_t::size_type p) {
		m_length = p;

		return *this;
	}

	buffer_t::size_type max_size() {
		return maxPacketSize;
	}

	buffer_t::size_type start() {
		return m_start;
	}

	buffer_t::size_type pos() {
		return m_position;
	}

	Packet& pos(buffer_t::size_type p) {
		m_position = p;

		return *this;
	}

	buffer_t::size_type parseLength() {
		m_length = ((buffer_t::size_type)m_buffer[0] | (buffer_t::size_type)m_buffer[1] << 8) + headerSize;
		return m_length - headerSize;
	}

	void writeMessageLength() { addHeader((uint16_t)(m_length)); }
	void addCryptoHeader(bool addChecksum)
	{
		if (addChecksum)
			addHeader((adlerChecksum(&m_buffer[m_start], m_length)));

		addHeader((uint16_t)(m_length));
	}

private:
	buffer_t::size_type m_start;
	buffer_t::size_type m_position;
	buffer_t::size_type m_length;
	buffer_t m_buffer;

	template <typename T>
	inline void addHeader(T value)
	{
		m_start -= sizeof(T);
		*(T*)&m_buffer[m_start] = value;
		m_length += sizeof(T);
	}
};


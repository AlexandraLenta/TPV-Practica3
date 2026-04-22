// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once

#include <iostream>
#include <SDL_net.h>
#include <cassert>
#include <memory>
#include <functional>
#include <type_traits>


/*
 * This class provides some utilities for serialization and
 * deserialization of data, in particular for structs. It also
 * includes some method to send and receive data over TCP/UDP.
 * The one of TCP sends also the size of the buffer we are sending,
 * so the other side knows how many bytes to expect.
 */

 // START OF MACROS
 /*
  The following are macros that allows inserting serialization methods
  automatically into a struct:

  _IMPL_SERIALIZATION_(a,b,c,...)

  Define methods serialize and deserialize for fields a, b, c, etc.
  This line is supposed to be added to a struct with corresponding
  fields.

  In order to call serialize of a base struct, we can use something
  like the following:

  _IMPL_SERIALIZATION_(static_cast<BaseType&>(*this),a,b,...)

  Example:

  struct Message {
  Uint8 id;

  _IMPL_SERIALIZATION_(id)
  };

  struct InfoMsg : Message {
  Uint8 x[10];
  Uint32 y;

  _IMPL_SERIALIZATION_(static_cast<Message&>(&this),x,y)
  };

  int main(int argc, char **argv) {
  Uint8 buffer[100];

  // serialize
  InfoMsg a;
  a.id = 25;
  for (auto i = 0u; i < 10; i++)
  a.x[i] = 2*i;
  a.y = 1234321;
  a.serialize(buffer);

  // deserialize
  InfoMsg b;
  b.deserialize(buffer);
  std::cout << "b.id = " << (int)b.id << std::endl;
  for (auto i = 0u; i < 10; i++)
  std::cout << "b.x[" << i << "] = " << (int)b.x[i] << std::endl;
  std::cout << "b.y = " << b.y << std::endl;

  return 0;
  }

  */
#define _IMPL_SERIALIZATION_(...) \
	inline Uint8* serialize(Uint8 *buf) {\
		buf = SDLNetUtils::serialize_all(buf, __VA_ARGS__);\
		return buf;\
	}\
	inline Uint8* deserialize(Uint8 *buf) {\
		buf = SDLNetUtils::deserialize_all(buf,__VA_ARGS__);\
		return buf;\
	}

  // END OF MACROS

class SDLNetUtils {

	template<typename T2, typename T1>
	static inline T2* safe_reinterpret_cast(T1* v) {
		// 1. Enforce alignment safety
		static_assert(alignof(T2) <= alignof(T1),
			"Target type T2 requires stricter alignment than source type T1.");

		// 2. Ensure we are dealing with standard objects (no weird member-pointers)
		static_assert(std::is_pointer_v<T2*> && std::is_pointer_v<T1*>,
			"Safe cast only supports standard object pointers.");

		return static_cast<T2*>(static_cast<void*>(v));
	}

	template<typename T2, typename T1>
	static inline T2& safe_reinterpret_cast(T1& v) {
		static_assert(alignof(T2) <= alignof(T1),
			"Target type T2 requires stricter alignment than source type T1.");

		return reinterpret_cast<T2&>(v);
	}

public:

	// A type for the message size
	//
	using msg_size_t = Uint16;

	// A struct for encapsulating a buffer.
	//
	// All uses of buff_t below are stack-allocated, which should be OK because the buffer is relatively
	// small. If it is a problem we can convert it to heap-allocated and use a shared_ptr when returning
	// it to the user. We can also make the buffer global, but then we won't have thread safety.
	//
	struct buff_t {
		static constexpr std::size_t buffer_size = 1024; // all messages should be smaller than this size
		Uint8 data[buffer_size];
		msg_size_t size = 0;
		bool error = false;
	};

	/*
	 * Initialize SDL
	 */
	inline static void init_SDLNet() {
		if (!NET_Init()) {
			print_SDLNet_error_and_exit();
		}
	}

	/*
	 * Close SDLNet
	 */
	inline static void close_SDLNet() {
		NET_Quit();
	}

	/*
	 * Print the SDLNet error
	 */
	inline static void print_SDLNet_error() {
		std::cout << "Error: " << SDL_GetError() << std::endl;
	}

	/*
	 * Print the SDLNet error and exit
	 */
	inline static void print_SDLNet_error_and_exit(int code = 1) {
		print_SDLNet_error();
		exit(code);
	}

	/*
	 *  print an IPv4 given as Uin32
	 */
	inline static void print_ip4(Uint32 ip, bool newline = false) {
		std::cout << (ip & 0xFF) << "." //
			<< ((ip >> 8) & 0xFF) << "." //
			<< ((ip >> 16) & 0xFF) << "." //
			<< ((ip >> 24) & 0xFF);
		if (newline)
			std::cout << std::endl;
	}

	/*
	 * Convert between Host (Big/Little-Endian) and Network Endianness (Big-Endian)
	 *
	 * sdlnet_hton: converts from Host Endianness to Network Endianness
	 * sdlnet_ntoh: converts from network Endianness to Host Endianness
	 *
	 * They are implemented for Uint64, Uint32, Uint16n and Uint8 (obviously Uint8
	 * is not needed, it is just for compatibility of some code that uses
	 * templates.
	 *
	 */
	inline static Uint64 sdlnet_hton(Uint64 v) {
		return SDL_Swap64BE(v);
	}

	inline static Uint64 sdlnet_ntoh(Uint64 nv) {
		return SDL_Swap64BE(nv);
	}

	inline static Uint32 sdlnet_hton(Uint32 v) {
		return SDL_Swap32BE(v);
	}

	inline static Uint32 sdlnet_ntoh(Uint32 nv) {
		return SDL_Swap32BE(nv);
	}

	inline static Uint16 sdlnet_hton(Uint16 v) {
		return SDL_Swap16BE(v);
	}

	inline static Uint16 sdlnet_ntoh(Uint16 nv) {
		return SDL_Swap16BE(nv);
	}

	inline static Uint8 sdlnet_hton(Uint8 v) {
		return v;
	}

	inline static Uint8 sdlnet_ntoh(Uint8 nv) {
		return nv;
	}

	/**
	 *  SERIALIZATION and DESERIALIZATION of FIXED LENGTH TYPES
	 */

	 // Serializing a list of arguments (instead of calling
	 // serialize for each one)
	 // The base-case, no arguments
	static inline Uint8* serialize_all(Uint8* buf) {
		return buf;
	}

	// The case of other non-array types
	template<typename T, typename ...Ts>
	static inline Uint8* serialize_all(Uint8* buf, T&& a, Ts &&...args) {
		buf = serialize(a, buf); // This is defined below for different value of T
		return serialize_all(buf, std::forward<Ts>(args)...);
	}

	// Deserializing a list of arguments (instead of calling
	// deserialize for each one)

	// the base-case, no argument
	static inline Uint8* deserialize_all(Uint8* buf) {
		return buf;
	}

	// the case of other non-array types
	template<typename T, typename ...Ts>
	static inline Uint8* deserialize_all(Uint8* buf, T&& a, Ts &&...args) {
		buf = deserialize(a, buf); // This is defined below for different T
		return deserialize_all(buf, std::forward<Ts>(args)...);
	}

	// Serialization/deserialization of a single value, for different types

	// The case when the value to serialize/deserialize is a struct. It must
	// have a serialize/deserialize
	template<typename T>
	inline static Uint8* serialize(T& v, Uint8* buf) {
		return v.serialize(buf);
	}

	template<typename T>
	inline static Uint8* deserialize(T& v, Uint8* buf) {
		return v.deserialize(buf);
	}

	// The case when the value to an array of size N
	template<typename T, std::size_t N>
	inline static Uint8* serialize(T(&a)[N], Uint8* buf) {
		return serialize_array(a, N, buf);
	}

	template<typename T, std::size_t N>
	inline static Uint8* deserialize(T(&a)[N], Uint8* buf) {
		return deserialize_array(a, N, buf);
	}

	// The case for char.
	//
	// It is treated as if it where Uint8. Use
	// it only to send/receive chars with non-negative values, i.e.,
	// ASCII codes
	inline static Uint8* serialize(char& v, Uint8* buf) {
		Uint8& aux_v = safe_reinterpret_cast<Uint8>(v);
		return serialize_uint(aux_v, buf);
	}

	inline static Uint8* deserialize(char& v, Uint8* buf) {
		Uint8& aux_v = safe_reinterpret_cast<Uint8>(v);
		return deserialize_uint(aux_v, buf);
	}

	// The case for Uint8
	inline static Uint8* serialize(Uint8& v, Uint8* buf) {
		return serialize_uint(v, buf);
	}

	inline static Uint8* deserialize(Uint8& v, Uint8* buf) {
		return deserialize_uint(v, buf);
	}

	// The case for Uint16
	inline static Uint8* serialize(Uint16& v, Uint8* buf) {
		return serialize_uint(v, buf);
	}

	inline static Uint8* deserialize(Uint16& v, Uint8* buf) {
		return deserialize_uint(v, buf);
	}

	// The case for Uint32
	inline static Uint8* serialize(Uint32& v, Uint8* buf) {
		return serialize_uint(v, buf);
	}

	inline static Uint8* deserialize(Uint32& v, Uint8* buf) {
		return deserialize_uint(v, buf);
	}

	// The case for Uint64
	inline static Uint8* serialize(Uint64& v, Uint8* buf) {
		return serialize_uint(v, buf);
	}

	inline static Uint8* deserialize(Uint64& v, Uint8* buf) {
		return deserialize_uint(v, buf);
	}

	// The case for Sint8
	inline static Uint8* serialize(Sint8& v, Uint8* buf) {
		return serialize_sint<Uint8>(v, buf);
	}

	inline static Uint8* deserialize(Sint8& v, Uint8* buf) {
		return deserialize_sint<Uint8>(v, buf);
	}

	// case for Sint16
	inline static Uint8* serialize(Sint16& v, Uint8* buf) {
		return serialize_sint<Uint16>(v, buf);
	}

	inline static Uint8* deserialize(Sint16& v, Uint8* buf) {
		return deserialize_sint<Uint16>(v, buf);
	}

	// case for Sint32
	inline static Uint8* serialize(Sint32& v, Uint8* buf) {
		return serialize_sint<Uint32>(v, buf);
	}

	inline static Uint8* deserialize(Sint32& v, Uint8* buf) {
		return deserialize_sint<Uint32>(v, buf);
	}

	// case for Sint64
	inline static Uint8* serialize(Sint64& v, Uint8* buf) {
		return serialize_sint<Uint64>(v, buf);
	}

	inline static Uint8* deserialize(Sint64& v, Uint8* buf) {
		return deserialize_sint<Uint64>(v, buf);
	}

	// FLOAT: there are several ways to serialize/deserialize

	// FLOAT - just send the bytes over
	//
	// Treating the 4 bytes as Uint32 and serialize it should
	// work fine.

	inline static Uint8* serialize(float& v, Uint8* buf) {
		static_assert(sizeof(float) == 4, "float is not 4 bytes");
		return serialize(safe_reinterpret_cast<Uint32>(v), buf);
	}

	inline static Uint8* deserialize(float& v, Uint8* buf) {
		static_assert(sizeof(float) == 4, "float is not 4 bytes");
		return deserialize(safe_reinterpret_cast<Uint32>(v), buf);
	}

	// FLOAT - using frexp/ldexp.
	//
	// Based on https://gist.github.com/ahamez/fa4760800d454a080cd83e5963ca5bf2
	//
	// Not sure about runtime of frexp and ldexp! I guess not the same
	// on all compilers.
	//

//	constexpr static auto max_int = std::numeric_limits<std::int32_t>::max();
//
//	inline static Uint8* serialize(float &v, Uint8 *buf) {
//		static_assert( sizeof(float) == 4, "float is not 32 bits");
//
//		int32_t exp = 0;
//		int32_t mant = static_cast<std::int32_t>(max_int * std::frexp(v, &exp));
//
//		buf = serialize(exp, buf);
//		buf = serialize(mant, buf);
//
//		return buf;
//	}
//
//	inline static Uint8* deserialize(float &v, Uint8 *buf) {
//		int32_t exp = 0;
//		int32_t mant = 0;
//
//		buf = deserialize(exp, buf);
//		buf = deserialize(mant, buf);
//
//		v = std::ldexp(static_cast<float>(mant) / max_int, exp);
//
//		return buf;
//
//	}

	// FLOAT - using bit-field
	//

//	typedef union {
//		float f;
//		struct {
//			uint32_t mantisa :23;
//			uint32_t exponent :8;
//			uint32_t sign :1;
//		} parts;
//	} float_cast;
//
//	inline static Uint8* serialize(float &v, Uint8 *buf) {
//		static_assert( sizeof(float) == 4, "float is not 32 bits");
//
//		float_cast data;
//
//		data.f = v;
//		uint32_t mantisa = data.parts.mantisa;
//		uint8_t exponent = data.parts.exponent;
//		uint8_t sign = data.parts.sign;
//		buf = serialize(mantisa, buf);
//		buf = serialize(exponent, buf);
//		buf = serialize(sign, buf);
//		return buf;
//	}
//
//	inline static Uint8* deserialize(float &v, Uint8 *buf) {
//		static_assert( sizeof(float) == 4, "float is not 32 bits");
//
//		uint32_t mantisa;
//		uint8_t exponent;
//		uint8_t sign;
//
//		buf = deserialize(mantisa, buf);
//		buf = deserialize(exponent, buf);
//		buf = deserialize(sign, buf);
//
//		float_cast data;
//		data.parts.mantisa = mantisa;
//		data.parts.exponent = exponent;
//		data.parts.sign = sign;
//
//		v = data.f;
//
//		return buf;
//	}

	// (de)serialization of arrays is done, by default, by calling the serialize/deserialize
	// on each element. There are special cases for 1-byte types to use memcpy (they
	// call serialize_arraybyte) that is defined in the private section below
	//
	template<typename T>
	inline static Uint8* serialize_array(T* v, std::size_t N, Uint8* buf) {
		static_assert (!std::is_compound_v<T>);
		for (auto i = 0u; i < N; i++)
			buf = serialize(v[i], buf);
		return buf;
	}

	template<typename T>
	inline static Uint8* deserialize_array(T* v, std::size_t N, Uint8* buf) {
		static_assert (!std::is_compound_v<T>);
		for (auto i = 0u; i < N; i++)
			buf = deserialize(v[i], buf);
		return buf;
	}

	// Case for array of char.
	//
	// We copy as if it were bytes, ignoring the sign. This should be used only for
	// char* that are C-string
	inline static Uint8* serialize_array(char* v, std::size_t N, Uint8* buf) {
		return serialize_arraybyte(safe_reinterpret_cast<Uint8>(v), N, buf);
	}

	inline static Uint8* deserialize_array(char* v, std::size_t N, Uint8* buf) {
		return deserialize_arraybyte(safe_reinterpret_cast<Uint8>(v), N, buf);
	}

	// Case for array of Uint8
	inline static Uint8* serialize_array(Uint8* v, std::size_t N, Uint8* buf) {
		return serialize_arraybyte(v, N, buf);
	}

	inline static Uint8* deserialize_array(Uint8* v, std::size_t N,
		Uint8* buf) {
		return deserialize_arraybyte(v, N, buf);
	}

	/*
	 * Send/Receive messages for TCP. The purpose is to add a header to
	 * the message indicating its size, so the other size can read it
	 * correctly. The users do not receive back this size, they should identify
	 * their data by other means.
	 *
	 */

	 // send a message (msgSize bytes starting at  v) in a TCP socket with
	 // a header indicating its size, so it can be read completely at the
	 // other end
	 //
	 // We assume the called is aware of the buffer size defined below as buffSize_
	 //
	 // returns true if the message was sent correctly, otherwise false
	 //
	inline static bool send(NET_StreamSocket* sock, Uint8* v,
		msg_size_t msgSize) {

		// we add this restriction because we will receive this message using
		// the 'receive' method below.
		assert(msgSize <= buff_t::buffer_size);

		bool result = false;

		// convert the size to network endianness
		msg_size_t size = sdlnet_hton(msgSize);

		static_assert(alignof(Uint8) <= alignof(msg_size_t));

		// send the size as a header
		result = NET_WriteToStreamSocket(sock, safe_reinterpret_cast<Uint8>(&size),
			sizeof(msg_size_t));

		if (!result)
			return false;

		// send the message itself
		result = NET_WriteToStreamSocket(sock, v, msgSize);

		return result;
	}

	// receive a message that was send by method 'send' above, i.e., with
	// the size as a header.
	//
	// returns a pointer to the buffer where the message is stored, or nullptr
	// if something went wring.
	//
	// We don't return the size, the user should identify the message by other means
	//
	inline static buff_t receive(NET_StreamSocket* sock) {

		buff_t buffer;

		int result = 0;

		Uint16 size;

		// read the size of the message, the first sizeof(msgSize_t) bytes, into buffer
		result = NET_ReadFromStreamSocket(sock, &size, sizeof(msg_size_t));
		if (result == 0) {
			return buffer;
		}
		else if (result != sizeof(msg_size_t)) {
			buffer.error = true;
			return buffer;
		}

		// convert size from network endianness to host endianness
		size = sdlnet_ntoh(size);

		// THIS IS NOT SAFE DUE TO THE ALIGNMENT REQUIREMENTS OF reinterpret_cast
		//
		//		static_assert(alignof(msg_size_t)<=alignof(buffer.data));
		//		Uint16 size = sdlnet_ntoh(*reinterpret_cast<msg_size_t*>(buffer.data));

		assert(size <= buff_t::buffer_size);

		// receive size bytes
		while (buffer.size < size) {
			result = NET_ReadFromStreamSocket(sock, buffer.data + buffer.size,
				size - buffer.size);
			if (result < 0) {
				buffer.error = true;
				buffer.size = 0;
				return buffer;
			}
			buffer.size += result;
		}

		return buffer; // Don't use std::move, the compiler should use NRVO
	}

	// Send a message using serialization, assuming the struct T has a method
	// serialize.
	//
	// returns true if the message was sent correctly, otherwise false
	//
	template<typename T>
	inline static bool serialized_send(T& v, NET_StreamSocket* sock) {
		buff_t buffer;

		// serialize the message into the buffer, it returns
		// a pointer to the cell after the buffer. We assume
		// the caller knows that the buffer size if bufferSize_
		Uint8* end = v.serialize(buffer.data);
		buffer.size = end - buffer.data;
		assert(buffer.size <= buff_t::buffer_size);

		// send the buffer
		return send(sock, buffer.data, buffer.size);
	}

	// Receive a message that was sent serializedSend (or send) using deserialization,
	// assuming the struct T has a deserialize method.
	//
	// returns true if the message was received correctly, otherwise false
	//
	template<typename T>
	inline static bool deserialized_receive(T& v, NET_StreamSocket* sock) {
		buff_t buffer = receive(sock);
		if (buffer.size == 0)
			return false;
		else {
			v.deserialize(buffer.data);
			return true;
		}
	}

	/*
	 * Send/Receive messages for UDP.
	 *
	 */

	 // Send/Receive Datagrams directly, without serialization
	inline static bool send(NET_DatagramSocket* sock, Uint8* v,
		msg_size_t msgSize, NET_Address* addr, Uint16 port) {
		return NET_SendDatagram(sock, addr, port, v, msgSize);
	}

	// we return a shared pointer, so when the packer is not used
	// anymore we can destroy it.
	using packet_t = std::shared_ptr<NET_Datagram>;
	inline static packet_t receive(NET_DatagramSocket* sock) {
		NET_Datagram* packet = nullptr;
		if (NET_ReceiveDatagram(sock, &packet) && packet != nullptr) {
			return packet_t(packet, [](NET_Datagram* p) {
				NET_DestroyDatagram(p);
				});
		}
		else {
			return nullptr;
		}
	}

	// Send/Receive messages for UDP for types that implement
	// serialize and deserialize
	template<typename T>
	inline static bool serialized_send(T& m, NET_DatagramSocket* sock,
		NET_Address* addr, Uint16 port) {
		buff_t buffer;
		Uint8* end = m.serialize(buffer.data);
		buffer.size = end - buffer.data;
		assert(buffer.size <= buff_t::buffer_size);
		return NET_SendDatagram(sock, addr, port, buffer.data, buffer.size);
	}

	template<typename T>
	inline static int deserialized_receive(T& m, NET_DatagramSocket* sock,
		NET_Address** addr, Uint16* port) {
		NET_Datagram* packet = nullptr;
		if (NET_ReceiveDatagram(sock, &packet) && packet != nullptr) {
			m.deserialize(packet->buf);

			if (addr != nullptr) {
				NET_RefAddress(packet->addr);
				*addr = packet->addr;
			}
			if (port != nullptr)
				*port = port;

			NET_DestroyDatagram(packet);
			return true;
		}
		else {
			return false;
		}
	}

private:

	// serializing an unsigned integer typed
	template<typename UnsignedT>
	inline static Uint8* serialize_uint(UnsignedT& v, Uint8* buf) {

		UnsignedT aux = sdlnet_hton(v);
		memcpy(buf, &aux, sizeof(UnsignedT));

		// THIS IS NOT SAFE DUE TO THE ALIGNMENT REQUIREMENTS OF reinterpret_cast
		//
		// static_assert(alignof(UnsignedT)<=alignof(Uint8));
		// *reinterpret_cast<UnsignedT*>(buf) = sdlnet_hton(v);
		return buf + sizeof(UnsignedT);
	}

	template<typename UnsignedT>
	inline static Uint8* deserialize_uint(UnsignedT& v, Uint8* buf) {

		UnsignedT aux;
		memcpy(&aux, buf, sizeof(UnsignedT));
		v = sdlnet_ntoh(aux);

		// THIS IS NOT SAFE DUE TO THE ALIGNMENT REQUIREMENTS OF reinterpret_cast
		//
		// static_assert(alignof(UnsignedT)<=alignof(Uint8));
		// v = sdlnet_ntoh(*reinterpret_cast<UnsignedT*>(buf));
		return buf + sizeof(UnsignedT);
	}

	/** signed integer **/

	// if both use two's complement, just treat it as the corresponding unsigned
	// type
	template<typename UnsignedT, typename SignedT>
	inline static Uint8* serialize_sint(SignedT& v, Uint8* buf) {
		return serialize_uint<UnsignedT>(safe_reinterpret_cast<UnsignedT>(v), buf);
	}

	template<typename UnsignedT, typename SignedT>
	inline static Uint8* deserialize_sint(SignedT& v, Uint8* buf) {
		return deserialize_uint<UnsignedT>(safe_reinterpret_cast<UnsignedT>(v), buf);
	}

	// if there is a risk that the different sides use different representations for signed
	// numbers, then better transmit the sign separately
	//
//	template<typename UnsignedT, typename SignedT>
//	inline static Uint8* serialize_sint(SignedT &v, Uint8 *buf) {
//		static_assert( sizeof(UnsignedT) == sizeof(SignedT), "Signed and unsigned types do not match" );
//		Uint8 sign;
//		UnsignedT abs_v;
//
//		if (v >= 0) {
//			abs_v = v;
//			sign = 0;
//		} else {
//			abs_v = -v;
//			sign = 1;
//		}
//
//		UnsignedT abs_nv = sdlnet_hton(abs_v);
//		*reinterpret_cast<Uint8*>(buf) = sign;
//		*reinterpret_cast<UnsignedT*>(buf + 1) = abs_nv;
//
//		return buf + sizeof(UnsignedT) + 1;
//	}
//
//	template<typename UnsignedT, typename SignedT>
//	inline static Uint8* deserialize_sint(SignedT &v, Uint8 *buf) {
//		static_assert( sizeof(UnsignedT) == sizeof(SignedT), "Signed and unsigned types do not match" );
//		Uint8 sign = *reinterpret_cast<Uint8*>(buf);
//		UnsignedT abs_nv = *reinterpret_cast<UnsignedT*>(buf + 1);
//		UnsignedT abs_v = sdlnet_ntoh(abs_nv);
//
//		if (sign == 0) {
//			v = abs_v;
//		} else {
//			v = -static_cast<SignedT>(abs_v);
//		}
//
//		return buf + sizeof(UnsignedT) + 1;
//	}

	// serializing array of bytes
	inline static Uint8* serialize_arraybyte(Uint8* v, std::size_t N,
		Uint8* buf) {
		memcpy(buf, v, N);
		return buf + N;
	}

	inline static Uint8* deserialize_arraybyte(Uint8* v, std::size_t N,
		Uint8* buf) {
		memcpy(v, buf, N);
		return buf + N;
	}

};


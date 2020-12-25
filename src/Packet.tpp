#include <type_traits>
#include <array>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <optional>
#include <assert.h>
#include <iostream>

#include "misc/varints.hpp"
#include "misc/BufferHelper.hpp"
#include "EngineSocket.hpp"

namespace pkt {
#define BUFFER_ERROR std::length_error(std::string(__PRETTY_FUNCTION__) + ":" + std::to_string(__LINE__))

template<typename... Types>
struct are_all_arithmetic;

template<>
struct are_all_arithmetic<> : std::true_type {};

template<typename T, typename... Types>
struct are_all_arithmetic<T, Types...> : std::integral_constant<bool,
	std::is_arithmetic<T>::value &&
	are_all_arithmetic<Types...>::value> {};

// Helper to determine whether there's a const_iterator for T.
template<typename T>
struct has_const_iterator {
private:
    template<typename C> static char test(typename C::const_iterator*);
    template<typename C> static int  test(...);

public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

template<typename>
struct is_std_array : std::false_type {};

template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template<typename>
struct is_tuple : std::false_type {};

template<typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type {};

template<typename>
struct is_optional : std::false_type {};

template<typename T>
struct is_optional<std::optional<T>> : std::true_type {};

//////////////////////////////
// Forward declarations
//////////////////////////////

template<typename... Ts>
sz_t getSize(const std::tuple<Ts...>& t);

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& !has_const_iterator<typename Container::value_type>::value
	&& std::is_arithmetic<typename Container::value_type>::value,
	sz_t>::type
getSize(const Container& c);

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& !has_const_iterator<typename Container::value_type>::value
	&& is_tuple<typename Container::value_type>::value,
	sz_t>::type
getSize(const Container& c);

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& has_const_iterator<typename Container::value_type>::value,
	sz_t>::type
getSize(const Container& c);

template<typename T, std::size_t N>
sz_t getSize(const std::array<T, N>& arr);

template<typename T>
sz_t getSize(const std::optional<T>& opt);

template<typename N>
typename std::enable_if<std::is_arithmetic<N>::value,
	sz_t>::type
writeToBuf(u8 ** b, const N& num, sz_t remaining);

template<typename... Ts>
sz_t writeToBuf(u8 ** b, const std::tuple<Ts...>& t, sz_t remaining);

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value,
	sz_t>::type
writeToBuf(u8 ** b, const Container& c, sz_t remaining);

template<typename T, std::size_t N>
sz_t writeToBuf(u8 ** b, const std::array<T, N>& arr, sz_t remaining);

template<typename T>
sz_t writeToBuf(u8 ** b, const std::optional<T>& opt, sz_t remaining);

template<typename N>
typename std::enable_if<std::is_arithmetic<N>::value,
	N>::type
readFromBuf(u8 ** b, sz_t remaining);

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& std::is_arithmetic<typename Container::value_type>::value,
	Container>::type
readFromBuf(u8 ** b, sz_t remaining);

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& !std::is_arithmetic<typename Container::value_type>::value,
	Container>::type
readFromBuf(u8 ** b, sz_t remaining);

template<class Tuple>
typename std::enable_if<is_tuple<Tuple>::value,
	Tuple>::type
readFromBuf(u8 ** b, sz_t remaining);

template<class Array>
typename std::enable_if<is_std_array<Array>::value,
	Array>::type
readFromBuf(u8 ** b, sz_t remaining);

template<class OptionalValue>
typename std::enable_if<is_optional<OptionalValue>::value,
	OptionalValue>::type
readFromBuf(u8 ** b, sz_t remaining);

//////////////////////////////

template<typename T, std::size_t N>
constexpr T add(const std::array<T, N>& arr) {
	T result = 0;
	for (sz_t i = 0; i < N; i++) {
		result += arr[i];
	}

	return result;
}

template<typename>
struct is_tuple_arithmetic : std::false_type {};

template<typename... Ts>
struct is_tuple_arithmetic<std::tuple<Ts...>> : are_all_arithmetic<Ts...> {
	// member could be removed if value is false?
	static constexpr sz_t size = add<sz_t, sizeof... (Ts)>({sizeof(Ts)...});
};

template<typename N>
typename std::enable_if<std::is_arithmetic<N>::value,
	sz_t>::type
getSize(const N&) {
	return sizeof(N);
}

template<class Tuple, std::size_t... Is>
sz_t getTupleBufferSize(const Tuple& t, std::index_sequence<Is...>) {
	return add<sz_t, sizeof... (Is)>({getSize(std::get<Is>(t))...});
}

template<typename... Ts>
sz_t getSize(const std::tuple<Ts...>& t) {
	if constexpr (are_all_arithmetic<Ts...>::value) {
		constexpr sz_t s = add<sz_t, sizeof... (Ts)>({sizeof(Ts)...});
		return s;
	} else {
		return getTupleBufferSize(t, std::index_sequence_for<Ts...>{});
	}
}

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& !has_const_iterator<typename Container::value_type>::value // <- i don't think this is necessary
	&& std::is_arithmetic<typename Container::value_type>::value, // eg: no vector of strings
	sz_t>::type
getSize(const Container& c) {
	using T = typename Container::value_type;
	return c.size() * sizeof(T) + unsignedVarintSize(c.size());
}

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& !has_const_iterator<typename Container::value_type>::value
	&& is_tuple<typename Container::value_type>::value,
	sz_t>::type
getSize(const Container& c) {
	using T = typename Container::value_type;
	using Result = is_tuple_arithmetic<T>;
	
	if constexpr (Result::value) {
		return unsignedVarintSize(c.size()) + Result::size * c.size();
	}

	sz_t size = unsignedVarintSize(c.size());
	for (const T& v : c) {
		size += getSize(v);
	}

	return size;
}

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& has_const_iterator<typename Container::value_type>::value,
	sz_t>::type
getSize(const Container& c) {
	using T = typename Container::value_type;
	using Tv = typename T::value_type; // type of the inner array's content

	sz_t size = unsignedVarintSize(c.size());
	for (const T& v : c) {
		if (std::is_arithmetic<Tv>::value) {
			size += unsignedVarintSize(v.size()) + v.size() * sizeof(Tv);
		} else {
			size += getSize(v);
		}
	}

	return size;
}

template<typename T, std::size_t N>
sz_t getSize(const std::array<T, N>& arr) {
	if constexpr (std::is_arithmetic<T>::value) {
		return sizeof(T) * N;
	} else {
		sz_t size = 0;
		for (sz_t i = 0; i < N; i++) {
			size += getSize(arr[i]);
		}
		
		return size;
	}
}

template<typename T>
sz_t getSize(const std::optional<T>& opt) {
	return sizeof(bool) + (opt.has_value() ? getSize(*opt) : 0);
}

template<typename N> // double pointer!
typename std::enable_if<std::is_arithmetic<N>::value,
	sz_t>::type
writeToBuf(u8 ** b, const N& num, sz_t remaining) {
	assert(remaining >= sizeof(N));
	*b += buf::writeBE(*b, num);
	return sizeof(N);
}

template<class Tuple, std::size_t... Is>
sz_t tupleToBuf(u8 ** b, const Tuple& t, sz_t remaining, std::index_sequence<Is...>) {
	u8 * start = *b;
	u8 useless[] {(writeToBuf(b, std::get<Is>(t), remaining - (*b - start)), u8(0))...};
	return *b - start;
}

template<typename... Ts>
sz_t writeToBuf(u8 ** b, const std::tuple<Ts...>& t, sz_t remaining) {
	return tupleToBuf(b, t, remaining, std::index_sequence_for<Ts...>{});
}

template<typename T, std::size_t N>
sz_t writeToBuf(u8 ** b, const std::array<T, N>& arr, sz_t remaining) {
	return tupleToBuf(b, arr, remaining, std::make_index_sequence<N>{});
}

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value,
	sz_t>::type
writeToBuf(u8 ** b, const Container& c, sz_t remaining) {
	using T = typename Container::value_type;

	sz_t byteSize = unsignedVarintSize(c.size());

	assert(remaining >= byteSize);
	*b += encodeUnsignedVarint(*b, c.size());

	if (std::is_arithmetic<T>::value && sizeof(T) == 1) {
		byteSize += c.size() * sizeof(T);
		assert(remaining >= byteSize);
		*b = reinterpret_cast<u8 *>(
			std::copy(c.cbegin(), c.cend(), reinterpret_cast<T *>(*b))
		);
	} else {
		for (const T& v : c) {
			byteSize += writeToBuf(b, v, remaining - byteSize);
		}
	}

	return byteSize;
}

template<typename T>
sz_t writeToBuf(u8 ** b, const std::optional<T>& opt, sz_t remaining) {
	assert(remaining >= sizeof(bool));
	
	*b += buf::writeBE(*b, opt.has_value());
	remaining -= sizeof(bool);
	
	if (opt.has_value()) {
		return sizeof(bool) + writeToBuf(b, *opt, remaining);
	}
	
	return sizeof(bool);
}

template<typename N>
typename std::enable_if<std::is_arithmetic<N>::value,
	N>::type
readFromBuf(u8 ** b, sz_t remaining) {
	if (remaining < sizeof(N)) {
		throw BUFFER_ERROR;
	}

	u8 * readAt = *b;
	*b += sizeof(N);
	return buf::readBE<N>(readAt);
}

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& std::is_arithmetic<typename Container::value_type>::value,
	Container>::type // reads array of static-sized elements
readFromBuf(u8 ** b, sz_t remaining) {
	using T = typename Container::value_type;

	if (!remaining) {
		throw BUFFER_ERROR;
	}

	u8 * readAt = *b;
	sz_t decodedBytes;
	u64 size = decodeUnsignedVarint(readAt, decodedBytes, remaining);

	if (remaining - decodedBytes < size * sizeof(T)) {
		throw BUFFER_ERROR;
	}

	*b += decodedBytes + size * sizeof(T);
	readAt += decodedBytes;
	return Container(
		reinterpret_cast<T *>(readAt),
		reinterpret_cast<T *>(*b)
	); // guaranteed copy ellsion in c++17
}

template<typename Container>
typename std::enable_if<has_const_iterator<Container>::value
	&& !is_std_array<Container>::value
	&& !std::is_arithmetic<typename Container::value_type>::value,
	Container>::type // reads array of arrays
readFromBuf(u8 ** b, sz_t remaining) {
	using T = typename Container::value_type;

	if (!remaining) {
		throw BUFFER_ERROR;
	}

	sz_t decodedBytes;
	u64 size = decodeUnsignedVarint(*b, decodedBytes, remaining);
	
	if (remaining - decodedBytes < size) { /* size of the elements will be 1 at least */
		throw BUFFER_ERROR;
	}
	
	*b += decodedBytes;
	remaining -= decodedBytes;

	Container c;
	c.reserve(size); // XXX: could fill ram lol
	while (size-- > 0) {
		u8 * prev = *b;
		c.emplace_back(readFromBuf<T>(b, remaining));
		remaining -= *b - prev;
	}

	return c; // NRVO pls
}

// idk if this is optimal
template<class Tuple, std::size_t... Is>
Tuple tupleFromBuf(u8 ** b, sz_t remaining, std::index_sequence<Is...>) {
	u8 * start = *b;
	return Tuple{readFromBuf<typename std::tuple_element<Is, Tuple>::type>(b, remaining - (*b - start))...};
}

template<class Tuple>
typename std::enable_if<is_tuple<Tuple>::value,
	Tuple>::type
readFromBuf(u8 ** b, sz_t remaining) {
	return tupleFromBuf<Tuple>(b, remaining, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
}

template<class Array, std::size_t... Is>
Array staticArrayFromBuf(u8 ** b, std::index_sequence<Is...>) {
	using T = typename Array::value_type;
	
	u8 * start = *b;
	*b += sizeof(T) * sizeof... (Is);
	
	if constexpr (sizeof(T) == 1) {
		return Array{start[Is]...};
	} else {
		return Array{buf::readBE<T>(start + Is * sizeof(T))...};
	}
}

template<class Array>
typename std::enable_if<is_std_array<Array>::value,
	Array>::type
readFromBuf(u8 ** b, sz_t remaining) {
	using T = typename Array::value_type;
	constexpr sz_t size = std::tuple_size<Array>::value;
	
	if constexpr (std::is_arithmetic<T>::value) {
		if (remaining < sizeof(T) * size) {
			throw BUFFER_ERROR;
		}
		
		return staticArrayFromBuf<Array>(b, std::make_index_sequence<size>{});
	} else {
		// this works for arrays too, cool!
		return tupleFromBuf(b, remaining, std::make_index_sequence<size>{});
	}
}

template<class OptionalValue>
typename std::enable_if<is_optional<OptionalValue>::value,
	OptionalValue>::type
readFromBuf(u8 ** b, sz_t remaining) {
	using T = typename OptionalValue::value_type;
	
	bool isValuePresent = readFromBuf<bool>(b, remaining);
	remaining -= sizeof(bool);
	
	if (isValuePresent) {
		return readFromBuf<T>(b, remaining);
	}
	
	return std::nullopt;
}

#undef BUFFER_ERROR
} // namespace pkt

template<typename... Args>
std::tuple<Args...> fromBuffer(u8* buffer, sz_t size) {
	using namespace pkt;
	
	u8* start = buffer;

	// fast size check
	constexpr sz_t expectedSize = add<sz_t, sizeof... (Args)>({sizeof(Args)...});
	if (are_all_arithmetic<Args...>::value && expectedSize != size) {
		throw std::length_error("Buffer too small/big");
	}

	return std::tuple<Args...>{readFromBuf<Args>(&buffer, size - (buffer - start))...};
}

template<u8 opCode, typename... Args>
void toBufferAndSend(EngineSocket& s, Args... args) {
	using namespace pkt;
	constexpr bool isFixedSize = are_all_arithmetic<Args...>::value;
	
	const sz_t size = sizeof(u32) + sizeof(opCode) + (isFixedSize
		? add<sz_t, sizeof... (Args)>({sizeof(Args)...})
		: add<sz_t, sizeof... (Args)>({getSize(args)...}));

	u8 buffer[size];
	u8 * start = &buffer[0];
	u8 * to = start;

	to += buf::writeBE(to, u32(size - sizeof(u32)));
	*to++ = opCode;

	if constexpr (isFixedSize) {
		//u8 useless[] {((to += buf::writeBE(to, args)), u8(0))...};
		(buf::writeBE(to, args), ...);
	} else {
		//u8 useless[] {(writeToBuf(&to, args, size - (to - start)), u8(0))...};
		(writeToBuf(&to, args, size - (to - start)), ...);
		assert(sz_t(to - start) == size);
	}

	s.send(start, size);
}

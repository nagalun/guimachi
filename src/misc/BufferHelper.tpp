#include <cstring>

static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__,
	"Host endianness not supported!");

#define bswap_64(x) __builtin_bswap64(x)
#define bswap_32(x) __builtin_bswap32(x)
#define bswap_16(x) __builtin_bswap16(x)

namespace buf {
	template <typename Number>
	std::size_t writeLE(std::uint8_t * const buf, Number integer) noexcept {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		switch (sizeof(Number)) {
		case 2:
			integer = bswap_16(integer);
			break;

		case 4:
			integer = bswap_32(integer);
			break;

		case 8:
			integer = bswap_64(integer);
			break;
		}
#endif
		std::memcpy(buf, &integer, sizeof(Number));
		return sizeof(Number);
	}

	template <typename Number>
	std::size_t writeBE(std::uint8_t * const buf, Number integer) noexcept {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		switch (sizeof(Number)) {
		case 2:
			integer = bswap_16(integer);
			break;

		case 4:
			integer = bswap_32(integer);
			break;

		case 8:
			integer = bswap_64(integer);
			break;
		}
#endif
		std::memcpy(buf, &integer, sizeof(Number));
		return sizeof(Number);
	}


	template <typename Number>
	Number readLE(std::uint8_t * const buf) noexcept {
		Number finalBytes;
		std::memcpy(&finalBytes, buf, sizeof(Number));
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		switch (sizeof(Number)) {
		case 2:
			finalBytes = bswap_16(finalBytes);
			break;

		case 4:
			finalBytes = bswap_32(finalBytes);
			break;

		case 8:
			finalBytes = bswap_64(finalBytes);
			break;
		}
#endif
		return finalBytes;
	}

	template <typename Number>
	Number readBE(std::uint8_t * const buf) noexcept {
		Number finalBytes;
		std::memcpy(&finalBytes, buf, sizeof(Number));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		switch (sizeof(Number)) {
		case 2:
			finalBytes = bswap_16(finalBytes);
			break;

		case 4:
			finalBytes = bswap_32(finalBytes);
			break;

		case 8:
			finalBytes = bswap_64(finalBytes);
			break;
		}
#endif
		return finalBytes;
	}
};

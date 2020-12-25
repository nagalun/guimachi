#pragma once

#include <tuple>

#include "misc/explints.hpp"

class EngineSocket;

// maybe could be changed to std::optional to avoid exceptions
// NOTE: doesn't read opcode!
template<typename... Args>
std::tuple<Args...> fromBuffer(u8* buffer, sz_t size);

// send to a single socket
template<u8 opCode, typename... Args>
void toBufferAndSend(EngineSocket&, Args... args);

template <typename F>
struct fromBufFromLambdaArgs : public fromBufFromLambdaArgs<decltype(&F::operator())> {};

template <typename ClassType, typename ReturnType, typename... Args>
struct fromBufFromLambdaArgs<ReturnType(ClassType::*)(Args...) const> {
	static std::tuple<Args...> call(u8 * d, sz_t s) {
		return fromBuffer<Args...>(d, s);
	}
};

#include "Packet.tpp"
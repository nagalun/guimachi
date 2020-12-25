#pragma once

#include <ostream>
#include <QString>

enum class EngineState {
	WAITING, // ui is retrying connection to engine in a bit
	WAITINGNOPERMS, // ui doesn't have permission to connect, retrying in a bit
	QUERYING, // ui is trying to talk with the engine
	UNAVAILABLE, // network unavailable, ask user if they want to auto reconnect
	DISCONNECTING,
	DISCONNECTED,
	RECONNECTING,
	RESOLVING,
	CONNECTING,
	CONNECTED,
	ENROLLING,
	ENROLLED,
	AUTHENTIFYING,
	AUTHENTIFIED,
	PROBING,
	PROBED,
	SYNCHRONIZING,
	SYNCHRONIZED,
	READY,
	UNKNOWN,
	UNKNOWNOK
};

const char * toString(const EngineState state);
QString toQString(const EngineState state);
std::ostream& operator<<(std::ostream &o, const EngineState state);

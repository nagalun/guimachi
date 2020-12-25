#include "EngineState.hpp"

#include <QCoreApplication>

const char * toString(const EngineState state) {
	using ES = EngineState;
	switch (state) {
		case ES::WAITING:
			return QT_TRANSLATE_NOOP("EngineState", "Waiting for service...");

		case ES::WAITINGNOPERMS:
			return QT_TRANSLATE_NOOP("EngineState", "Permission denied! Retrying...");
			
		case ES::QUERYING:
			return QT_TRANSLATE_NOOP("EngineState", "Querying service...");
			
		case ES::UNAVAILABLE:
			return QT_TRANSLATE_NOOP("EngineState", "Server unavailable");

		case ES::DISCONNECTING:
			return QT_TRANSLATE_NOOP("EngineState", "Disconnecting...");
			
		case ES::DISCONNECTED:
			return QT_TRANSLATE_NOOP("EngineState", "Disconnected");
			
		case ES::RECONNECTING:
			return QT_TRANSLATE_NOOP("EngineState", "Reconnecting...");

		case ES::RESOLVING:
			return QT_TRANSLATE_NOOP("EngineState", "Resolving...");
			
		case ES::CONNECTING:
			return QT_TRANSLATE_NOOP("EngineState", "Connecting...");
			
		case ES::CONNECTED:
			return QT_TRANSLATE_NOOP("EngineState", "Connected");
			
		case ES::ENROLLING:
			return QT_TRANSLATE_NOOP("EngineState", "Enrolling...");
		
		case ES::AUTHENTIFYING:
			return QT_TRANSLATE_NOOP("EngineState", "Authentifying...");
			
		case ES::AUTHENTIFIED:
			return QT_TRANSLATE_NOOP("EngineState", "Authentified");
			
		case ES::ENROLLED:
			return QT_TRANSLATE_NOOP("EngineState", "Enrolled");
			
		case ES::PROBING:
			return QT_TRANSLATE_NOOP("EngineState", "Probing...");
			
		case ES::PROBED:
			return QT_TRANSLATE_NOOP("EngineState", "Probed");
			
		case ES::SYNCHRONIZING:
			return QT_TRANSLATE_NOOP("EngineState", "Synchronizing...");
			
		case ES::SYNCHRONIZED:
			return QT_TRANSLATE_NOOP("EngineState", "Synchronized");
			
		case ES::READY:
			return QT_TRANSLATE_NOOP("EngineState", "Ready");
		
		case ES::UNKNOWN:
			return QT_TRANSLATE_NOOP("EngineState", "Unknown...");
			
		case ES::UNKNOWNOK:
		default:
			return QT_TRANSLATE_NOOP("EngineState", "Unknown");
	}
}

QString toQString(const EngineState state) {
	return QCoreApplication::translate("EngineState", toString(state));
}

std::ostream& operator<<(std::ostream &o, const EngineState state) {
	return o << toString(state);
}

#include "Peer.hpp"
#include "opcodes.hpp"
#include "EngineInterface.hpp"

#include <QCoreApplication>
#include <iostream>

bool operator  <(std::reference_wrapper<Peer> a, std::reference_wrapper<Peer> b) {
	return a.get() < b.get();
}

bool operator ==(std::reference_wrapper<Peer> a, std::reference_wrapper<Peer> b) {
	return a.get() == b.get();
}

const Peer::Endpoint nullEndpoint;

Peer::Peer(EngineInterface &ei, str id)
: ei(ei),
  id(id),
  peerBits(0),
  authStatus(0),
  tunBits(0),
  tunAvailable(false),
  tunHasActivity(false),
  pendingMessage(false),
  lastTypingSentOn(std::chrono::system_clock::now()),
  extraActivityCb([] { }) { }

bool Peer::isChatEnabled() const {
	return peerBits & PEER::BITS::CHAT_ENABLED;
}

bool Peer::isIpv6Enabled() const {
	return peerBits & PEER::BITS::IPV6_ENABLED;
}

bool Peer::isIpv4Enabled() const {
	return !(peerBits & PEER::BITS::IPV4_DISABLED);
}

bool Peer::hasPendingMessage() const {
	return pendingMessage;
}

bool Peer::hasAliasSet() const {
	return aliasIpv4.toIPv4Address(); // i'm sure there's a better way, too lazy though
}

bool Peer::isTunnelReady() const {
	return tunAvailable;
}

bool Peer::isTunnelActive() const {
	return tunHasActivity;
}

bool Peer::isTunnelBad() const {
	// returns true if it's anything but blocked
	return tunBits & ~TUN_BITS::BLOCKED;
}


u32 Peer::getTunnelBits() const {
	return tunBits;
}

Peer::TunState Peer::getTunnelState() const {
	using namespace ENDPOINT::TYPE;
	using namespace PEER;

	if (tunBits & TUN_BITS::BLOCKED) { // If it isn't 0 we have a problem (?)
		return TunState::BLOCKED;
	}

	if (!isTunnelReady() && getAuthState() != AUTH::OK) { // Peer can be online but have no tunnel
		return TunState::OFFLINE;
	}

	switch (getCurrentEndpoint().getType()) {
		case DIRECT:
			return TunState::DIRECT;

		case RELAY:
			return TunState::RELAYED;

		case SERVER:
			return TunState::SERVER;

		default:
			return TunState::BLOCKED;
	}
}

u16 Peer::getAuthState() const {
	return authStatus;
}

u32 Peer::getPeerBits() const {
	return peerBits;
}

const std::string &Peer::getId() const {
	return id;
}

const std::string &Peer::getNick() const {
	return nick;
}

const QHostAddress& Peer::getRealAddress4() const {
	return tunIpv4;
}

const QHostAddress& Peer::getAddress4() const {
	return hasAliasSet() ? aliasIpv4 : tunIpv4;
}

const QHostAddress& Peer::getAddress6() const {
	return tunIpv6;
}

const Peer::Endpoint &Peer::getCurrentEndpoint() const {
	for (const auto &e : endpoints) {
		if (e.isCurrentlyUsed()) {
			return e;
		}
	}

	return nullEndpoint;
}

void Peer::clearPendingMessage() {
	pendingMessage = false;
}

void Peer::sendTyping() {
	auto now(std::chrono::system_clock::now());
	if (isTunnelReady() && std::chrono::duration_cast<std::chrono::seconds>(now - lastTypingSentOn) >= std::chrono::seconds(2)) {
		lastTypingSentOn = now;
		std::cout << "Sending typing to " << getNick() << std::endl;
		ei.imSendTyping(id);
	}
}

void Peer::resetTypingTimeout() {
	static const auto timerst(std::chrono::system_clock::now()); // sys_clock::time_point::min() doesn't seem to work well
	lastTypingSentOn = timerst;
}

std::shared_ptr<Promise<bool, u8>> Peer::sendMessage(std::string msg) {
	return ei.imSendMessage(false, id, std::move(msg));
}

bool Peer::operator==(const Peer &p) const {
	return id == p.id;
}

bool Peer::operator <(const Peer &p) const {
	return id < p.id;
}

const char * toString(const Peer::TunState state) {
	switch (state) {
		case Peer::TunState::OFFLINE:
			return QT_TRANSLATE_NOOP("TunState", "OFFLINE");
			
		case Peer::TunState::BLOCKED:
			return QT_TRANSLATE_NOOP("TunState", "BLOCKED");
			
		case Peer::TunState::SERVER:
			return QT_TRANSLATE_NOOP("TunState", "SERVER");
			
		case Peer::TunState::RELAYED:
			return QT_TRANSLATE_NOOP("TunState", "RELAYED");
			
		case Peer::TunState::DIRECT:
			return QT_TRANSLATE_NOOP("TunState", "DIRECT");
			
		default:
			return QT_TRANSLATE_NOOP("TunState", "UNKNOWN");
	}
}

QString toQString(const Peer::TunState state) {
	return QCoreApplication::translate("TunState", toString(state));
}

std::ostream& operator<<(std::ostream &o, const Peer::TunState state) {	
	return o << toString(state);
}


void Peer::update(str newnick, QHostAddress tun4, QHostAddress tun6, QHostAddress alias4, u32 pbits, u8 auth, u32 tbits, bool tavail) {
	nick = std::move(newnick);
	tunIpv4 = std::move(tun4);
	tunIpv6 = std::move(tun6);
	aliasIpv4 = std::move(alias4);
	peerBits = pbits;
	authStatus = auth;
	tunBits = tbits;
	tunAvailable = tavail;
}

void Peer::setEndpoints(std::vector<Endpoint> v) {
	endpoints = std::move(v);
}

void Peer::setExtraActivityCb(std::function<void()> f) {
	activityTimeout.disconnect(); // delete previous callbacks
	
	QObject::connect(&activityTimeout, &QTimer::timeout, &activityTimeout, [this] {
		tunHasActivity = false;
	});
	
	QObject::connect(&activityTimeout, &QTimer::timeout, &activityTimeout, std::move(f));
}

void Peer::setTunIsActive() {
	tunHasActivity = true;
	activityTimeout.start(1000);
}

void Peer::setPendingMessage() {
	pendingMessage = true;
}

Peer::Endpoint::Endpoint(u32 id, u8 type, u8 status, bool tcp, QHostAddress ip, u16 port, bool active)
: id(id),
  type(type),
  status(status),
  port(port),
  remoteIpv4(std::move(ip)),
  tcp(tcp),
  active(active) { }

Peer::Endpoint::Endpoint()
: id(-1),
  type(-1),
  status(-1),
  port(0),
  tcp(false),
  active(false) { }

u8 Peer::Endpoint::getType() const {
	return type;
}

bool Peer::Endpoint::isCurrentlyUsed() const {
	return isActive(); // TODO: i don't remember the difference here
}

bool Peer::Endpoint::isActive() const {
	return active;
}

bool Peer::Endpoint::isTcp() const {
	return tcp;
}

const QHostAddress &Peer::Endpoint::address() const {
	return remoteIpv4;
}

QString Peer::Endpoint::addressToString() const {
	return remoteIpv4.toString();
}

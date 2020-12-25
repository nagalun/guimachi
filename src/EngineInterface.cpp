#include "EngineInterface.hpp"

#include <iostream>
#include <unordered_map>
#include <functional>
#include <array>
#include <optional>
#include <fstream>
#include <stdexcept>
#include <tuple>

#include <QLocalSocket>
#include <QHostAddress>
#include <QLocale>
#include <QTimer>

#include "misc/streamoverloads.hpp"
#include "Packet.hpp"
#include "Guimachi.hpp"
#include "Network.hpp"
#include "Peer.hpp"
#include "NetworkUser.hpp"
#include "NetworkModel.hpp"
#include "NetworkChatWindow.hpp"
#include "PeerChatWindow.hpp"

EngineInterface::EngineInterface()
: batchUpdate(false),
  selfBits(0),
  es(EngineState::WAITING) {
	hookSocketEvents();
	hookMessageHandlers();
}

void EngineInterface::run() {
	s.open();
}

const std::string& EngineInterface::getNick() const {
	return nick;
}

const QHostAddress& EngineInterface::getTunIpv4() const {
	return selfTunIpv4;
}

const QHostAddress& EngineInterface::getTunIpv6() const {
	return selfTunIpv6;
}

bool EngineInterface::isIpv6Enabled() const {
	return selfBits & PEER::BITS::IPV6_ENABLED;
}

bool EngineInterface::isIpv4Enabled() const {
	return !(selfBits & PEER::BITS::IPV4_DISABLED); 
}

bool EngineInterface::isConnected() const {
	return s.isOpened();
}

EngineState EngineInterface::getState() const {
	return es;
}

std::unordered_map<str, Network>& EngineInterface::getNetworkMap() {
	return networks;
}

Network * EngineInterface::getNetwork(const str& id) {
	auto search = networks.find(id);
	return search != networks.end() ? &search->second : nullptr;
}

Peer * EngineInterface::getPeer(const str& id) {
	auto search = peers.find(id);
	return search != peers.end() ? &search->second : nullptr;
}


void EngineInterface::engHello() {
	send<CL_OPCODES::ENGINE_HELLO>(u8(PROTOCOL_VERSION));
}

void EngineInterface::engGetInfo(str lang, u32 unk) {
	send<CL_OPCODES::ENGINE_GET_INFO>(lang, unk);
}

void EngineInterface::engGetState(u8 unk) {
	send<CL_OPCODES::GET_STATE>(unk);
}

void EngineInterface::engLogin(bool autoRetryConnection) {
	send<CL_OPCODES::ENGINE_LOGIN>(autoRetryConnection);
}

void EngineInterface::engLogoff() {
	send<CL_OPCODES::ENGINE_LOGOFF>();
}

void EngineInterface::engSetNick(str nick) {
	// TODO: nick limits?
	send<CL_OPCODES::SET_NICK>(nick);
}

void EngineInterface::netwList() {
	// used by the linux client
	send<CL_OPCODES::NETW_GET_LIST>();
}

void EngineInterface::netwManage(str netwId, u8 actionOpcode, u8 unk) {
	send<CL_OPCODES::NETW_MANAGE>(netwId, actionOpcode, unk);
}

void EngineInterface::netwConnect(str netwId) {
	netwManage(netwId, NETW::ACTION::CONNECT);
}

void EngineInterface::netwDisconnect(str netwId) {
	netwManage(netwId, NETW::ACTION::DISCONNECT);
}

void EngineInterface::netwLeave(str netwId) {
	netwManage(netwId, NETW::ACTION::LEAVE);
}

void EngineInterface::netwDelete(str netwId) {
	netwManage(netwId, NETW::ACTION::DELETE);
}

std::shared_ptr<Promise<bool, u8>> EngineInterface::imSendMessage(bool toNetwork, str destination, str message) {
	// TODO: is valid peer/net?
	auto t = imPromises.emplace(destination, std::make_shared<Promise<bool, u8>>());

	if (!t.second) {
		auto p = std::make_shared<Promise<bool, u8>>();
		p->reject(255); // already trying to send a message, wait until it's sent! TODO: enum
		return p;
	}

	send<CL_OPCODES::IM_SEND>(toNetwork, std::move(destination), std::move(message));
	return t.first->second;
}

void EngineInterface::imSendTyping(str destination) {
	// TODO: is valid peer & connected?
	send<CL_OPCODES::IM_TYPING>(destination);
}


void EngineInterface::hookSocketEvents() {
	s.onOpen([this] {
		std::cout << "EngineSocket opened" << std::endl;
		es = EngineState::QUERYING;
		emit engineStateChanged(es);
		engHello();
	});

	s.onClose([this] {
		std::cout << "Socket closed" << std::endl;
		//nm.clear();
		for (auto& i : peers) {
			emit peerDeleted(i.second);
		}
		
		for (auto& i : networks) {
			emit networkDeleted(i.second);
		}
		
		networks.clear();
		peers.clear();
	});

	s.onError([this](auto e) {
		using E = QLocalSocket::LocalSocketError;

		es = EngineState::WAITING;
		switch (e) {
			case E::ServerNotFoundError:
				std::cout << "Server not found, is the service running?" << std::endl;
				break;
				
			case E::SocketAccessError:
				es = EngineState::WAITINGNOPERMS;
				break;

			default:
				break;
		}
		
		emit engineStateChanged(es);

		QTimer::singleShot(5000, [this] {
			std::cout << "Timed out" << std::endl;
			if (s.isClosed()) {
				s.open();
			}
		});
	});

	s.onMessage([this](u8 * buf, sz_t size) {
		u16 opc = static_cast<u16>(buf[0]);

		if (!processMessage(buf[0], buf + 1, size - 1)) {
			std::cout << "Unhandled MSG [OC: " << opc << ", SZ: " << size - 1 << "]" << std::endl;
			std::ofstream("packet-" + std::to_string(opc) + "-" + std::to_string(size - 1) + ".bin", std::ofstream::trunc | std::ofstream::binary)
			.write(reinterpret_cast<char *>(buf) + 1, size - 1);
		}
	});
}

void EngineInterface::hookMessageHandlers() {
	using namespace SV_OPCODES;

	reg<ENGINE_HELLO>([this](u8 protoVer, u32 verMajor, u32 verMinor, u32 verRev, u32 verBuild, u32 clientId) {
		std::string systemLang(QLocale::system().name().toUtf8().constData(), 2);
		std::cout << "Service version: " << verMajor << "." << verMinor << "." << verRev << "." << verBuild << std::endl;
		std::cout << "Protocol version: " << u16(protoVer) << std::endl;
		std::cout << "IPC id: " << clientId << std::endl;
		std::cout << "Requesting lang: " << systemLang << std::endl;
		engGetInfo(systemLang, 0);
	});

	reg<ENGINE_INFO>([this](str selfPeerId, u32 ma, u32 mi, u32 rv, u32 bu, str lang, str dataPath, u8 unk, std::vector<u8> key, u32 unk2, u32 selfBits/*, std::array<u8, 7>, str unkAddress, u64, str gateway, str listUrl, u32, str, u16, str, u16 port, u64*/) {  /* TODO: data remaining */
		std::cout << "ENGINE_INFO--------------" << std::endl;
		std::cout << "Your ID: " << selfPeerId << std::endl;
		std::cout << "Language: " << lang << std::endl;
		std::cout << "Service data path: " << dataPath << std::endl;
		std::cout << "Unk byte before key: " << u16(unk) << std::endl;
		std::cout << "Client key size: " << key.size() << std::endl;
		std::cout << std::hex << unk2 << std::dec  << std::endl;
		std::cout << "Self bits: " << std::hex << selfBits << std::dec  << std::endl;
		std::cout << "-------------------------" << std::endl;
		//this->selfBits = selfBits;
		
		// ugly hack until i figure out more about the engine info packet
		if (es == EngineState::QUERYING) {
			es = networks.size() > 0 ? EngineState::READY : EngineState::DISCONNECTED;
			emit engineStateChanged(es);
		}
		/*if (es == EngineState::QUERYING) {
			engGetState(); // i don't think this actually just gets the state, not sure what it does
			
			QTimer::singleShot(300, [this] {
				if (es == EngineState::QUERYING) {
					std::cout << "getState timed out, assuming disconnected" << std::endl;
					es = EngineState::DISCONNECTED;
					onEngineStateChanged(es);
				}
			});
		}*/
	});

	reg<IM_SENT>([this](u8 a, bool sentToNetwork, str sentToPeerId) {
		std::cout << "Sent message to " << sentToPeerId << ", " << u16(a) << ", " << sentToNetwork << std::endl;

		auto search = imPromises.find(sentToPeerId);

		if (search != imPromises.end()) {
			search->second->resolve(true);
			imPromises.erase(search);
		}
	});

	reg<IM_LOCAL_QUEUE>([this](std::vector<std::tuple<u8, str, std::vector<std::tuple<str, u32, str>>>> queuedMessages) {
		for (const auto &t : queuedMessages) {
			std::cout << "(" << u16(std::get<0>(t)) << ") Queued messages from: " << std::get<1>(t) << std::endl;

			for (const auto &t2 : std::get<2>(t)) {
				std::cout << "[" << std::get<0>(t2) << "]: " << std::get<2>(t2) << std::endl;
			}
		}
	});

	reg<IM_CHATLOG>([this](u8 status, u8 unk, str peerId, u32 row, std::vector<std::tuple<char, u8, u32, str>> messages) {  /* TODO: untested */
		std::cout << "Chatlog for " << peerId << std::endl;

		for (const auto &t : messages) {
			std::cout << std::get<0>(t) << " " << std::get<3>(t) << std::endl;
		}
	});

	reg<UNK_U16>([this](u8 a, u8 b) {  /* offline chats? */
		std::cout << "UNK_U16: " << a << ", " << b << std::endl;
	});

	reg<PEER_KEY>([this](u8 a, str peerId, std::vector<u8> key, u32 timestamp, u8 trustStatus, u32 timestamp2) {
		std::cout << "Key received for " << peerId << ", trust: " << trustStatus << std::endl;
	});

	reg<KNOWN_PEERS>([this](u32 a, u8 b, std::vector<str> peers) {
		std::cout << "Known peers (" << a << ", " << b << "): ";

		for (const auto &peerId : peers) {
			std::cout << peerId << ", ";
		}

		std::cout << std::endl;
	});

	reg<IM_RECEIVED>([this](str peerId, bool isGroupMsg, str netId, u32 timestamp, str msg) {
		auto syst(std::chrono::system_clock::from_time_t(timestamp));
		if (isGroupMsg) {
			std::swap(peerId, netId);
			std::cout << "(" << netId << ") [" << peerId << "]: " << msg << std::endl;
			Network& n = networks.at(netId);
			n.setPendingMessage();
			emit receivedNetworkMessage(syst, n, peers.at(peerId), std::move(msg));
		} else {
			std::cout << "[" << peerId << "]: " << msg << std::endl;
			Peer& p = peers.at(peerId);
			p.setPendingMessage();
			emit receivedPeerMessage(syst, p, std::move(msg));
		}
	});

	reg<IM_TYPING>([this](str peerId) {
		std::cout << peerId << " is typing..." << std::endl;
		emit peerTyping(peers.at(peerId));
	});

	reg<SET_STATE>([this](
	                   u32 zero, u8 zero2,
	                   u8 state, u8 completed,
	                   u8 unk, u64 unk2, u32 unk3, u32 unk4, u32 unk5,
	                   str connectionAddress, u32 resolvedIpv4, u16 port,
	                   std::array<u8, 16> maybeResolvedIpv6,
	                   u32 unk6, u32 unk7, u16 unk8, u32 unkIpv4, u8 unk9,
	                   str selfNick, u32 selfTunIpv4, u32 selfTunMask,
	                   u32 selfTunIpv4_2, u32 selfTunMask_2,
	                   std::array<u8, 16> selfTunIpv6, u8 tunIpv6Prefix,
	                   u32 selfBits /*0x20129076*/, u32 unk11, u16 unk12, u16 unk13,
	                   str attachedMail,
	                   u32 danger1, u8 danger2, /* This array's size might not be a varint! */
	std::vector<std::tuple<str, u8, u8>> networks) { /* TODO: data remaining */
		nick = std::move(selfNick);
		this->selfBits = selfBits;
		this->selfTunIpv4.setAddress(selfTunIpv4);
		this->selfTunIpv6.setAddress(selfTunIpv6.data());
		using namespace ENGINE::STATE;
		using ES = EngineState;
		switch (state) {
			case UNAVAILABLE:
				es = ES::UNAVAILABLE;
				break;
			
			case DISCONNECTING:
				es = completed ? ES::DISCONNECTED : ES::DISCONNECTING;
				break;
				
			case RECONNECTING:
				es = ES::RECONNECTING;
				break;
				
			//case RESOLVING:
			case RESOLVING2:
				es = ES::RESOLVING;
				break;
				
			case CONNECTING:
			case CONNECTING2:
				es = completed ? ES::CONNECTED : ES::CONNECTING;
				break;
				
			case ENROLLING:
				es = completed ? ES::ENROLLED : ES::ENROLLING;
				break;
				
			case AUTHENTIFYING:
				es = completed ? ES::AUTHENTIFIED : ES::AUTHENTIFYING;
				break;
				
			case PROBING:
			case PROBING2:
				es = completed ? ES::PROBED : ES::PROBING;
				break;
				
			case SYNCHRONIZING:
				es = completed ? ES::SYNCHRONIZED : ES::SYNCHRONIZING;
				break;
				
			case READY:
				es = ES::READY;
				break;
				
			default:
				es = completed ? ES::UNKNOWNOK : ES::UNKNOWN;
				std::cout << (u16)state << ", " << (u16)completed << std::endl;
				break;
		}
		
		
		emit engineStateChanged(es);
		
		//std::cout << es << ", " << nick << ", " << this->selfTunIpv4 << "/" << this->selfTunIpv6 << ", bits: " << std::hex << selfBits << std::dec << std::endl;

		/*for (const auto& t : networks) {
			std::cout << std::get<0>(t) << ", ";
		}
		std::cout << std::endl;*/
	});

	reg<NETW_BATCH_UPD>([this](bool state) {
		batchUpdate = state;
		//std::cout << "batch upd" << state << std::endl;
	});

	reg<NETW_UPDATED>([this](
	                      str netId, str netName, str netOwner,
	                      u32 maxMembers, u32 unk, u8 unk2,
	                      str loginMsg, u32 bits, u16 unk3,
	                      u16 bits2, u32 unk4, u32 unk5, u16 unk6,
	                      bool connected, bool unk7, u32 unk8,
	str domain, str linkedAccount) {
		auto res = networks.try_emplace(netId, *this, netId);
		Network& n = res.first->second;
		n.update(std::move(netName), std::move(netOwner), maxMembers, std::move(loginMsg), std::move(domain), std::move(linkedAccount), bits & NETW::BITS::CAN_LEAVE, connected);
		
		std::cout << "Network: " << n.getName() << " (" << n.getId() << ") [" << n.getOwner() << "] " << n.getSize() << "/" << n.getCapacity() << " [" << (n.isConnected() ? "ON" : "OFF") << "], canLeave: " << n.canLeave() << std::endl;
		
		if (res.second) {
			emit networkCreated(n);
		} else {
			emit networkUpdated(n);
		}
	});

	reg<NETW_DELETED>([this](str netId) {
		auto search = networks.find(netId);
		std::cout << "Network deleted: " << search->second.getName() << std::endl;
		
		emit networkDeleted(search->second);
		networks.erase(search);
	});

	reg<PEER_UPDATED>([this](str peerId, str peerName, u32 tunState, u32 bits, u8 unk, str domain, u32 peerIpv4, u32 peerIpv4_2, std::array<u8, 16> peerIpv6, u8 ipv6Prefix, bool tunAvailable, u8 unk2, u32 peerIpv4_3, u8 unk3, u32 selfIpv4, std::array<u8, 5>, std::optional<u16>, u8 maybeAuthStatus, std::array<u8, 7>, u32, u32, u8, std::vector<std::tuple<u32, u8, u8, bool, u16, u16, u32, u16, bool>> endpoints, u32 aliasIpv4) {  /* TODO: split and identify unknown data */
		if (unk != 0) std::cout << "unk is " << u16(unk) << " in PEER_UPDATED" << std::endl;

		if (unk2 != 0) std::cout << "unk2 is " << u16(unk2) << " in PEER_UPDATED" << std::endl;

		if (unk3 != 1) std::cout << "unk3 is " << u16(unk3) << " in PEER_UPDATED" << std::endl;

		std::vector<Peer::Endpoint> ev;
		for (auto [id, type, status, isTcp, unk4, unk5, remoteIpv4, remotePort, active] : endpoints) {
			ev.emplace_back(id, type, status, isTcp, QHostAddress(remoteIpv4), remotePort, active);
			//std::cout << "-> " << std::hex << id << "|" << u16(type) << "|" << u16(status) << "|" << u16(isTcp) << "|" << unk4 << "|" << unk5 << "|" << remoteIpv4 << "|" << remotePort << "|" << active << std::dec << std::endl;
		}

		auto res = peers.try_emplace(peerId, *this, peerId);
		Peer& peer = res.first->second;
		peer.update(std::move(peerName),
			QHostAddress(peerIpv4), QHostAddress(peerIpv6.data()),
			QHostAddress(aliasIpv4), bits, maybeAuthStatus, tunState, tunAvailable);
		peer.setEndpoints(std::move(ev));
		
		if (res.second) {
			// i'm gonna assume the timer doesn't get called if the peer gets destroyed
			peer.setExtraActivityCb([this, &peer] {
				emit peerUpdated(peer); // TODO: only update DecorationRole
			});
			
			emit peerCreated(peer);
		} else {
			emit peerUpdated(peer);
		}
		
		std::cout << "Peer: " << peer.getNick() << " (" << peer.getAddress4() << "/" << peer.getAddress6() << "|" << peer.getCurrentEndpoint().address() << "), ready: " << peer.isTunnelReady() << ", auth: " << peer.getAuthState() << ", tunnel: " << peer.getTunnelState() << ", tunbits: " << std::hex << tunState << std::dec << std::endl;
		
		if (peerIpv4 != peerIpv4_2) {
			std::cout << peerIpv4 << "(peer4_1) != " << peerIpv4_2 << "(peer4_2)!" << std::endl;
		}
	});

	reg<PEER_DELETED>([this](str peerId) {
		auto search = peers.find(peerId);
		std::cout << "Peer deleted: " << search->second.getNick() << std::endl;
		for (auto& i : networks) {
			i.second.removePeer(search->second);
		}
		
		emit peerDeleted(search->second);
		peers.erase(search);
	});

	reg<PEER_JOIN_NETW>([this](str netId, str peerId, bool online, bool owner, bool admin, bool premium, bool suspended) {
		Network& net = networks.at(netId); // throws if non existant, which shouldn't happen.
		
		auto tup = net.updatePeer(peers.at(peerId), online, owner, admin, premium, suspended);
		
		const NetworkUser& nu = tup.second;
		
		if (tup.first) { // is new?
			emit networkUserCreated(net, nu);
		} else {
			emit networkUserUpdated(net, nu);
		}
		
		std::cout << "NetPeer: " << nu.getPeer().getNick() << "@" << net.getName() << ": on: " << nu.isOnline()
			<< ", owner: " << nu.isOwner() << ", admin: " << nu.isAdmin() << ", premium: " << nu.isPremium()
			<< ", susp.: " << nu.isSuspended() << std::endl;
	});

	reg<PEER_QUIT_NETW>([this](str netId, str peerId) {
		Network& net = networks.at(netId);
		Peer& peer = peers.at(peerId);
		
		emit networkUserDeleted(net, net.getPeer(peer));
		net.removePeer(peer);
		
		std::cout << "NetPeer: " << peer.getNick() << "@" << net.getName() << ": left network" << std::endl;
	});

	manualReg(TUN_ACTIVITY, [this](u8 * data, sz_t s) {
		u8 curr;

		while ((curr = data[0])) {
			sz_t size;
			std::string peerId(getVarintString(data, size));
			data += size;
			
			Peer& peer = peers.at(peerId);
			peer.setTunIsActive();
			emit peerUpdated(peer);
		}

		//std::cout << "act" << std::endl;
	});

	reg<TUN_DOMAIN>([this](str domName, u8 unk, u8 a, u8 b, u8 c, u32 ipv4, u32 mask, std::array<u8, 16> ipv6, u8 prefix) {  /* TODO: data remaining */
		//std::cout << domName << ": " << u16(unk) << ", " << u16(a) << ", " << u16(b) << ", " << u16(c) << std::endl;
	});
}

template<u8 opCode, typename... Args>
void EngineInterface::send(Args... args) {
	toBufferAndSend<opCode>(s, std::move(args)...);
}

bool EngineInterface::processMessage(u8 opcode, u8 *data, sz_t size) {
	auto s = handlers.find(opcode);

	if (s != handlers.end()) {
		s->second(data, size);
		return true;
	}

	return false;
}

void EngineInterface::manualReg(u8 opCode, std::function<void(u8 *, sz_t)> f) {
	handlers.emplace(opCode, std::move(f));
}

template<u8 opCode, class F>
void EngineInterface::reg(F f) {
	handlers.emplace(opCode, [f{std::move(f)}](u8 * data, sz_t size) {
		try {
			std::apply(f, fromBufFromLambdaArgs<F>::call(data, size));
		} catch (const std::length_error& e) {
			std::cerr << "opc where this failed: " << u16(opCode) << ", what(): " << e.what() << std::endl;
			std::ofstream("failing-packet-" + std::to_string(u16(opCode)) + "-" + std::to_string(size) + ".bin", std::ofstream::trunc | std::ofstream::binary)
			.write(reinterpret_cast<char *>(data), size);
			throw;
		}
	});
}



#include "EngineSocket.hpp"

#include <stdexcept>
#include <iostream>

#include <QString>

#ifndef __WIN32
#include <unistd.h>
#include <sys/types.h>
#define UID_NEEDED_ON_HANDSHAKE
#endif

#include "misc/BufferHelper.hpp"
#include "misc/explints.hpp"

static const char *sockPath =
#ifdef __WIN32
    R"(\\.\pipe\Hamachi2.Rpc)";
#else
    R"(/var/run/logmein-hamachi/ipc.sock)";
#endif

EngineSocket::EngineSocket()
: handshakePending(true),
  writeQueueCurrentPos(0) {
	writeQueueBuf.reserve(512);

	QObject::connect(&s, &QLocalSocket::connected, [this] {
		std::cout << "Socket connected, handshaking" << std::endl;

		if (!doSyncHandshake()) {
			onErrorHandler(QLocalSocket::SocketAccessError);
			close();
			return;
		}

		onOpenHandler();
	});

	QObject::connect(&s, &QLocalSocket::disconnected, [this] {
		std::cout << "Socket disconnected" << std::endl;
		handshakePending = true;
		onCloseHandler();
	});

	QObject::connect(&s, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error), [this](auto e) {
		std::cout << "Socket error " << (int)e << std::endl;
		onErrorHandler(e);
	});

	QObject::connect(&s, &QLocalSocket::readyRead, [this] {
		u64 amount = s.bytesAvailable();

		if (amount < sizeof(u32) || handshakePending) {
			return;
		}
		//std::cout << "Ready read " << amount << std::endl;

		do {
			u32 size;
			s.peek(reinterpret_cast<char *>(&size), sizeof(u32));
			size = __builtin_bswap32(size); // packet size is big endian

			amount -= sizeof(u32);

			if (amount < size) { // wait a bit longer if we didn't receive all data yet
				std::cout << amount << " < " << size << std::endl;
				break;
			}

			s.skip(sizeof(u32));
			readPacket(size);
			amount -= size;
		} while (amount > sizeof(u32));
	});

	QObject::connect(&s, &QLocalSocket::bytesWritten, [this](i64) {
		tryWriteQueue();
	});
}

bool EngineSocket::isClosed() const {
	return s.state() == QLocalSocket::LocalSocketState::UnconnectedState;
}

bool EngineSocket::isOpened() const {
	return s.state() == QLocalSocket::LocalSocketState::ConnectedState;
}


void EngineSocket::open() {
	s.setServerName(sockPath);
	s.connectToServer();
}

void EngineSocket::close() {
	s.close();
}

void EngineSocket::send(const u8 *buf, sz_t size) {
	writeQueueBuf.insert(writeQueueBuf.end(), buf, buf + size);
	tryWriteQueue();
}

void EngineSocket::onOpen(std::function<void()> f) {
	onOpenHandler = std::move(f);
}

void EngineSocket::onMessage(std::function<void(u8 *, sz_t size)> f) {
	onMessageHandler = std::move(f);
}

void EngineSocket::onError(std::function<void(QLocalSocket::LocalSocketError)> f) {
	onErrorHandler = std::move(f);
}

void EngineSocket::onClose(std::function<void()> f) {
	onCloseHandler = std::move(f);
}

void EngineSocket::tryWriteQueue() {
	sz_t qSize = writeQueueBuf.size();

	if (qSize == 0) {
		return;
	}

	u8 *data = writeQueueBuf.data() + writeQueueCurrentPos;
	i64 written = s.write(reinterpret_cast<char *>(data), qSize - writeQueueCurrentPos);

	if (written < 0) {
		std::cerr << "Error while writing to socket" << std::endl;
		close();
		return;
	}

	writeQueueCurrentPos += written;

	if (writeQueueCurrentPos == qSize) {
		writeQueueBuf.clear();
		writeQueueCurrentPos = 0;
	}
}

void EngineSocket::readPacket(u32 size) {
	if (size == 0) {
		return;
	}

	if (size > 1024 * 256) {
		std::cerr << "Unusually large packet, beware of stack overflows" << std::endl;
	}

	u8 pkt[size];

	if (s.read(reinterpret_cast<char *>(&pkt[0]), size) < 0) {
		throw std::runtime_error("Read failed on readPacket");
	}

	onMessageHandler(&pkt[0], size);
}

/* The initial handshake on linux is different,
 * you need to send a 0 and the current user's UID (or not... =])
 */
bool EngineSocket::doSyncHandshake() {
	handshakePending = true;

#ifdef UID_NEEDED_ON_HANDSHAKE
	u8 expectedReply = 0x01;
	u8 handshake[5] = {0};
	buf::writeLE<u32>(&handshake[1], getuid());
#else
	u8 expectedReply = 0x00;
	u8 handshake[1] = {0};
#endif

	sz_t w = s.write(reinterpret_cast<char *>(&handshake[0]), sizeof(handshake));

	if (w != sizeof(handshake) || !s.waitForBytesWritten(-1) || !s.waitForReadyRead(1000)) {
		std::cout << "Read/write timeout in handshake" << std::endl;
		return false;
	}

	u8 reply;
	s.read(reinterpret_cast<char *>(&reply), sizeof(u8));

	if (reply != expectedReply) {
		std::cout << "Handshake failed! Expected " << u16(expectedReply) << ", got " << u16(reply) << std::endl;
#ifdef UID_NEEDED_ON_HANDSHAKE
		std::cout << "Do you have permission to control the service?" << std::endl;
#endif
		return false;
	}

	handshakePending = false;
	return true;
}

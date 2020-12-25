#pragma once

#include <string>
#include <vector>
#include <functional>

#include <QLocalSocket>

#include "misc/explints.hpp"

class EngineSocket {
	QLocalSocket s;
	bool handshakePending;
	sz_t writeQueueCurrentPos; // the buffer is cleared when all of it is sent to avoid moving data
	std::vector<u8> writeQueueBuf;
	std::function<void()> onOpenHandler;
	std::function<void(u8*, sz_t size)> onMessageHandler;
	std::function<void(QLocalSocket::LocalSocketError)> onErrorHandler;
	std::function<void()> onCloseHandler;

public:
	EngineSocket();

	bool isClosed() const;
	bool isOpened() const;

	void open();
	void close();
	void send(const u8* buf, sz_t size);

	void onOpen(std::function<void()>);
	void onMessage(std::function<void(u8*, sz_t size)>);
	void onError(std::function<void(QLocalSocket::LocalSocketError)>);
	void onClose(std::function<void()>);

private:
	void tryWriteQueue();
	void readPacket(u32);
	bool doSyncHandshake();
};

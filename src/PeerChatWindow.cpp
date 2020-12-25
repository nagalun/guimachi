#include "PeerChatWindow.hpp"

#include <iostream>

#include "Peer.hpp"
#include "InputBox.hpp"

PeerChatWindow::PeerChatWindow(Peer * _peer)
: peer(_peer),
  isTyping(false) {
	connect(chatInput, &QPlainTextEdit::textChanged, [this] {
		if (peer) {
			peer->sendTyping();
		}
	});
	
	connect(chatInput, &InputBox::sendContents, [this] {
		sendMessage(chatInput->toPlainText().toUtf8().constData());
	});
	
	QObject::connect(&typingTimeout, &QTimer::timeout, &typingTimeout, [this] {
		isTyping = false;
		updateStatusLabel();
	});
	
	setWindowTitle(QString("%1 (%2)")
		.arg(peer->getNick().c_str(), peer->getId().c_str()));
}

void PeerChatWindow::updateStatusLabel() {
	if (isTyping) {
		statusLabel->setText(tr("Typing..."));
	} else {
		statusLabel->clear();
	}
}

void PeerChatWindow::peerTyping() {
	isTyping = true;
	updateStatusLabel();
	typingTimeout.start(2100);
}

void PeerChatWindow::receiveMessage(std::chrono::system_clock::time_point time, std::string msg) {
	isTyping = false;
	typingTimeout.stop();
	updateStatusLabel();
	ChatWindow::receiveMessage("← " + QString::fromStdString(msg), time);
}

void PeerChatWindow::sendMessage(std::string str) {
	if (!str.size() || !peer) {
		return;
	}
	
	chatInput->setReadOnly(true);
	std::cout << "Peer sending '" << str << "'" << std::endl;
	
	peer->sendMessage(std::move(str))->then([this] (bool) {
		std::cout << "Sent message to peer" << std::endl;
		ChatWindow::receiveMessage("→ " + chatInput->toPlainText());
		chatInput->setReadOnly(false);
		chatInput->clear();
		if (peer) {
			peer->resetTypingTimeout();
		}
		
	}, [this] (u8 errcode) {
		std::cout << "Couldn't send message to peer" << std::endl;
		chatInput->setReadOnly(false);
	});
}

void PeerChatWindow::setDestinationPtr(void * ptr) {
	peer = static_cast<Peer *>(ptr);
}

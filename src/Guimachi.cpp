#include "Guimachi.hpp"

#include <iostream>

#include "icons.hpp"
#include "ChatWindow.hpp"
#include "PeerChatWindow.hpp"
#include "NetworkChatWindow.hpp"
#include "misc/colors.hpp"

// TODO: fix peer/network new message flag not unsetting when opening chat from the tray

Guimachi::Guimachi(int& argc, char ** argv)
: QApplication(argc, argv),
  sets("nagalun", "guimachi"),
  tl(*this),
  nm(*this, this),
  aShowMainWindow("Show"),
  aConnect(loadSvgIconReplacingColor(":/art/power.svg", getTextColor()), tr("&Connect")),
  aPreferences(QIcon::fromTheme("preferences-system"), tr("&Settings")),
  aQuit(QIcon::fromTheme("application-exit"), tr("&Exit")),
  aAbout(QIcon::fromTheme("help-about"), tr("About &Guimachi")),
  aAboutQt(QIcon::fromTheme("help-about"), tr("About &Qt")),
  statusIconReady(loadSvgIconReplacingColor(":/art/icon.svg", "#2498d0")),
  statusIconNotice(loadSvgIconReplacingColor(":/art/icon.svg", "#d08c24")),
  statusIconOffline(loadSvgIconReplacingColor(":/art/icon.svg", "#bbbbbb")),
  trayMenu(),
  tray(statusIconOffline, this),
  mw(*this, nm) {
	QApplication::setApplicationDisplayName("Guimachi");
	QApplication::setWindowIcon(statusIconReady);
	QApplication::setQuitOnLastWindowClosed(false);
	
	aShowMainWindow.setCheckable(true);
	aConnect.setCheckable(true);
	
	configureThings();
	registerActions();
}

void Guimachi::configureThings() {
	trayMenu.addAction(&aShowMainWindow);
	trayMenu.addSeparator();
	trayMenu.addAction(&aConnect);
	trayMenu.addAction(&aPreferences);
	trayMenu.addSeparator();
	trayMenu.addAction(&aQuit);
	
	tray.setContextMenu(&trayMenu);
	
	aConnect.setEnabled(false);
	
	connect(&ei, &EngineInterface::engineStateChanged, [this] (EngineState es) {
		std::cout << es << std::endl;
		if (!aConnect.isEnabled()) {
			aConnect.setEnabled(true);
		}
		
		switch (es) {
			case EngineState::SYNCHRONIZING:
				mw.setNetworkListSorting(false);
				break;
				
			case EngineState::DISCONNECTING:
				mw.setNetworkListSorting(false);
				[[fallthrough]];
			case EngineState::WAITING:
			case EngineState::WAITINGNOPERMS:
			case EngineState::QUERYING:
			case EngineState::UNAVAILABLE:
			case EngineState::UNKNOWN:
			case EngineState::UNKNOWNOK:
				aConnect.setEnabled(false);
				break;
				
			case EngineState::DISCONNECTED:
				if (aConnect.isChecked()) {
					aConnect.setChecked(false);
				}
				updateTray(es);
				nm.print();
				break;
				
			case EngineState::READY:
				mw.expandAllNetworks();
				mw.setNetworkListSorting(true);
				updateTray(es);
				[[fallthrough]];
			default:
				if (!aConnect.isChecked()) {
					aConnect.setChecked(true);
				}
				break;
		}
		
		tray.setToolTip(toQString(es));
		
		mw.updateStatusLabel();
	});
	
	connect(&ei, &EngineInterface::receivedPeerMessage, [this] (auto time, Peer& p, str msg) {
		PeerChatWindow * pcw = createOrGetChatWindow(p.getId(), p, false);
		QString qmsg(QString::fromStdString(msg)); // qstrings suck
		pcw->receiveMessage(time, std::move(msg));
		if (pcw->isHidden()) {
			nm.update(p, ChangeMode::MODIFY);
			updateTray(ei.getState());
			tray.showMessage(QString::fromStdString(p.getNick()), qmsg, statusIconReady);
		} else {
			p.clearPendingMessage();
		}
	});
	
	connect(&ei, &EngineInterface::receivedNetworkMessage, [this] (auto time, Network& n, Peer& p, str msg) {
		NetworkChatWindow * ncw = createOrGetChatWindow(n.getId(), n, false);
		QString qmsg("%1: %2");
		qmsg = qmsg.arg(QString::fromStdString(p.getNick()), QString::fromStdString(msg));
		ncw->receiveMessage(time, p, std::move(msg));
		if (ncw->isHidden()) {
			nm.update(n, ChangeMode::MODIFY);
			updateTray(ei.getState());
			tray.showMessage(QString::fromStdString(n.getName()), qmsg, statusIconReady);
		} else {
			n.clearPendingMessage();
		}
	});
	
	connect(&ei, &EngineInterface::peerCreated, [this] (Peer& p) {
		nm.update(p, ChangeMode::INSERT);
		PeerChatWindow * pcw = getChatWindow(p);
		if (pcw) {
			pcw->setDestinationPtr(std::addressof(p));
			pcw->receiveSystemMessage(tr("[%1] is available again.")
				.arg(QString::fromStdString(p.getNick())));
		}
	});
	
	connect(&ei, &EngineInterface::peerUpdated, [this] (Peer& p) {
		nm.update(p, ChangeMode::MODIFY);
	});
	
	connect(&ei, &EngineInterface::peerTyping, [this] (Peer& p) {
		PeerChatWindow * pcw = getChatWindow(p);
		if (pcw) {
			pcw->peerTyping();
		}
	});
	
	connect(&ei, &EngineInterface::peerDeleted, [this] (Peer& p) {
		nm.update(p, ChangeMode::DELETE);
		PeerChatWindow * pcw = getChatWindow(p);
		if (pcw) {
			pcw->setDestinationPtr(nullptr);
			pcw->receiveSystemMessage(tr("[%1] left all your networks.")
				.arg(QString::fromStdString(p.getNick())));
		}
	});
	
	connect(&ei, &EngineInterface::networkUserCreated, [this] (Network& n, const NetworkUser& nu) {
		nm.update(nu, &n, ChangeMode::INSERT);
		NetworkChatWindow * ncw = getChatWindow(n);
		if (ncw) {
			ncw->receiveSystemMessage(tr("[%1] joined the network.")
				.arg(QString::fromStdString(nu.getPeer().getNick())));
		}
	});
	
	connect(&ei, &EngineInterface::networkUserUpdated, [this] (Network& n, const NetworkUser& nu) {
		nm.update(nu, &n, ChangeMode::MODIFY);
	});
	
	connect(&ei, &EngineInterface::networkUserDeleted, [this] (Network& n, const NetworkUser& nu) {
		nm.update(nu, &n, ChangeMode::DELETE);
		NetworkChatWindow * ncw = getChatWindow(n);
		if (ncw) {
			ncw->receiveSystemMessage(tr("[%1] left the network.")
				.arg(QString::fromStdString(nu.getPeer().getNick())));
		}
	});
	
	connect(&ei, &EngineInterface::networkCreated, [this] (Network& n) {
		nm.update(n, ChangeMode::INSERT);
		NetworkChatWindow * ncw = getChatWindow(n);
		if (ncw) {
			ncw->setDestinationPtr(std::addressof(n));
			ncw->receiveSystemMessage(tr("You joined the network."));
		}
	});
	
	connect(&ei, &EngineInterface::networkUpdated, [this] (Network& n) {
		nm.update(n, ChangeMode::MODIFY);
	});
	
	connect(&ei, &EngineInterface::networkDeleted, [this] (Network& n) {
		nm.update(n, ChangeMode::DELETE);
		NetworkChatWindow * ncw = getChatWindow(n);
		if (ncw) {
			ncw->setDestinationPtr(nullptr);
			ncw->receiveSystemMessage(tr("You left the network."));
		}
	});
}

void Guimachi::registerActions() {
	QObject::connect(&tray, &QSystemTrayIcon::activated, [this] (auto reason) {
		if (reason == QSystemTrayIcon::Trigger) {
			if (!activateNextNotice()) {
				aShowMainWindow.trigger();
			}
		}
	});
	
	QObject::connect(&aShowMainWindow, &QAction::triggered, [this] (bool shown) {
		if (shown) {
			mw.show();
		} else {
			mw.close();
		}
	});
	
	QObject::connect(&aConnect, &QAction::triggered, [this] (bool state) {
		aConnect.setEnabled(false);
		if (state) {
			ei.engLogin();
		} else {
			ei.engLogoff();
		}
	});
	
	QObject::connect(&aPreferences, &QAction::triggered, [this] {
		// TODO
	});
	
	QObject::connect(&aQuit, &QAction::triggered, [this] {
		QApplication::closeAllWindows();
		QApplication::exit(0);
	});
	
	QObject::connect(&aAbout, &QAction::triggered, [this] {
		openAboutDialog();
	});
	
	QObject::connect(&aAboutQt, &QAction::triggered, [this] {
		QApplication::aboutQt();
	});
}

MainWindow& Guimachi::getMainWindow() {
	return mw;
}

EngineInterface& Guimachi::getEngineInterface() {
	return ei;
}

QSystemTrayIcon& Guimachi::getTrayIcon() {
	return tray;
}

QSettings& Guimachi::getSettings() {
	return sets;
}


PeerChatWindow * Guimachi::createOrGetChatWindow(const str& dest, Peer& p, bool show) {
	if (ChatWindow * cw = getChatWindow(dest)) {
		if (show) {
			if (p.hasPendingMessage()) {
				p.clearPendingMessage();
				nm.update(p, ChangeMode::MODIFY);
			}
			
			bringUpChatWindow(dest, cw);
		}
		
		return dynamic_cast<PeerChatWindow *>(cw);
	} else {
		return static_cast<PeerChatWindow *>(openChatWindow(dest, std::make_unique<PeerChatWindow>(&p), show));
	}
}

NetworkChatWindow * Guimachi::createOrGetChatWindow(const str& dest, Network& n, bool show) {
	if (ChatWindow * cw = getChatWindow(dest)) {
		if (show) {
			if (n.hasPendingMessage()) {
				n.clearPendingMessage();
				nm.update(n, ChangeMode::MODIFY);
			}
			
			bringUpChatWindow(dest, cw);
		}
		
		return dynamic_cast<NetworkChatWindow *>(cw);
	} else {
		return static_cast<NetworkChatWindow *>(openChatWindow(dest, std::make_unique<NetworkChatWindow>(&n), show));
	}
}

void Guimachi::bringUpChatWindow(const str& dest, ChatWindow * cw) {
	bool reposition = false;
	if (cw->isHidden()) {
		reposition = true;
	}
	
	Qt::WindowFlags flags = cw->windowFlags();
	cw->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
	
	cw->show();
	
	if (cw->isMinimized()) {
		cw->setWindowState(cw->windowState() & ~Qt::WindowMinimized);
	}
	
	cw->setWindowFlags(flags);
	
	cw->raise();
	cw->activateWindow();
	
	
	if (reposition) {
		QVariant pos = sets.value("chats/geometry-" + QString::fromStdString(dest));
		if (pos.isValid()) {
			cw->restoreGeometry(pos.toByteArray());
			cw->clearWasMoved();
		}
	}
	
	updateTray(ei.getState());
}

bool Guimachi::bringUpChatWindow(const str& dest) {
	auto search = activeChats.find(dest);
	if (search != activeChats.end()) {
		bringUpChatWindow(dest, search->second.get());
		return true;
	}
	
	return false;
}

ChatWindow *  Guimachi::openChatWindow(str destination, std::unique_ptr<ChatWindow> cw, bool show) {
	if (show) {
		bringUpChatWindow(destination, cw.get());
	}

	auto it = activeChats.insert_or_assign(std::move(destination), std::move(cw)).first;
	it->second->setCloseCb([this, it] (bool wasMoved) {
		if (wasMoved) {
			sets.setValue("chats/geometry-" + QString::fromStdString(it->first), it->second->saveGeometry());
			it->second->clearWasMoved();
		}
	});
	
	return it->second.get();
}

ChatWindow * Guimachi::getChatWindow(const str& dest) {
	auto search = activeChats.find(dest);
	return search != activeChats.end() ? search->second.get() : nullptr;
}

PeerChatWindow * Guimachi::getChatWindow(const Peer& p) {
	return dynamic_cast<PeerChatWindow *>(getChatWindow(p.getId()));
}

NetworkChatWindow * Guimachi::getChatWindow(const Network& n) {
	return dynamic_cast<NetworkChatWindow *>(getChatWindow(n.getId()));
}

bool Guimachi::activateNextNotice() {
	for (auto& chat : activeChats) {
		if (chat.second->isHiddenAndHasNewMessages()) {
			bringUpChatWindow(chat.first, chat.second.get());
			return true;
		}
	}
	
	return false;
}

bool Guimachi::hasPendingNotices() const {
	for (const auto& chat : activeChats) {
		if (chat.second->isHiddenAndHasNewMessages()) {
			return true;
		}
	}
	
	return false;
}

void Guimachi::updateTray(EngineState es) {
	if (hasPendingNotices()) {
		tray.setIcon(statusIconNotice);
		return;
	}
	
	switch (es) {
		case EngineState::READY:
			tray.setIcon(statusIconReady);
			break;
		
		case EngineState::DISCONNECTED:
			tray.setIcon(statusIconOffline);
			break;
			
		default:
			break;
	}
}

void Guimachi::openAboutDialog() {
	
}

int Guimachi::exec() {
	tray.setVisible(true);
	ei.run();
	//mw.show();
	return QApplication::exec();
}

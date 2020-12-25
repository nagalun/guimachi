#pragma once

#include <map>
#include <string>
#include <memory>
#include <QObject>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QSettings>
#include <QSystemTrayIcon>

#include "MainWindow.hpp"
#include "EngineState.hpp"
#include "EngineInterface.hpp"
#include "NetworkModel.hpp"
#include "TranslationLoader.hpp"
#include "ChatWindow.hpp"

class PeerChatWindow;
class NetworkChatWindow;

class Guimachi : public QApplication {
	Q_OBJECT
	
	QSettings sets;
	
	TranslationLoader tl;
	NetworkModel nm;
	EngineInterface ei;
	
	QAction aShowMainWindow;
	QAction aConnect;
	QAction aPreferences;
	QAction aQuit;
	QAction aAbout;
	QAction aAboutQt;

	QIcon statusIconReady; // could be auto-colored
	QIcon statusIconNotice; // if chat message pending - or for other attention-grabbing events
	QIcon statusIconOffline;
	QMenu trayMenu;
	QSystemTrayIcon tray;
	// destinationId -> ChatWindow
	std::map<std::string, std::unique_ptr<ChatWindow>> activeChats; // ChatWindow will extend qwidget
	MainWindow mw; // unattached from chat windows?
	
public:
	Guimachi(int& argc, char ** argv);
	
	void configureThings();
	void registerActions();
	
	MainWindow& getMainWindow();
	EngineInterface& getEngineInterface();
	QSystemTrayIcon& getTrayIcon();
	QSettings& getSettings();

	void bringUpChatWindow(const str& dest, ChatWindow *);
	bool bringUpChatWindow(const str& dest);
	ChatWindow * getChatWindow(const str& dest);
	PeerChatWindow * getChatWindow(const Peer&);
	NetworkChatWindow * getChatWindow(const Network&);
	PeerChatWindow * createOrGetChatWindow(const str& dest, Peer&, bool show = true);
	NetworkChatWindow * createOrGetChatWindow(const str& dest, Network&, bool show = true);
	ChatWindow * openChatWindow(str destination, std::unique_ptr<ChatWindow>, bool show = true);
	
	bool activateNextNotice();
	bool hasPendingNotices() const;
	void updateTray(EngineState);
	
	void openAboutDialog();
	int exec();
	
	friend MainWindow;
};

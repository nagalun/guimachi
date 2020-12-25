#pragma once

#include <memory>

#include <QTreeView>
#include <QMainWindow>
#include <QPushButton>
#include <QAction>
#include <QLabel>
#include <QMenu>

class Guimachi;
class QIcon;
class QCloseEvent;
class QToolBar;
class NetworkModel;

class MainWindow : public QMainWindow {
	Q_OBJECT
	
	Guimachi& app;
	NetworkModel& nm;
	
	QToolBar& status;
	QTreeView networkList;
	QIcon statusIcon;
	QLabel statusLabel;
	
	QAction aJoinNetwork;
	QAction aCreateNetwork;
	
	std::unique_ptr<QMenu> currentContextMenu;
	bool wasMoved;
	
public:
	MainWindow(Guimachi&, NetworkModel&);
	
	void show();
	void hide();
	void activate();
	
	void updateStatusLabel();
	
	void expandAllNetworks();
	void setNetworkListSorting(bool);
	
private:
	void populateMenuBar();
	void populateToolBar();
	void configureThings();
	void registerActions();
	
	void storeGeometry();
	void restoreGeometry();
	
	void changeEvent(QEvent *);
	void moveEvent(QMoveEvent *);
	void closeEvent(QCloseEvent *);
};

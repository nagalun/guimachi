#include "MainWindow.hpp"

#include <QString>
#include <QMenuBar>
#include <QToolBar>
#include <QCloseEvent>
#include <QMoveEvent>
#include <QIcon>
#include <QTimer>
//#include <QAbstractItemModelTester>
#include <QTreeWidgetItem>

#include "Guimachi.hpp"
#include "NetworkModel.hpp"
#include "EngineState.hpp"

MainWindow::MainWindow(Guimachi& app, NetworkModel& nm)
: QMainWindow(),
  app(app),
  nm(nm),
  status(*addToolBar(tr("Status"))),
  networkList(this),
  statusLabel(this),
  aJoinNetwork(QIcon::fromTheme("list-add"), tr("&Join a network..."), this),
  aCreateNetwork(QIcon::fromTheme("list-add"), tr("&Create a network..."), this),
  wasMoved(false) {
	configureThings();
	populateMenuBar();
	populateToolBar();
	updateStatusLabel();
	registerActions();
}

void MainWindow::show() {
	app.aShowMainWindow.setChecked(true);
	
	Qt::WindowFlags flags = windowFlags();
	setWindowFlags(flags | Qt::WindowStaysOnTopHint);
	

	QMainWindow::show();
	restoreGeometry();
	
	if (isMinimized()) {
		setWindowState(windowState() & ~Qt::WindowMinimized);
	}
	
	setWindowFlags(flags);
	activate();
}

void MainWindow::hide() {
	storeGeometry();
	QMainWindow::hide();
}

void MainWindow::activate() {
	showNormal();
	raise();
	activateWindow();
}

void MainWindow::updateStatusLabel() {
	QString newText;
	if (app.ei.isIpv4Enabled()) {
		newText += app.ei.getTunIpv4().toString();
		if (app.ei.isIpv6Enabled()) {
			newText += " / ";
		}
	}
	
	if (app.ei.isIpv6Enabled()) {
		newText += app.ei.getTunIpv6().toString();
	}
	
	if (!newText.isEmpty()) {
		newText += '\n';
	}
	
	EngineState es = app.ei.getState();
	if (es == EngineState::READY && app.ei.getNick().size() > 0) {
		newText += QString::fromStdString(app.ei.getNick());
	} else {
		newText += toQString(es);
	}
	
	statusLabel.setText(newText);
	
	//statusLabel.setMinimumSize(statusLabel.sizeHint());
	setMinimumWidth(status.sizeHint().width());
}

void MainWindow::expandAllNetworks() {
	networkList.expandAll();
}

void MainWindow::setNetworkListSorting(bool state) {
	networkList.setSortingEnabled(state);
}


void MainWindow::populateMenuBar() {
	QMenuBar& mb = *menuBar();
	
	QMenu& sysMenu = *mb.addMenu(tr("&System"));
	sysMenu.addAction(&app.aConnect);
	sysMenu.addAction(&app.aPreferences);
	sysMenu.addSeparator();
	sysMenu.addAction(&app.aQuit);
	
	QMenu& netMenu = *mb.addMenu(tr("&Network"));
	netMenu.addAction(&aJoinNetwork);
	netMenu.addAction(&aCreateNetwork);
	
	QMenu& aboutMenu = *mb.addMenu(tr("&About"));
	aboutMenu.addAction(&app.aAbout);
	aboutMenu.addAction(&app.aAboutQt);
}

void MainWindow::populateToolBar() {
	status.setFloatable(false);
	status.setMovable(false);
	
	status.addAction(&app.aConnect);
	status.addSeparator();
	status.addWidget(&statusLabel);
	
	status.setIconSize(QSize(32, 32));
	status.setFixedHeight(48);
}

void MainWindow::configureThings() {
	//new QAbstractItemModelTester(&nm, QAbstractItemModelTester::FailureReportingMode::Fatal, this);
	setWindowTitle("Guimachi");
	
	statusLabel.setTextInteractionFlags(Qt::TextSelectableByMouse);
	//statusLabel.setWordWrap(true);
	
	networkList.sortByColumn(0, Qt::SortOrder::AscendingOrder);
	networkList.setUniformRowHeights(true);
	networkList.setHeaderHidden(true);
	networkList.setModel(&nm);
	
	//setWindowIcon(app.statusIcon);
	setCentralWidget(&networkList);
	
	networkList.setContextMenuPolicy(Qt::CustomContextMenu);
	connect(&networkList, &QTreeWidget::customContextMenuRequested, [this] (const QPoint& p) {
		QModelIndex idx = networkList.indexAt(p);
		currentContextMenu.reset(nm.getContextMenu(idx));
		if (currentContextMenu) {
			QPoint ofsp(mapToGlobal(p));
			ofsp.setY(ofsp.y() + currentContextMenu->sizeHint().height());
			currentContextMenu->popup(ofsp);
		}
	});
}

void MainWindow::registerActions() {
	
}

void MainWindow::storeGeometry() {
	if (wasMoved) {
		app.getSettings().setValue("mw/geometry", saveGeometry());
		wasMoved = false;
	}
}

void MainWindow::restoreGeometry() {
	QVariant pos = app.getSettings().value("mw/geometry");
	if (pos.isValid()) {
		QWidget::restoreGeometry(pos.toByteArray());
		wasMoved = false;
	}
}

void MainWindow::changeEvent(QEvent* ev) {
	switch (ev->type()) {
		/*case QEvent::WindowStateChange:
			if (windowState() & Qt::WindowMinimized) {
				QTimer::singleShot(0, this, &MainWindow::hide);
			}
			break;*/
			
		default:
			QMainWindow::changeEvent(ev);
			break;
	}
}

void MainWindow::moveEvent(QMoveEvent *) {
	wasMoved = true;
}

void MainWindow::closeEvent(QCloseEvent* ev) {
	/*if (QSystemTrayIcon::isSystemTrayAvailable()) {
		hide();
		ev->ignore();
	} else {*/
	app.aShowMainWindow.setChecked(false);
	storeGeometry();
	QMainWindow::closeEvent(ev);
	//}
}

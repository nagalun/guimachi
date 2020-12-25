#include "NetworkModel.hpp"

#include <iterator>
#include <iostream>
#include <memory>
#include <algorithm>

#include <QFont>
#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QMenu>
#include <QClipboard>
#include <QList>

#include "Guimachi.hpp"
#include "Network.hpp"
#include "NetworkUser.hpp"
#include "Peer.hpp"
#include "icons.hpp"
#include "misc/colors.hpp"

class NetworkNode {
public:
	const void * item;
	NetworkNode * parent;
	std::vector<NetworkNode *> children;
	int row;
	bool isUser;
	
	NetworkNode(const void * item, bool isUser, NetworkNode * parent, int row)
	: item(item),
	  parent(parent),
	  row(row),
	  isUser(isUser) { }
	
	~NetworkNode() {
		for (NetworkNode * nn : children) {
			delete nn;
		}
	}
};

NetworkModel::NetworkModel(Guimachi& gm, QObject * parent)
: QAbstractItemModel(parent),
  gm(gm),
  tunReady(loadSvgIconReplacingColor(":/art/tun.svg", "#33bb33")),
  tunRelayed(loadSvgIconReplacingColor(":/art/tun.svg", "#888899")),
  tunServer(loadSvgIconReplacingColor(":/art/tun.svg", "#555599")),
  tunBlocked(loadSvgIconReplacingColor(":/art/tun.svg", "#bb3333")),
  tunOffline(loadSvgIconReplacingColor(":/art/tun.svg", "#dddddd")),
  tunReadyActivity(loadSvgIconReplacingColor(":/art/tun_activity.svg", "#33bb33")),
  tunRelayedActivity(loadSvgIconReplacingColor(":/art/tun_activity.svg", "#888899")),
  tunServerActivity(loadSvgIconReplacingColor(":/art/tun_activity.svg", "#555599")),
  tunProblem(loadSvgIconReplacingColor(":/art/alert.svg", getTextColor())),
  chatMessage(loadSvgIconReplacingColor(":/art/chaticon.svg", getTextColor())) { }

NetworkModel::~NetworkModel() {
	for (NetworkNode * nn : nodes) {
		delete nn;
	}
}

QModelIndex NetworkModel::index(int row, int column, const QModelIndex &parent) const {
	if (!parent.isValid()) {
		//std::cout << "access: " << row << ", " << column << std::endl;
		return row >= 0 && row < (int)nodes.size() ? createIndex(row, column, nodes[row]) : QModelIndex();
	}
	
	NetworkNode& nn = *static_cast<NetworkNode *>(parent.internalPointer());
	if (nn.isUser) { // users have no parents
		return QModelIndex();
	}
	
	//std::cout << "children size: " << nn.children.size() << ", access: " << row << std::endl;
	return createIndex(row, column, nn.children.at(row));
}

QModelIndex NetworkModel::parent(const QModelIndex &i) const {
	if (!i.isValid()) {
		return QModelIndex();
	}
	
	NetworkNode& nn = *static_cast<NetworkNode *>(i.internalPointer());
	if (!nn.parent) {
		return QModelIndex();
	}
	
	NetworkNode& pnn = *nn.parent;
	return createIndex(pnn.row, 0, &pnn);
}

int NetworkModel::rowCount(const QModelIndex &parent) const {
	if (!parent.isValid()) {
		return nodes.size();
	}
	
	NetworkNode& nn = *static_cast<NetworkNode *>(parent.internalPointer());
	if (nn.isUser) {
		return 0;
	}
	
	//const Network& n = *static_cast<const Network *>(nn.item);
	return nn.children.size();//.getPeerMap().size();
}

int NetworkModel::columnCount(const QModelIndex&) const {
	return 1;
}

QVariant NetworkModel::data(const QModelIndex &i, int role) const {
	if (!i.isValid()) {
		return QVariant();
	}
	
	NetworkNode& nn = *static_cast<NetworkNode *>(i.internalPointer());
	
	if (nn.isUser) {
		return displayData(*static_cast<const NetworkUser *>(nn.item), role);
	} else {
		return displayData(*static_cast<const Network *>(nn.item), role);
	}
}

void NetworkModel::update(const Peer& p, ChangeMode cm) {
	const Peer * pp = std::addressof(p);
	switch (cm) {
		case ChangeMode::DELETE: {
			auto search = peerLinks.find(pp);
			std::vector<NetworkNode *> pvec(search->second); // copy the vector since the update function deletes elements from it
			for (NetworkNode * nn : pvec) {
				update(*static_cast<const NetworkUser *>(nn->item), nullptr, ChangeMode::DELETE);
			}
			
			peerLinks.erase(search);
		} break;
		
		case ChangeMode::MODIFY: {
			const auto& v = peerLinks.at(pp);
			for (NetworkNode * nn : v) {
				QModelIndex idx(createIndex(nn->row, 0, nn));
				dataChanged(idx, idx); // inefficient, should only update specific roles
			}
		} break;
		
		case ChangeMode::INSERT: {
			peerLinks.try_emplace(pp); 
		} break;
	}
}

void NetworkModel::update(const Network& n, ChangeMode cm) {
	const Network * np = std::addressof(n);
	switch (cm) {
		case ChangeMode::DELETE: {
			n.forEach([this] (const NetworkUser& nu) {
				update(nu, nullptr, ChangeMode::DELETE);
			});
			
			auto search = links.find(np); // no check if exists, not needed anyways
			NetworkNode * nn = search->second;
			links.erase(search);
			
			beginRemoveRows(QModelIndex(), nn->row, nn->row);
			for (auto it = nodes.erase(nodes.begin() + nn->row); it != nodes.end(); it++) {
				(*it)->row--; // shift all following rows by 1
			}
			endRemoveRows();
			
			delete nn;
		} break;
		
		case ChangeMode::MODIFY: {
			NetworkNode * nn = links.at(np);
			QModelIndex idx(createIndex(nn->row, 0, nn));
			dataChanged(idx, idx); // inefficient
		} break;
		
		case ChangeMode::INSERT: {
			int i = nodes.size();
			beginInsertRows(QModelIndex(), i, i); // always at the end...
			NetworkNode * nn = nodes.emplace_back(new NetworkNode(np, false, nullptr, i));
			links.emplace(np, nn);
			endInsertRows();
		} break;
	}	
}

void NetworkModel::update(const NetworkUser& nu, const Network * np, ChangeMode cm) {
	const NetworkUser * nup = std::addressof(nu);
	const Peer * pp = std::addressof(nu.getPeer());
	switch (cm) {
		case ChangeMode::DELETE: {
			auto search = links.find(nup);
			NetworkNode * nn = search->second;
			links.erase(search);
			{
				auto& pvec = peerLinks.at(pp);
				auto search = std::find(pvec.begin(), pvec.end(), nn);
				if (search != pvec.end()) {
					pvec.erase(search);
				}
			}
			
			beginRemoveRows(createIndex(nn->parent->row, 0, nn->parent), nn->row, nn->row);
			auto& cvec = nn->parent->children;
			for (auto it = cvec.erase(cvec.begin() + nn->row); it != cvec.end(); it++) {
				(*it)->row--; // shift all following rows by 1
			}
			endRemoveRows();
			delete nn;
		} break;
		
		case ChangeMode::MODIFY: {
			NetworkNode * nn = links.at(nup);
			QModelIndex idx(createIndex(nn->row, 0, nn));
			dataChanged(idx, idx); // inefficient
		} break;
		
		case ChangeMode::INSERT: {
			auto search = links.find(np);
			NetworkNode * netwnn = search->second;
			
			int i = netwnn->children.size();
			beginInsertRows(createIndex(netwnn->row, 0, netwnn), i, i); // always at the end...
			NetworkNode * nn = netwnn->children.emplace_back(new NetworkNode(nup, true, netwnn, i));
			links.emplace(nup, nn);
			peerLinks.at(pp).emplace_back(nn);
			endInsertRows();
		} break;
	}
}

void NetworkModel::clear() {
	beginResetModel();
	for (NetworkNode * nn : nodes) {
		delete nn;
	}
	nodes.clear();
	links.clear();
	peerLinks.clear();
	endResetModel();
}

void NetworkModel::print() {
	std::cout << nodes.size() << ", " << links.size() << ", " << peerLinks.size() << std::endl;
}


/*void NetworkModel::populateModel() {
	beginResetModel();
	nodes.clear();
	int i = 0;
	for (const auto& n : networkMap) {
		NetworkNode * nn = nodes.emplace_back(new NetworkNode(std::addressof(n.second), false, nullptr, i++));
		int j = 0;
		for (const auto& m : n.second.getPeerMap()) {
			nn->children.emplace_back(new NetworkNode(std::addressof(m.second), true, nn, j++));
		}
	}
	
	endResetModel();
}*/


QMenu * NetworkModel::getContextMenu(const QModelIndex& i) const {
	return data(i, CustomItemDataRole::ContextMenuRole).value<QMenu *>();
	//if (!m.isValid())
}

QVariant NetworkModel::displayData(const Network& n, int role) const {
	switch (role) {
		case Qt::DisplayRole: {
			return QString::fromStdString(n.getName());
		};
		
		case Qt::FontRole: {
			QFont qf;
			qf.setBold(n.isConnected());
			return qf;
		};
		
		case Qt::DecorationRole:
			if (n.hasPendingMessage()) {
				return chatMessage;
			}
			
			return n.isConnected() ? tunReady : tunOffline;
		
		case Qt::ToolTipRole: {
			return tr("ID: %1\nOwner: %2\nCapacity: %3/%4")
				.arg(QString::fromStdString(n.getId()))
				.arg(QString::fromStdString(n.getOwner()))
				.arg(n.getSize())
				.arg(n.getCapacity());
		};
		
		case CustomItemDataRole::ContextMenuRole: {
			QVariant qv;
			QMenu * menu = new QMenu;
			qv.setValue(menu);
			
			if (n.isConnected()) {
				menu->addAction(tr("Disconn&ect"), [&n] {
					n.disconnect();
				});
			} else {
				menu->addAction(tr("Conn&ect"), [&n] {
					n.connect();
				});
			}
			
			menu->addAction(tr("Network &Chat"), [this, &n] {
				// please
				gm.createOrGetChatWindow(n.getId(), const_cast<Network&>(n));
			});
			
			menu->addSeparator();

			menu->addAction(tr("Copy &ID"), [&n] {
				QApplication::clipboard()->setText(QString::fromStdString(n.getId()));
			});
			
			if (n.canLeave()) {
				menu->addAction(tr("&Leave network"), [&n] {
					n.leave();
				});
			}
			
			return qv;
		};
	}
	
	return QVariant();
}

QVariant NetworkModel::displayData(const NetworkUser& nu, int role) const {
	using TS = Peer::TunState;
	Peer& p = nu.getPeer();

	switch (role) {
		case Qt::DisplayRole: {
			QString str(QString::fromStdString(p.getNick()));
			
			if (p.getTunnelState() != Peer::TunState::OFFLINE) {
				str += " - ";
				if (p.isIpv4Enabled()) {
					str += p.getAddress4().toString();
					if (p.isIpv6Enabled()) {
						str += " / ";
					}
				}
				
				if (p.isIpv6Enabled()) {
					str += p.getAddress6().toString();
				}
			}
			
			return str;
		};
		
		case Qt::DecorationRole:
			if (p.hasPendingMessage()) {
				return chatMessage;
			}
			
			switch (p.getTunnelState()) {
				case TS::OFFLINE: return tunOffline;
				case TS::BLOCKED: return tunBlocked;
				case TS::SERVER:  return p.isTunnelActive() ? tunServerActivity : tunServer;
				case TS::RELAYED: return p.isTunnelActive() ? tunRelayedActivity : tunRelayed;
				case TS::DIRECT:  return p.isTunnelActive() ? tunReadyActivity : tunReady;
			}
			break;
		
		case Qt::ForegroundRole:
			if (!nu.isOnline()) {
				QColor tc(QApplication::palette().text().color());
				tc.setAlpha(127);
				return tc;
			}
			break;
		
		case Qt::ToolTipRole: {
			return tr("ID: %1\nReady: %2 (%3)\nTunnel Bits: %4")
				.arg(QString::fromStdString(p.getId()))
				.arg(p.isTunnelReady() ? tr("Yes") : tr("No"))
				.arg(toQString(p.getTunnelState()))
				.arg(QString::number(p.getTunnelBits(), 16).toUpper());
		};

		case CustomItemDataRole::ContextMenuRole: {
			QVariant qv;
			QMenu * menu = new QMenu;
			qv.setValue(menu);
			
			menu->addAction(tr("&Chat"), [this, &p] {
				gm.createOrGetChatWindow(p.getId(), p);
			});
			
			menu->addSeparator();
			
			if (p.isIpv4Enabled()) {
				menu->addAction(tr("Copy IPv&4 address"), [&p] {
					QApplication::clipboard()->setText(p.getAddress4().toString());
				});
			}

			if (p.isIpv6Enabled()) {
				menu->addAction(tr("Copy IPv&6 address"), [&p] {
					QApplication::clipboard()->setText(p.getAddress6().toString());
				});
			}
			
			menu->addAction(tr("Copy &ID"), [&p] {
				QApplication::clipboard()->setText(QString::fromStdString(p.getId()));
			});
			
			return qv;
		};
	}
	
	return QVariant();
}

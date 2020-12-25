#pragma once

#include "Network.hpp"
#include "types.hpp"

#include <unordered_map>
#include <vector>

#include <QAbstractItemModel>
#include <QObject>
#include <QModelIndex>
#include <QVariant>
#include <QIcon>

class NetworkNode;
class NetworkUser;
class QMenu;
class Guimachi;

enum class ChangeMode {
	DELETE,
	MODIFY,
	INSERT
};

enum CustomItemDataRole {
	ContextMenuRole = 0x0100
};

class NetworkModel : public QAbstractItemModel {
	Q_OBJECT
	
	Guimachi& gm;
	
	std::vector<NetworkNode *> nodes; // unique_ptr doesn't like incomplete types
	// the keys are both NetworkUsers and Networks, we actually don't care about their type here
	std::unordered_map<const void *, NetworkNode *> links;
	// the same peer can be in multiple networks
	std::unordered_map<const Peer *, std::vector<NetworkNode *>> peerLinks;
	
	QIcon tunReady;
	QIcon tunRelayed;
	QIcon tunServer;
	QIcon tunBlocked;
	QIcon tunOffline;
	QIcon tunReadyActivity;
	QIcon tunRelayedActivity;
	QIcon tunServerActivity;
	
	QIcon tunProblem;
	QIcon chatMessage;
	
public:
	explicit NetworkModel(Guimachi&, QObject *);
	virtual ~NetworkModel();
	
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	
	void update(const Peer&, ChangeMode);
	void update(const Network&, ChangeMode);
	void update(const NetworkUser&, const Network *, ChangeMode);
	
	void clear();
	void print();
	
//private:
	//void populateModel();
	QMenu * getContextMenu(const QModelIndex&) const;
	QVariant displayData(const Network&, int role) const;
	QVariant displayData(const NetworkUser&, int role) const;
};

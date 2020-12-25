#pragma once

#include <ostream>

class QHostAddress;
class QString;

std::ostream& operator<<(std::ostream &o, const QString &s);
std::ostream& operator<<(std::ostream &o, const QHostAddress &addr);

#ifndef DEVNAMES_H
#define DEVNAMES_H

#include <QString>

struct DevNames{
	inline static const QString& RomaWork() { static QString str="RomaWork"; return str; }
	inline static const QString& RomaHome() { static QString str="RomaHome"; return str; }
	inline static const QString& RomaNotebook() { static QString str="RomaNotebook"; return str; }
};

#endif // DEVNAMES_H

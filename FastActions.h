#ifndef FASTACTIONS_H
#define FASTACTIONS_H

#include <vector>

#include <QStringList>
#include <QDebug>
#include <QMessageBox>

#include "MyQShortings.h"

namespace FastActions_ns {
	const QString execute = "[execute]";

	const QStringList all {execute};
}

struct FastAction
{
	QString command;
	FastAction(QString command): command{std::move(command)} {}

	QString DoAction() const;
};

using vectFunctions = std::vector<std::function<void()>>;

struct FastActions
{
	QStringList actionsVals;
	std::vector<FastAction> actions;

	vectFunctions GetVectFunctions(std::function<void(QString error)> errorWorker = defaultErrWorker)
	{
		vectFunctions vf;
		for(auto &a:actions)
		{
			FastAction copyA = a;
			auto foo = [copyA, errorWorker]()
			{
				if(auto res = copyA.DoAction(); !res.isEmpty())
				{
					if(errorWorker) errorWorker(res);
				}
			};

			vf.emplace_back(std::move(foo));
		}
		return vf;
	}

	inline static std::function<void(QString error)> defaultErrWorker = [](QString error){ QMbError(error); };

	static FastActions Scan(const QString &text);
};

#endif // FASTACTIONS_H

#ifndef FASTACTIONS_H
#define FASTACTIONS_H

#include <vector>
#include <set>

#include <QStringList>
#include <QDebug>
#include <QMessageBox>

#include "MyQShortings.h"

using QStringRefWr_const = std::reference_wrapper<const QString>;
using QStringRefWrVector = std::vector<QStringRefWr_const>;
using QStringRefWr_const_set = std::set<QStringRefWr_const>;

namespace FastActions_ns {
	inline const QString& execute() { static QString str = "[execute]"; return str; }
	inline const QString& git_extensions() { static QString str = "[git_extensions]"; return str; }

	inline const QStringList& all() { static QStringList list { execute(), git_extensions() }; return list; };
	inline const QStringList& all_adder_texts() { static QStringList list { execute(), git_extensions() }; return list; };
}

struct Features
{
	static const QString& messageForNotify() { static QString str = "[message for notify]"; return str; }

	static const QStringList& all() { static QStringList list { messageForNotify() }; return list; };

	static QStringRef HeadForCheckFeature(const QString &content);
	static bool CheckFeature(const QString &content, const QString &feature);
	static QStringRefWr_const_set ScanForFeatures(const QString &content);
};

struct GitExtensionsTool
{
	inline static QString GitExtensionsExe;
	static QString SetAndGetGitExtensionsExe(QString filesPath);
	static bool ExecuteGitExtensions(QString repoDirToOpen, bool chooseGitExensionsExeIfNeed, QString filesPath);
};

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

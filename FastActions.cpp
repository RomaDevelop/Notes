#include "FastActions.h"

#include <QFileDialog>

#include "MyQDifferent.h"
#include "MyQFileDir.h"
#include "MyQExecute.h"

QString FastAction::DoAction() const
{
	QString res;

	if(command.startsWith(FastActions_ns::execute()))
	{
		auto &curKeyWordCommand = FastActions_ns::execute();

		QString subj(command);
		subj.remove(0, curKeyWordCommand.size());
		while(subj.startsWith(' ')) subj.remove(0,1);
		if(!QFile::exists(subj))
		{
			QFileInfo fiSubg(subj);
			qdbg << fiSubg.path();
			if(!QFileInfo(fiSubg.path()).isDir())
				return "for command " + curKeyWordCommand + " path ["+fiSubg.path()+"] doesn't exists";
			else return "for command " + curKeyWordCommand + " bad file name ["+subj+"]";
		}

		if(!MyQExecute::Execute(subj))
			return "MyQExecute::Execute("+subj+") result false";
	}
	else if(command.startsWith(FastActions_ns::git_extensions()))
	{
		auto &curKeyWordCommand = FastActions_ns::git_extensions();

		QString subj(command);
		subj.remove(0, curKeyWordCommand.size());
		while(subj.startsWith(' ')) subj.remove(0,1);

		if(!QFileInfo(subj).isDir())
			return "for command " + curKeyWordCommand + " path ["+subj+"] doesn't exists";

		GitExtensionsTool::ExecuteGitExtensions(subj, true, MyQDifferent::PathToExe()+"/files");
	}
	else res += "unknown command " + command;

	return res;
}

FastActions FastActions::Scan(const QString &text)
{
	FastActions actions;
	for(int i=0; i<text.size(); i++)
	{
		auto c = text[i];
		for(auto &actionKW:FastActions_ns::all())
		{
			if(actionKW.startsWith(c)
					&& i+actionKW.size() <= text.size()
					&& text.midRef(i, actionKW.size()) == actionKW)
			{
				int from = i;
				int to = text.indexOf('\n', from);
				if(to==-1) to = text.size();
				QString action = text.mid(from, to - from);

				while(action.endsWith('\r') || action.endsWith('\t')) action.chop(1);

				actions.actions.emplace_back(std::move(action));
				actions.actionsVals.push_back(actions.actions.back().command);
			}
		}
	}
	return actions;
}

QStringRef Features::HeadForCheckFeature(const QString &content)
{
	return QStringRef(&content, 0, content.size() > 1500 ? 1500 : content.size());
}

bool Features::CheckFeature(const QString &content, const QString &feature)
{
	QStringRef head = HeadForCheckFeature(content);
	return head.contains(feature);
}

QStringRefWr_c_set Features::ScanForFeatures(const QString &content)
{
	QStringRefWr_c_set result;
	QStringRef head = HeadForCheckFeature(content);
	for(auto &feature:all())
	{
		if(head.contains(feature))
			result.insert(feature);
	}
	return result;
}

QString GitExtensionsTool::SetAndGetGitExtensionsExe(QString filesPath)
{
	if(GitExtensionsExe.isEmpty() && QFile::exists(filesPath+"/git_extensions_exe.txt"))
		GitExtensionsExe = MyQFileDir::ReadFile1(filesPath+"/git_extensions_exe.txt");

	if(GitExtensionsExe.isEmpty() || !QFileInfo(GitExtensionsExe).isFile())
	{
		QMbInfo("Программа для работы с репозиториями не обнаружена. Укажите её далее, иначе операция не возможна.");
		GitExtensionsExe = QFileDialog::getOpenFileName(nullptr, "Укажите программу для работы с репозиториями",
														"", "*.exe");
		MyQFileDir::WriteFile(filesPath+"/git_extensions_exe.txt", GitExtensionsExe);
	}
	return GitExtensionsExe;
}

bool GitExtensionsTool::ExecuteGitExtensions(QString repoDirToOpen, bool chooseGitExensionsExeIfNeed, QString filesPath)
{
	auto gitExtensions = GitExtensionsExe;
	if(chooseGitExensionsExeIfNeed)
	{
		gitExtensions = SetAndGetGitExtensionsExe(filesPath);
		if(gitExtensions.isEmpty()) { QMbError("Git extensions not set"); return false; }
	}

	auto execRes = MyQExecute::Execute(gitExtensions, {repoDirToOpen});
	if(!execRes) QMbError("Error executing gitExtensions ["+gitExtensions+"] with param ["+repoDirToOpen+"]" );
	return execRes;
}

#include "FastActions.h"

#include "MyQFileDir.h"
#include "MyQExecute.h"

QString FastAction::DoAction() const
{
	QString res;

	if(QString curKeyWordCommand = FastActions_ns::execute; command.startsWith(curKeyWordCommand))
	{
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
		qdbg << "MyQExecute::Execute(subj)" << subj;
		if(!MyQExecute::Execute(subj))
			return "MyQExecute::Execute("+subj+") result false";
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
		for(auto &actionKW:FastActions_ns::all)
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

#include "Note.h"

#include <QDebug>

#include "MyQShortings.h"
#include "MyQFileDir.h"
#include "MyQDialogs.h"

void Note::SaveNote()
{
	QString endValue = "[endValue]\n";
	QString dtFormat = "yyyy.MM.dd hh:mm:ss";

	QString noteText;
	noteText.append(this->name + endValue);
	noteText.append(QSn(this->activeNotify) + endValue);
	noteText.append(this->dtNotify.toString(dtFormat) + endValue);
	noteText.append(this->dtPostpone.toString(dtFormat) + endValue);
	noteText.append(this->content.code + endValue);

	if(!this->file.isEmpty())
	{
		if(!QFile::remove(this->file)) QMbError("Error removing file " + this->file);
	}
	this->file = MakeNameFileToSaveNote();

	if(!MyQFileDir::WriteFile(this->file, noteText))
	{
		QMbError("Во время сохранения произошла ошибка, сейчас будет показан файл который не сохранился, сохраните содержимое самостоятельно");
		MyQDialogs::ShowText(noteText);
	}
}

QString Note::MakeNameFileToSaveNote()
{
	QString fileName = notesSavesPath;
	fileName += "/note";
	fileName += QSn(index).rightJustified(5,'0');
	fileName += "-";
	fileName += QString(name).replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
	fileName += ".txt";
	return fileName;
}

void Note::Remove()
{
	if(removeWorker) removeWorker();
	else qdbg << "Note::Remove() execed, but removeWorker not valid";
}

bool Note::CheckAlarm(const QDateTime & dateToCompare)
{
	return dateToCompare >= dtPostpone;
}

void Note::ConnectUpdated(std::function<void ()> aUpdatedCb)
{
	if(aUpdatedCb)
		updatedCbs.push_back(aUpdatedCb);
	else qdbg << "ConnectUpdated invalid aUpdated";
}

void Note::EmitUpdated()
{
	for(auto &cb:updatedCbs) cb();
	for(auto &cb:updatedCbs2) cb.cb(cb.handler);
}

void Note::ConnectUpdated(std::function<void (void* handler)> aUpdatedCb, void * handler)
{
	if(aUpdatedCb && handler)
	{
		updatedCbs2.emplace_back();
		updatedCbs2.back().cb = aUpdatedCb;
		updatedCbs2.back().handler = handler;
	}
	else qdbg << "ConnectUpdated invalid aUpdated or handler";
}

bool Note::RemoveCb(void * handler)
{
	for(uint i=0; i<updatedCbs2.size(); i++)
		if(updatedCbs2[i].handler == handler)
		{
			updatedCbs2.erase(updatedCbs2.begin()+i);
			return true;
		}
	return false;
}



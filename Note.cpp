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

bool Note::CheckAlarm(const QDateTime & dateToCompare)
{
	return dateToCompare >= dtPostpone;
}

void Note::ConnectUpdated(std::function<void ()> aUpdated)
{
	updatedCbs.push_back(aUpdated);
}

void Note::EmitUpdated()
{
	for(auto &cb:updatedCbs) if(cb) cb();
}



#include "Note.h"

#include <QDebug>

#include "MyQShortings.h"
#include "MyQFileDir.h"
#include "MyQDialogs.h"

void Note::SetName(QString newName)
{
	name = std::move(newName);
	for(auto &cb:cbsNameUpdated) cb.cb(cb.handler);
}

void Note::SetContent(QString content)
{
	this->content.code = std::move(content);
	for(auto &cb:cbsContentUpdated) cb.cb(cb.handler);
}

bool Note::CmpDTs(const QDateTime &dtNotify, const QDateTime &dtPostpone)
{
	return this->dtNotify == dtNotify && this->dtPostpone == dtPostpone;
}

void Note::SetDT(QDateTime dtNotify, QDateTime dtPostpone)
{
	this->dtNotify = std::move(dtNotify);
	this->dtPostpone = std::move(dtPostpone);
	for(auto &cb:cbsDTUpdated) cb.cb(cb.handler);
}

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

void Note::RemoveNoteFromBase()
{
	if(removeNoteFromBaseWorker) removeNoteFromBaseWorker();
	else qdbg << "Note::Remove() execed, but removeWorker not valid";
}

bool Note::CheckAlarm(const QDateTime & dateToCompare)
{
	return dateToCompare >= dtPostpone;
}

void Note::SetCBNameUpdated(std::function<void (void *)> aUpdatedCb, void *handler, int &localCbCounter)
{
	if(aUpdatedCb && handler)
	{
		auto &cbRef = cbsNameUpdated.emplace_back();
		cbRef.cb = aUpdatedCb;
		cbRef.handler = handler;
		localCbCounter++;
	}
	else qdbg << "ConnectNameUpdated invalid aUpdated or handler";
}

void Note::SetCBContentUpdated(std::function<void (void *)> aUpdatedCb, void *handler, int &localCbCounter)
{
	if(aUpdatedCb && handler)
	{
		auto &cbRef = cbsContentUpdated.emplace_back();
		cbRef.cb = aUpdatedCb;
		cbRef.handler = handler;
		localCbCounter++;
	}
	else qdbg << "ConnectContentUpdated invalid aUpdated or handler";
}

void Note::SetCBDTUpdated(std::function<void (void *handler)> aUpdatedCb, void *handler, int &localCbCounter)
{
	if(aUpdatedCb && handler)
	{
		auto &cbRef = cbsDTUpdated.emplace_back();
		cbRef.cb = aUpdatedCb;
		cbRef.handler = handler;
		localCbCounter++;
	}
	else qdbg << "ConnectDTUpdated invalid aUpdated or handler";
}

void Note::RemoveCbs(void * handler, int removedCountShouldBe)
{
	int countRemoved = 0;
	for(uint i=0; i<cbsNameUpdated.size(); i++)
		if(cbsNameUpdated[i].handler == handler)
		{
			cbsNameUpdated.erase(cbsNameUpdated.begin()+i);
			countRemoved++;
		}
	for(uint i=0; i<cbsContentUpdated.size(); i++)
		if(cbsContentUpdated[i].handler == handler)
		{
			cbsContentUpdated.erase(cbsContentUpdated.begin()+i);
			countRemoved++;
		}
	for(uint i=0; i<cbsDTUpdated.size(); i++)
		if(cbsDTUpdated[i].handler == handler)
		{
			cbsDTUpdated.erase(cbsDTUpdated.begin()+i);
			countRemoved++;
		}
	if(removedCountShouldBe != countRemoved)
		QMbError("removedCountShouldBe != countRemoved for note " + name);
}



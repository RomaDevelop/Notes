#include "Note.h"

#include <QDebug>

#include "MyQShortings.h"



bool Note::CheckAlarm(const QDateTime & dateToCompare)
{
	return dateToCompare >= dtPostpone;
}

void Note::ConnectUpdated(std::function<void ()> aUpdated)
{
	updatedCb = aUpdated;
}

void Note::EmitUpdated()
{
	if(updatedCb) updatedCb();
}

void Note::ConnectContentUpdated(std::function<void ()> cb)
{
	contentUpdatedCb = cb;
}

void Note::EmitContentUpdated()
{
	if(contentUpdatedCb) contentUpdatedCb();
}

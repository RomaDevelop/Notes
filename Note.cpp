#include "Note.h"

#include <QDebug>

#include "MyQShortings.h"

void Note::ConnectUpdated(std::function<void ()> aUpdated)
{
	updated = aUpdated;
}

void Note::EmitUpdated()
{
	if(updated) updated();
}

#ifndef RESOURCES_H
#define RESOURCES_H

#include <QMessageBox>

#include "Resource.h"

DECLARE_RESOURCES(Resources)
	SET_ERROR_WORKER([](QString str){ QMbError(str); })
	DECLARE_RESOURCE(action, "action.ico")
	DECLARE_RESOURCE(add, "add.ico")
	DECLARE_RESOURCE(list, "list.ico")
	DECLARE_RESOURCE(remove, "remove.ico")
END_DECLARE_RESOURCES

#endif // RESOURCES_H

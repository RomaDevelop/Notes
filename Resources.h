#ifndef RESOURCES_H
#define RESOURCES_H

#include <QMessageBox>

#include "Resource.h"

DECLARE_RESOURCES(Resources)
	SET_ERROR_WORKER([](QString str){ QMbError(str); })
	DECLARE_RESOURCE(action, "action.ico")
	DECLARE_RESOURCE(add, "add.ico")
	DECLARE_RESOURCE(list, "list.ico")
	DECLARE_RESOURCE(list_mo, "list_mo.ico")
	DECLARE_RESOURCE(list_na, "list_na.ico")
	DECLARE_RESOURCE(list_ro, "list_ro.ico")
	DECLARE_RESOURCE(remove, "remove.ico")
	DECLARE_RESOURCE(find, "find.ico")
END_DECLARE_RESOURCES;

#endif // RESOURCES_H

#include "WidgetMain.h"

#include <QApplication>

#include "Resources.h"

#include "MyQDifferent.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setQuitOnLastWindowClosed(false);

	Resources::Init("C:\\Work\\C++\\Notes\\resources", MyQDifferent::PathToExe()+"\\files\\resources", true);

	WidgetMain w;
	w.show();
	return a.exec();
}

#include "WidgetMain.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setQuitOnLastWindowClosed(false);

	WidgetMain w;
	w.show();
	return a.exec();
}

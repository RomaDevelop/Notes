#include "MainWidgetNotes.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setQuitOnLastWindowClosed(false);

	MainWidget w;
	w.show();
	return a.exec();
}

#include "WidgetMain.h"

#include <QApplication>

#include "Resources.h"

#include "MyQDifferent.h"

int main(int argc, char *argv[])
{

	qdbg << "сохранять ширину колонок";
	qdbg << "дома из-за увеличенных шрифтов есть некоторые уродства";
	qdbg << "экзешнику картинку";

	QApplication a(argc, argv);
	a.setQuitOnLastWindowClosed(false);

	//Resources::Init("C:\\Work\\C++\\Notes\\resources", MyQDifferent::PathToExe()+"\\files\\resources", true);
	Resources::Init("D:\\Documents\\C++ QT\\Notes\\resources", MyQDifferent::PathToExe()+"\\files\\resources", true);
	#error

	WidgetMain w;
	w.show();
	return a.exec();
}

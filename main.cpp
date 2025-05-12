#include "WidgetMain.h"

#include <QApplication>

#include "Resources.h"

#include "MyQDifferent.h"
#include "LaunchParams.h"

int main(int argc, char *argv[])
{

	qdbg << "сохранять ширину колонок";
	qdbg << "дома из-за увеличенных шрифтов есть некоторые уродства";
	qdbg << "экзешнику картинку";

	QApplication a(argc, argv);
	a.setQuitOnLastWindowClosed(false);

	LaunchParams::Init({
						   LaunchParams::DeveloperData("RomaWork", "TKO3-206", "C:/Work/C++/Notes",
						   "D:/Documents/C++ QT/"),
						   LaunchParams::DeveloperData("RomaHome", "don't remember", "D:/Documents/C++ QT/Notes",
						   "C:/Work/C++/")
					   });

	Resources::Init(LaunchParams::CurrentDeveloper()->sourcesPath + "/resources", MyQDifferent::PathToExe()+"/files/resources", true);

	WidgetMain w;
	w.show();
	return a.exec();
}

#include "WidgetMain.h"

#include <QApplication>

#include "Resources.h"

#include "MyQDifferent.h"
#include "LaunchParams.h"

#include "DevNames.h"

#include "DialogInputTime.h"

int main(int argc, char *argv[])
{
	qdbg << "дома из-за увеличенных шрифтов есть некоторые уродства";
	qdbg << "экзешнику картинку";

	QApplication a(argc, argv);
	a.setQuitOnLastWindowClosed(false);

	LaunchParams::Init({
						   LaunchParams::DeveloperData(DevNames::RomaWork(), "TKO3-206", "C:/Work/C++/Notes",
						   "D:/Documents/C++ QT/"),
						   LaunchParams::DeveloperData(DevNames::RomaHome(), "ELBRUS-HOME-10", "D:/Documents/C++ QT/Notes",
						   "C:/Work/C++/"),
						   LaunchParams::DeveloperData(DevNames::RomaNotebook(), "ELBRUS-notebook", "D:/Work/Notes",
						   "C:/Work/C++/"),
					   });

	Resources::Init(LaunchParams::CurrentDeveloper().sourcesPath + "/resources", MyQDifferent::PathToExe()+"/files/resources", true);

	WidgetMain w;
	w.show();

	return a.exec();
}

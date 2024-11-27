#include "MainWidget.h"

#include <QDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>

#include "MyQDifferent.h"
#include "MyQDialogs.h"

#include "NoteEditor.h"

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

	QPushButton *btn1 = new QPushButton("+");
	btn1->setFixedWidth(25);
	hlo1->addWidget(btn1);
	connect(btn1,&QPushButton::clicked,[this](){
		notes.push_back(std::make_unique<Note>());
		notes.back()->name = MyQDialogs::InputText();
		table->setRowCount(table->rowCount()+1);
		table->setItem(table->rowCount()-1, 0, new QTableWidgetItem);
		table->item(table->rowCount()-1, 0)->setText(notes.back()->name);
	});

	hlo1->addStretch();

	table = new QTableWidget;
	table->setColumnCount(1);
	hlo2->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		auto editor = new NoteEditor(*notes[r].get());
		editor->show();
	});

	settingsFile = MyQDifferent::PathToExe()+"/files/settings.ini";
	QTimer::singleShot(0,this,[this]
	{
		//move(10,10);
		//resize(1870,675);
		LoadSettings();
	});
}

MainWidget::~MainWidget()
{

}

void MainWidget::closeEvent(QCloseEvent * event)
{
//	auto answ = QMessageBox::question(this,"Закрытие ...","...");
//	if(0){}
//	else if(answ == QMessageBox::Yes) {/*ничего не делаем*/}
//	else if(answ == QMessageBox::No) { event->ignore(); return; }
//	else { qCritical() << "not realesed button 0x" + QString::number(answ,16); return; }

	SaveSettings();
	event->accept();
}

void MainWidget::SaveSettings()
{
	QDir().mkpath(MyQDifferent::PathToExe()+"/files");

	QSettings settings(settingsFile, QSettings::IniFormat);

	settings.setValue("geo", this->saveGeometry());

	settings.beginGroup("group");
	settings.setValue("other", "something");
	settings.endGroup();
}

void MainWidget::LoadSettings()
{
	if(!QFile::exists(settingsFile)) return;

	QSettings settings(settingsFile, QSettings::IniFormat);

	this->restoreGeometry(settings.value("geo").toByteArray());

	settings.beginGroup("group");
	if(0) qDebug() << settings.value("other");
	settings.endGroup();
}





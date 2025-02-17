#include "WidgetNotifie.h"

#include <QVBoxLayout>
#include <QCloseEvent>
#include <QSettings>
#include <QLabel>
#include <QPushButton>

#include "MyQShortings.h"
#include "MyQTableWidget.h"
#include "MyQFileDir.h"
#include "MyQDifferent.h"
#include "declare_struct.h"

#include "NoteEditor.h"

WidgetAlarms::WidgetAlarms(QWidget * parent)
	: QWidget(parent)
{
	settingsFile = MyQDifferent::PathToExe() + "/files/settings_widget_alarms.ini";

	setWindowTitle("Notes notify");
	setWindowFlag(Qt::WindowCloseButtonHint);

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

	table = new QTableWidget;
	table->setColumnCount(1);
	table->setHorizontalHeader(nullptr);
	hlo2->addWidget(table);
	connect(table, &QTableWidget::cellDoubleClicked, [this](int r, int){
		NoteEditor::MakeNoteEditor(*notes[r]);
	});
}

WidgetAlarms::~WidgetAlarms()
{
	SaveSettings();
}

void WidgetAlarms::GiveNotes(const std::vector<Note *> & givingNotes)
{
	for(uint i=0; i<notes.size();)
	{
		if(std::find(givingNotes.begin(), givingNotes.end(), notes[i]) == givingNotes.end())
			RemoveNote(i);
		else ++i;
	}

	for(auto &newNote:givingNotes)
	{
		if(std::find(notes.begin(), notes.end(), newNote) == notes.end())
			AddNote(newNote);
	}
}

void WidgetAlarms::AddNote(Note * note)
{
	notes.emplace_back(note);

	table->setRowCount(table->rowCount()+1);
	int row = table->rowCount()-1;
	auto widget = new QWidget;
	table->setCellWidget(row, 0, widget);

	auto hlo = new QHBoxLayout(widget);
	hlo->setContentsMargins(0,0,0,0);
	hlo->addWidget(new QLabel(note->name + " ("+note->dtNotify.toString()+")"));
	hlo->addStretch();
	hlo->addWidget(new QPushButton("Перенести на ..."));

	auto btnPostpone = new QPushButton("Отложить на ...");
	hlo->addWidget(btnPostpone);
	connect(btnPostpone, &QPushButton::clicked, [this, btnPostpone](){
		static QMenu *menu = nullptr;
		if(!menu)
		{
			menu = new QMenu(this);
			declare_struct_2_fields_move(Delay, QString, text, int, seconds);
			std::vector<Delay> delays {{"5 минут", 60*5}, {"10 минут", 60*10}};
			for(auto &delay:delays)
			{
				menu->addAction(new QAction(delay.text, menu));
				int delaySecs = delay.seconds;
				connect(menu->actions().back(), &QAction::triggered, [this, delaySecs](){
					Note* currentNote = notes[table->currentRow()];
					currentNote->dtPostpone = QDateTime::currentDateTime().addSecs(delaySecs);
					currentNote->EmitUpdated();
				});
			}
		}
		menu->exec(btnPostpone->mapToGlobal(QPoint(0, btnPostpone->height())));
	});
}

void WidgetAlarms::RemoveNote(int index)
{
	table->removeRow(index);
	notes.erase(notes.begin() + index);
}

void WidgetAlarms::showEvent(QShowEvent * event)
{
	if(!QFile::exists(settingsFile)) return;

	QSettings settings(settingsFile, QSettings::IniFormat);
	restoreGeometry(settings.value("geo").toByteArray());
	MyQTableWidget::LoadColsWidths(table, settings.value("col_widths").toByteArray());
	event->accept();
}

void WidgetAlarms::closeEvent(QCloseEvent * event)
{
	SaveSettings();
	event->accept();
}

void WidgetAlarms::SaveSettings()
{
	QDir().mkpath(QFileInfo(settingsFile).path());
	MyQFileDir::WriteFile(settingsFile, "");
	QSettings settings(settingsFile, QSettings::IniFormat);

	settings.setValue("geo", saveGeometry());
	settings.setValue("col_widths", MyQTableWidget::SaveColsWidths(table));
}


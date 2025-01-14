#include "WidgetNotifie.h"

#include <QVBoxLayout>
#include <QCloseEvent>
#include <QSettings>

#include "MyQTableWidget.h"
#include "MyQFileDir.h"
#include "MyQDifferent.h"

#include "NoteEditor.h"

WidgetAlarms::WidgetAlarms(QWidget * parent)
	: QWidget(parent)
{
	settingsFile = MyQDifferent::PathToExe() + "/files/settings_widget_alarms.ini";

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
	table->setRowCount(table->rowCount()+1);
	int row = table->rowCount()-1;
	table->setItem(row, 0, new QTableWidgetItem);
	table->item(row,0)->setText(note->name);
	notes.emplace_back(note);
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
	QDir().mkpath(QFileInfo(settingsFile).path());
	MyQFileDir::WriteFile(settingsFile, "");
	QSettings settings(settingsFile, QSettings::IniFormat);

	settings.setValue("geo", saveGeometry());
	settings.setValue("col_widths", MyQTableWidget::SaveColsWidths(table));
	event->accept();
}


#include "WidgetNoteEditor.h"

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
#include <QDateTimeEdit>
#include <QShortcut>
#include <QStatusBar>

#include "MyCppDifferent.h"
#include "PlatformDependent.h"
#include "MyQDifferent.h"
#include "MyQString.h"
#include "MyQWidget.h"
#include "MyQDialogs.h"

#include "FastActions.h"
#include "DataBase.h"

WidgetNoteEditor* WidgetNoteEditor::MakeOrShowNoteEditor(Note &note, bool noteIsTmpNote)
{
	if(!noteIsTmpNote) DataBase::AddOpensCount(QSn(note.id), 1);

	if(auto existingEditor = existingEditors.find(&note); existingEditor == existingEditors.end())
	{
		auto editor = new WidgetNoteEditor(note);
		editor->show();
		return editor;
	}
	else
	{
		if(existingEditor->second->isMinimized())
		{
			existingEditor->second->showNormal();
		}
		else
		{
			existingEditor->second->show();
			PlatformDependent::SetTopMost(existingEditor->second,true);
			PlatformDependent::SetTopMost(existingEditor->second,false);
		}
		return existingEditor->second;
	}
}

WidgetNoteEditor *WidgetNoteEditor::MakeOrShowNoteEditorTmpNote(QStringList noteRecord)
{
	std::shared_ptr<Note> tmpNote = Note::CreateFromRecord_shptr(noteRecord);
	auto editor = MakeOrShowNoteEditor(*tmpNote.get(), true);
	editor->StoreTmpNote(tmpNote);
	editor->SetReadOnly();
	return editor;
}

void WidgetNoteEditor::SetReadOnly()
{
	auto children = this->findChildren<QWidget*>();
	for(auto &obj:children)
	{
		if(auto castedW = dynamic_cast<QLineEdit*>(obj)) castedW->setReadOnly(true);
		else if(auto castedW = dynamic_cast<QDateTimeEdit*>(obj)) castedW->setReadOnly(true);
		else if(auto castedW = dynamic_cast<MyQTextEdit*>(obj)) castedW->setReadOnly(true);
		else if(auto castedW = dynamic_cast<QPushButton*>(obj)) castedW->setEnabled(false);
		else
		{
			static std::set<QString> ignoreTypes
				{ "QWidget", "QLabel", "QScrollBar", "QStatusBar", "QSizeGrip" };

			if(ignoreTypes.count(obj->metaObject()->className()) != 0) continue;
			QMbError("Unrealesed type to SetReadOnly " + QString(obj->metaObject()->className()));
		}
	}
}

WidgetNoteEditor::WidgetNoteEditor(Note &note, QWidget *parent):
	QWidget(parent),
	note {note}
{
	setWindowTitle(note.Name() + " - NoteEditor");
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hloNameAndDates = new QHBoxLayout;
	QHBoxLayout *hloButtons = new QHBoxLayout;
	QHBoxLayout *hloTextEdit = new QHBoxLayout;

	vlo_main->addLayout(hloNameAndDates);

	leName = new QLineEdit(note.Name());
	MyQWidget::SetFontPointSize(leName, 12);
	MyQWidget::SetFontBold(leName, true);
	connect(leName, &QLineEdit::textChanged, this, [this](){ SetHaveChangesTrue("leName"); });

	dtEditNotify = new QDateTimeEdit(note.DTNotify());
	MyQWidget::SetFontPointSize(dtEditNotify, 12);
	MyQWidget::SetFontBold(dtEditNotify, true);
	dtEditNotify->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	dtEditNotify->setCalendarPopup(true);

	auto btnDtEditNotifyAdd = new QPushButton("...");
	btnDtEditNotifyAdd->setFixedWidth(28);
	connect(btnDtEditNotifyAdd,&QPushButton::clicked, [this, btnDtEditNotifyAdd](){
		MyQDialogs::MenuUnderWidget(btnDtEditNotifyAdd, {
			{"09:00:00", [this](){ dtEditNotify->setTime(QTime(9,0,0)); }},
			{"09:30:00", [this](){ dtEditNotify->setTime(QTime(9,30,0)); }},
			{"10:00:00", [this](){ dtEditNotify->setTime(QTime(10,0,0)); }},
			{"10:30:00", [this](){ dtEditNotify->setTime(QTime(10,30,0)); }},
			{"11:00:00", [this](){ dtEditNotify->setTime(QTime(11,0,0)); }},
			{"11:30:00", [this](){ dtEditNotify->setTime(QTime(11,30,0)); }},
			{"12:00:00", [this](){ dtEditNotify->setTime(QTime(12,0,0)); }},
			{"13:00:00", [this](){ dtEditNotify->setTime(QTime(13,0,0)); }},
			{"14:00:00", [this](){ dtEditNotify->setTime(QTime(14,0,0)); }},
			{"15:00:00", [this](){ dtEditNotify->setTime(QTime(15,0,0)); }},
			{"16:00:00", [this](){ dtEditNotify->setTime(QTime(16,0,0)); }},
			{"17:00:00", [this](){ dtEditNotify->setTime(QTime(17,0,0)); }},
			{"18:00:00", [this](){ dtEditNotify->setTime(QTime(18,0,0)); }},
			{"19:00:00", [this](){ dtEditNotify->setTime(QTime(19,0,0)); }},
			{"20:00:00", [this](){ dtEditNotify->setTime(QTime(20,0,0)); }},
			{"21:00:00", [this](){ dtEditNotify->setTime(QTime(21,0,0)); }},
		});
	});

	dtEditPostpone = new QDateTimeEdit(note.DTPostpone());
	MyQWidget::SetFontPointSize(dtEditPostpone, 12);
	MyQWidget::SetFontBold(dtEditPostpone, true);
	dtEditPostpone->setDisplayFormat("dd.MM.yyyy HH:mm:ss");

	connect(dtEditNotify, &QDateTimeEdit::dateTimeChanged, [this](const QDateTime &datetime){
		dtEditPostpone->setDateTime(datetime);
		SetHaveChangesTrue("dtEditNotify");
	});
	connect(dtEditPostpone, &QDateTimeEdit::dateTimeChanged, [this](const QDateTime &){
		SetHaveChangesTrue("dtEditPostpone");
	});

	hloNameAndDates->addWidget(leName);
	hloNameAndDates->addWidget(dtEditNotify);
	hloNameAndDates->addWidget(btnDtEditNotifyAdd);
	hloNameAndDates->addWidget(dtEditPostpone);

	vlo_main->addLayout(hloButtons);
	vlo_main->addLayout(hloTextEdit);

	auto SlotSetBold = [this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		if(format.fontWeight() == QFont::Weight::Normal)
			format.setFontWeight(QFont::Weight::Bold);
		else format.setFontWeight(QFont::Weight::Normal);

		cursor.setCharFormat(format);

		textEdit->setFocus();
	};
	auto SlotSetUndL = [this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		format.setFontUnderline(!format.fontUnderline());
		cursor.setCharFormat(format);

		textEdit->setFocus();
	};
	auto SlotSetItal = [this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		format.setFontItalic(!format.fontItalic());
		cursor.setCharFormat(format);

		textEdit->setFocus();
	};

	QPushButton *btnBold = new QPushButton("B");
	btnBold->setStyleSheet("font-weight: bold;");
	btnBold->setFixedWidth(25);
	hloButtons->addWidget(btnBold);
	connect(btnBold,&QPushButton::clicked, SlotSetBold);
	QShortcut *shortcutCtrlB = new QShortcut(QKeySequence("Ctrl+B"), this);
	connect(shortcutCtrlB, &QShortcut::activated, SlotSetBold);

	QPushButton *btnUnderlined = new QPushButton("U");
	btnUnderlined->setStyleSheet("text-decoration: underline;");
	btnUnderlined->setFixedWidth(25);
	hloButtons->addWidget(btnUnderlined);
	connect(btnUnderlined,&QPushButton::clicked,SlotSetUndL);
	QShortcut *shortcutCtrlU = new QShortcut(QKeySequence("Ctrl+U"), this);
	connect(shortcutCtrlU, &QShortcut::activated, SlotSetUndL);

	QPushButton *btnItalic = new QPushButton("I");
	btnItalic->setStyleSheet("font-style: italic;");
	btnItalic->setFixedWidth(25);
	hloButtons->addWidget(btnItalic);
	connect(btnItalic,&QPushButton::clicked,SlotSetItal);
	QShortcut *shortcutCtrlI = new QShortcut(QKeySequence("Ctrl+I"), this);
	connect(shortcutCtrlI, &QShortcut::activated, SlotSetItal);

	QPushButton *btnAPlus = new QPushButton("A+");
	btnAPlus->setFixedWidth(30);
	hloButtons->addWidget(btnAPlus);
	connect(btnAPlus,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		format.setFontPointSize(format.fontPointSize()+1);
		cursor.setCharFormat(format);
		textEdit->setFocus();
	});

	QPushButton *btnAMinus = new QPushButton("A-");
	btnAMinus->setFixedWidth(30);
	hloButtons->addWidget(btnAMinus);
	connect(btnAMinus,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		int start = cursor.selectionStart();
		QTextCharFormat format = MyQTextEdit::LetterFormat(textEdit, start);

		format.setFontPointSize(format.fontPointSize()-1);
		cursor.setCharFormat(format);
		textEdit->setFocus();
	});

	QPushButton *btnUpeerFirstLetter = new QPushButton("Aa");
	btnUpeerFirstLetter->setFixedWidth(30);
	hloButtons->addWidget(btnUpeerFirstLetter);
	connect(btnUpeerFirstLetter,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		QString selectedText = cursor.selectedText();
		selectedText = MyQString::ToUpperWordStartLetter(selectedText);
		cursor.insertText(selectedText);

		cursor.setPosition(cursor.position() - selectedText.length(), QTextCursor::KeepAnchor);
		textEdit->setTextCursor(cursor);
		textEdit->setFocus();
	});

	QPushButton *btnSentenceCase = new QPushButton("Aa.");
	btnSentenceCase->setFixedWidth(30);
	hloButtons->addWidget(btnSentenceCase);
	connect(btnSentenceCase,&QPushButton::clicked,[this](){
		auto cursor = textEdit->textCursor();
		if(!cursor.hasSelection()) return;

		QString selectedText = cursor.selectedText();
		selectedText = MyQString::ToSentenceCase(selectedText);
		cursor.insertText(selectedText);

		cursor.setPosition(cursor.position() - selectedText.length(), QTextCursor::KeepAnchor);
		textEdit->setTextCursor(cursor);
		textEdit->setFocus();
	});

	QPushButton *btnAddFeature = new QPushButton(" Add feature ");
	hloButtons->addWidget(btnAddFeature);
	connect(btnAddFeature,&QPushButton::clicked,[this, btnAddFeature](){

		auto addFoo = [this](){
			auto cursor = this->textEdit->textCursor();
			cursor.setPosition(0);
			cursor.insertText(Features::messageForNotify());
			textEdit->setFocus();
		};

		MyQDialogs::MenuUnderWidget(btnAddFeature,
									{
										{ Features::messageForNotify(), addFoo },
									});
	});

	QPushButton *btnAddAction = new QPushButton(" Add action ");
	hloButtons->addWidget(btnAddAction);
	connect(btnAddAction,&QPushButton::clicked,[this, btnAddAction](){

		std::vector<MyQDialogs::MenuItem> items;
		for(auto &fastAction:FastActions_ns::all_adder_texts())
		{
			items.emplace_back();
			items.back().text = fastAction;
			items.back().worker = [this, &fastAction](){
				this->textEdit->textCursor().insertText(fastAction);
				textEdit->setFocus();
			};
		}

		MyQDialogs::MenuUnderWidget(btnAddAction, std::move(items));
	});

	QPushButton *btnDoActions = new QPushButton(" Do actions ");
	hloButtons->addWidget(btnDoActions);
	connect(btnDoActions,&QPushButton::clicked,[this, btnDoActions](){
		auto actions = FastActions::Scan(textEdit->toPlainText());

		if(!actions.actionsVals.isEmpty())
			MyQDialogs::MenuUnderWidget(btnDoActions, actions.actionsVals, actions.GetVectFunctions());
		else MyQDialogs::MenuUnderWidget(btnDoActions, { MyQDialogs::DisabledItem("Actions not found") });
	});

	hloButtons->addStretch();

//	QPushButton *btnTest = new QPushButton(" Test ");
//	hloButtons->addWidget(btnTest);
//	connect(btnTest,&QPushButton::clicked, [this, btnDtEditNotifyAdd](){
//		qdbg << btnDtEditNotifyAdd->height() << dtEditNotify->height();
//	});

	textEdit = new MyQTextEdit;
	textEdit->richTextPaste = false;
	textEdit->setTabStopDistance(40);
	hloTextEdit->addWidget(textEdit);
	if(note.Content() == Note::StartText()) note.SetContent("<span style='font-size: 14pt;'>"+Note::StartText()+"</span>");
	textEdit->document()->setDefaultStyleSheet("p { font-size: 14pt; }");
	textEdit->setHtml(note.Content());
	connect(textEdit, &QTextEdit::textChanged, [this](){
		SetHaveChangesTrue("textEdit");
	});

	auto dtUpdateFoo = [this](void *){
		MyCppDifferent::any_guard(notChagesNow, true, false);
		dtEditNotify->setDateTime(this->note.DTNotify());
		dtEditPostpone->setDateTime(this->note.DTPostpone());
	};

	QStatusBar *statusBar = new QStatusBar(this);
	vlo_main->addWidget(statusBar);
	labelStatus = new QLabel;
	statusBar->addWidget(labelStatus);

	note.AddCBDTUpdated(dtUpdateFoo, this, cbCounter);

	settingsFile = MyQDifferent::PathToExe()+"/files/settings_note_editor.ini";
	QTimer::singleShot(0,this,[this]
	{
		for(auto &foo:postShowFunctions)
			foo();
		postShowFunctions.clear();
		LoadSettings();
	});

	InitTimerNoteSaver();

	existingEditors[&note] = this;
}

void WidgetNoteEditor::InitTimerNoteSaver()
{
	auto timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, [this](){ SaveNoteFromEditor(); });
	timer->start(1000);
}

WidgetNoteEditor::~WidgetNoteEditor()
{
	qdbg << "~WidgetNoteEditor " + leName->text();

	note.RemoveCbs(this, cbCounter);

	SaveNoteFromEditor(true);

	if(auto thisEditor = existingEditors.find(&note); thisEditor != existingEditors.end())
	{
		existingEditors.erase(thisEditor);
	}
	else QMbc(0,"Error", "destructor called, but this editor not in the existingEditors");
}

void WidgetNoteEditor::closeEvent(QCloseEvent * event)
{
	SaveSettings();
	event->accept();
}

void WidgetNoteEditor::SaveSettings()
{
	QDir().mkpath(MyQDifferent::PathToExe()+"/files");

	QSettings settings(settingsFile, QSettings::IniFormat);

	settings.setValue("geoNoteEditor", saveGeometry());
}

void WidgetNoteEditor::LoadSettings()
{
	if(!QFile::exists(settingsFile)) return;

	QSettings settings(settingsFile, QSettings::IniFormat);

	restoreGeometry(settings.value("geoNoteEditor").toByteArray());
	std::vector<QPoint> existingsEditorsPoses;
	for(auto &pair:existingEditors) if(pair.second != this and pair.second->isVisible()) existingsEditorsPoses.push_back(pair.second->pos());

	auto checkPos = [&existingsEditorsPoses](const QPoint& pos) -> bool
	{
		for(auto &existingPos:existingsEditorsPoses)
		{
			if(abs(abs(existingPos.x()) - abs(pos.x())) < 10 and abs(abs(existingPos.y()) - abs(pos.y())) < 10)
			{
				//qdbg << "checkPos false";
				return false;
			}
		}
		//qdbg << "checkPos true";
		return true;
	};

	if(!checkPos(pos()))
	{
		QScreen* editorScreen = MyQWidget::WidgetScreen(this);
		if(!editorScreen) { QMbError("!editorScreen"); return; }
		auto screenGeo = editorScreen->geometry();

		auto curPos = pos();
		int poses = 0;
		while(!checkPos(curPos))
		{
			curPos += QPoint(75,75);
			if(screenGeo.x() + screenGeo.width() < curPos.x() + this->width() + 20
					|| screenGeo.y() + screenGeo.height() < curPos.y() + this->height() + 40)
			{
				while(curPos.y() - screenGeo.y() > 30)
				{
					curPos -= QPoint(1,1);
				}
				curPos += QPoint(30,0);
			}
			poses++;
			if(poses > 100) break;
		}
		move(curPos);
	}
}

void WidgetNoteEditor::SetHaveChangesTrue(QString widget)
{
	if(notChagesNow) return;

	qdbg << "SetHaveChangesTrue: " + widget;

	haveChanges = true;
	lastChagesDid = QDateTime::currentDateTime();
}

void WidgetNoteEditor::SaveNoteFromEditor(bool forceSave)
{
	if(!haveChanges) return;

	if(!forceSave)
	{
		if(lastSaveDid.isValid() && lastSaveDid.secsTo(QDateTime::currentDateTime()) < 5) return;
		if(lastChagesDid.isValid() && lastChagesDid.secsTo(QDateTime::currentDateTime()) < 5) return;
	}

	note.SetContent(textEdit->toHtml());

	QString newName = leName->text();
	if(note.Name() != newName) note.SetName(newName);

	auto dtN = dtEditNotify->dateTime(), dtP = dtEditPostpone->dateTime();
	if(!note.CmpDTs(dtN, dtP)) note.SetDT(dtN, dtP);

	haveChanges = false;
	lastSaveDid = QDateTime::currentDateTime();
	labelStatus->setText("Saved at " + lastSaveDid.toString(DateTimeFormat) + " saved index ("+QSn(++savedCount)+")");

	if(0) CodeMarkers::to_do("при вызове этой функции коллбеки обновления вызываются несколько раз, лишняя работа");
}





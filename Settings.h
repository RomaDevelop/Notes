#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDebug>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>

template<typename T>
struct property
{
	property(T value_, std::function<void()> cbChanged_): value{std::move(value_)}, cbChanged{std::move(cbChanged_)}
	{
		/*qDebug() << "property(T value_)";*/
		if(!cbChanged)
		{
			qCritical() << "invalid property arg cbChanged_";
			cbChanged = [](){ qDebug() << "property changed"; };
		}
	}
	property(const property& other) { /*qDebug() << "copy";*/ value = other.value; cbChanged(); }
	property(property&& other) { /*qDebug() << "move";*/ value = std::move(other.value); cbChanged(); }
	property& operator=(const property& other) {
		//qDebug() << "copy property = ";
		if (this != &other) value = other.value;
		cbChanged();
		return *this;
	}
	property& operator=(property&& other) {
		//qDebug() << "move property =";
		if (this != &other) value = std::move(other.value);
		cbChanged();
		return *this;
	}
	property& operator=(const T &value_) {
		//qDebug() << "copy T = ";
		value = value_;
		cbChanged();
		return *this;
	}
	property& operator=(T &&value_) {
		//qDebug() << "move T =";
		value = std::move(value_);
		cbChanged();
		return *this;
	}

	const T& get() { return value; }
	operator T() const { return value; }

private:
	T value;
	std::function<void()> cbChanged;
};

struct Settings
{
	inline static bool haveNotSavedChanges = false;
	inline static bool disableCbPropChanged = false;

	inline static std::function<void()> cbPropChanged = [](){
		/*qDebug() << "property changed";*/
		if(disableCbPropChanged) return;
		haveNotSavedChanges = true;
	};

	inline static property<bool> AlarmsJoinEnabled {false, cbPropChanged};
	inline static property<int> AlarmsJoinMaxSecs {60*4, cbPropChanged};

	static QString SaveToString();
	static void LoadFromString(const QString &str);
};

class DialogSettings : public QDialog
{
	Q_OBJECT
public:
	static void Execute();

private:
	explicit DialogSettings();

	QCheckBox *chBoxAlarmsJoinEnabled;
	QLineEdit *leAlarmsJoinMaxSecs;
};

#endif // SETTINGS_H

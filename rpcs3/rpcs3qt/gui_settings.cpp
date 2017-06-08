#include "gui_settings.h"

#include "game_list_frame.h"

#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

gui_settings::gui_settings(QObject* parent) : QObject(parent), settings(ComputeSettingsDir() + tr("CurrentSettings") + ".ini", QSettings::Format::IniFormat, parent),
	settingsDir(ComputeSettingsDir())
{
}

gui_settings::~gui_settings()
{
	settings.sync();
}

QString gui_settings::ComputeSettingsDir()
{
	QString path = QDir(QDir::currentPath()).relativeFilePath(QCoreApplication::applicationDirPath());
	path += "/GuiConfigs/";
	return path;
}

void gui_settings::ChangeToConfig(const QString& name)
{
	if (name != tr("CurrentSettings"))
	{ // don't try to change to yourself.
		Reset(false);

		QSettings other(settingsDir.absoluteFilePath(name + ".ini"), QSettings::IniFormat);

		QStringList keys = other.allKeys();
		for (QStringList::iterator i = keys.begin(); i != keys.end(); i++)
		{
			settings.setValue(*i, other.value(*i));
		}
		settings.sync();
	}
}

void gui_settings::Reset(bool removeMeta)
{
	if (removeMeta)
	{
		settings.clear();
	}
	else
	{
		settings.remove(GUI::logger);
		settings.remove(GUI::main_window);
		settings.remove(GUI::game_list);
	}
}

QVariant gui_settings::GetValue(const GUI_SAVE& entry)
{
	return settings.value(entry.key + "/" + entry.name, entry.def);
}

QVariant gui_settings::List2Var(const q_pair_list& list)
{
	QByteArray ba;
	QDataStream stream(&ba, QIODevice::WriteOnly);
	stream << list;
	return QVariant(ba);
}

q_pair_list gui_settings::Var2List(const QVariant& var)
{
	q_pair_list list;
	QByteArray ba = var.toByteArray();
	QDataStream stream(&ba, QIODevice::ReadOnly);
	stream >> list;
	return list;
}

void gui_settings::SetValue(const GUI_SAVE& entry, const QVariant& value)
{
	settings.beginGroup(entry.key);
	settings.setValue(entry.name, value);
	settings.endGroup();
}

QStringList gui_settings::GetGameListCategoryFilters()
{
	QStringList filterList;
	if (GetCategoryVisibility(category::hdd_Game)) filterList.append(category::hdd_Game);
	if (GetCategoryVisibility(category::disc_Game)) filterList.append(category::disc_Game);
	if (GetCategoryVisibility(category::home)) filterList.append(category::home);
	if (GetCategoryVisibility(category::audio_Video)) filterList.append(category::audio_Video);
	if (GetCategoryVisibility(category::game_Data)) filterList.append(category::game_Data);
	if (GetCategoryVisibility(category::unknown)) filterList.append(category::unknown);
	return filterList;
}

bool gui_settings::GetCategoryVisibility(QString cat)
{
	GUI_SAVE value;

	if (cat == category::hdd_Game) value = GUI::cat_hdd_game;
	else if (cat == category::disc_Game) value = GUI::cat_disc_game;
	else if (cat == category::home) value = GUI::cat_home;
	else if (cat == category::audio_Video) value = GUI::cat_audio_video;
	else if (cat == category::game_Data) value = GUI::cat_game_data;
	else if (cat == category::unknown) value = GUI::cat_unknown;
	else
	{
		value = GUI::cat_other;
		LOG_WARNING(GENERAL, "Category %s is unknown. Entry %s was loaded", sstr(cat), sstr(value.name));
	}

	return GetValue(value).toBool();
}

void gui_settings::SetCategoryVisibility(const QString& cat, const bool& val)
{
	GUI_SAVE value;

	if (cat == category::hdd_Game) value = GUI::cat_hdd_game;
	else if (cat == category::disc_Game) value = GUI::cat_disc_game;
	else if (cat == category::home) value = GUI::cat_home;
	else if (cat == category::audio_Video) value = GUI::cat_audio_video;
	else if (cat == category::game_Data) value = GUI::cat_game_data;
	else if (cat == category::unknown) value = GUI::cat_unknown;
	else
	{
		value = GUI::cat_other;
		LOG_WARNING(GENERAL, "Category %s is unknown. Entry %s has been set to %d", sstr(cat), sstr(value.name), val);
	}

	SetValue(value, val);
}

void gui_settings::ShowInfoBox(const GUI_SAVE& entry, const QString& title, const QString& text, QWidget* parent)
{
	if (GetValue(entry).toBool())
	{
		QCheckBox* cb = new QCheckBox(tr("Don't show again"));
		QMessageBox* mb = new QMessageBox(QMessageBox::Information, title, text, QMessageBox::Ok, parent);
		mb->setCheckBox(cb);
		mb->deleteLater();
		mb->exec();
		if (mb->checkBox()->isChecked())
		{
			SetValue(entry, false);
			LOG_WARNING(GENERAL, "Entry %s was set to false", sstr(entry.name));
		}
	}
	else LOG_WARNING(GENERAL, "Entry %s is false, Info Box not shown", sstr(entry.name));
}

void gui_settings::SetGamelistColVisibility(int col, bool val)
{
	SetValue(GUI_SAVE(GUI::game_list, "Col" + QString::number(col) + "visible", true), val);
}

void gui_settings::SaveCurrentConfig(const QString& friendlyName)
{
	SetValue(GUI::m_currentConfig, friendlyName);
	BackupSettingsToTarget(friendlyName);
}

logs::level gui_settings::GetLogLevel()
{
	return (logs::level) GetValue(GUI::l_level).toUInt();
}

bool gui_settings::GetGamelistColVisibility(int col)
{
	return GetValue(GUI_SAVE(GUI::game_list, "Col" + QString::number(col) + "visible", true)).toBool();
}

QStringList gui_settings::GetConfigEntries()
{
	QStringList nameFilter;
	nameFilter << "*.ini";
	QFileInfoList entries = settingsDir.entryInfoList(nameFilter, QDir::Files);
	QStringList res;
	for (QFileInfo entry : entries)
	{
		res.append(entry.baseName());
	}

	return res;
}

void gui_settings::BackupSettingsToTarget(const QString& friendlyName)
{	
	QSettings target(ComputeSettingsDir() + friendlyName + ".ini", QSettings::Format::IniFormat);
	QStringList keys = settings.allKeys();
	for (QStringList::iterator i = keys.begin(); i != keys.end(); i++)
	{
		if (!i->startsWith(GUI::meta))
		{
			target.setValue(*i, settings.value(*i));
		}
	}
	target.sync();
}

QStringList gui_settings::GetStylesheetEntries()
{
	QStringList nameFilter;
	nameFilter << "*.qss";
	QString path = settingsDir.absolutePath();
	QFileInfoList entries = settingsDir.entryInfoList(nameFilter, QDir::Files);
	QStringList res;
	for (QFileInfo entry : entries)
	{
		res.append(entry.baseName());
	}

	return res;
}

QString gui_settings::GetCurrentStylesheetPath()
{
	return settingsDir.absoluteFilePath(GetValue(GUI::m_currentStylesheet).toString() + ".qss");
}

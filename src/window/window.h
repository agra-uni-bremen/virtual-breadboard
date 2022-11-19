#pragma once

#include "central.h"

#include <QMainWindow>
#include <QLabel>
#include <QMenu>

class MainWindow : public QMainWindow {
	Q_OBJECT

	Central *m_central;

	QMenu *m_config;
	std::vector<QMenu*> m_json_dirs;
	QMenu *m_lua;

	QLabel *m_debug_label;
	QLabel *m_connection_label;

	void createDropdown();
	void saveJSON(const QString& file);
	void loadJsonDirEntries(const QString& dir);
	void addJsonDir(const QString& dir);
	void removeJsonDir(const QString& dir);

public:
	MainWindow(const std::string& additional_device_dir, const std::string& host, const std::string& port, bool overwrite_integrated_devices=false, QWidget *parent=0);
	~MainWindow();
	void loadJSON(const QString& configfile);
};

#pragma once

#include "central.h"

#include <QMainWindow>
#include <QLabel>
#include <QMenu>

class MainWindow : public QMainWindow {
	Q_OBJECT

	Central *central;

	QMenu *config;
	std::vector<QMenu*> json_dirs;
	QMenu *lua;

	QLabel *debug_label;
	QLabel *connection_label;

	void createDropdown();
	void saveJSON(QString file);
	void loadJsonDirEntries(QString dir);
	void addJsonDir(QString dir);
	void removeJsonDir(QString dir);

public:
	MainWindow(QString configfile, std::string additional_device_dir, const std::string host, const std::string port, bool overwrite_integrated_devices=false, QWidget *parent=0);
	~MainWindow();
};

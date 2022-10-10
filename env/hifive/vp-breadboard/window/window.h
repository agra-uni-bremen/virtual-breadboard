#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QMenu>

#include "central.h"

class MainWindow : public QMainWindow {
	Q_OBJECT

	Central *central;

	QMenu *config;
	std::vector<QMenu*> json_dirs;
	QMenu *devices_options;
	QMenu *devices;

	QLabel *debug_label;
	QLabel *connection_label;

	void createDropdown();

public:
	MainWindow(QString configfile, std::string additional_device_dir, const std::string host, const std::string port, bool overwrite_integrated_devices=false, QWidget *parent=0);
	~MainWindow();

public slots:
	void quit();

private slots:
	void toggleDebug();
	void connectionUpdate(bool active);
	void saveJSON(QString file);
	void loadJsonDirEntries(QString dir);
	void addJsonDir(QString dir);
	void removeJsonDir(QString dir);
	void loadDevices();
	void addLUA(QString dir);
	void overwriteLUA(QString dir);
};

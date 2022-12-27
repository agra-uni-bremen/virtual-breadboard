#include <window/window.h>

#include <QApplication>
#include <QDirIterator>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>

MainWindow::MainWindow(const std::string& additional_device_dir,
		const std::string& host, const std::string& port,
		bool overwrite_integrated_devices, QWidget *parent) : QMainWindow(parent) {
	setWindowTitle("MainWindow");

	m_central = new Central(host, port, this);
	m_central->loadLUA(additional_device_dir, overwrite_integrated_devices);
	setCentralWidget(m_central);
	connect(m_central, &Central::connectionUpdate, [this](bool active){
		m_connection_label->setText(active ? "Connected" : "Disconnected");
	});
	connect(m_central, &Central::sendStatus, this->statusBar(), &QStatusBar::showMessage);

	createDropdown();
}

MainWindow::~MainWindow() = default;

void MainWindow::loadJSON(const QString& configfile) {
	m_central->loadJSON(configfile);
}

void MainWindow::saveJSON(const QString& file) {
	statusBar()->showMessage("Saving breadboard status to config file " + file, 10000);
	m_central->saveJSON(file);
	QDir dir(file);
	dir.cdUp();
	loadJsonDirEntries(dir.absolutePath());
}

void MainWindow::removeJsonDir(const QString& dir) {
	auto dir_menu = std::find_if(m_json_dirs.begin(), m_json_dirs.end(),
								 [dir](QMenu* menu){return menu->title() == dir;});
	if(dir_menu == m_json_dirs.end()) return;
	statusBar()->showMessage("Remove JSON directory", 10000);
	QAction* json_dir_action = (*dir_menu)->menuAction();
	m_config->removeAction(json_dir_action);
	m_json_dirs.erase(dir_menu);
}

void MainWindow::loadJsonDirEntries(const QString& dir) {
	QString title = dir.startsWith(":") ? "Integrated" : dir;
	auto dir_menu = std::find_if(m_json_dirs.begin(), m_json_dirs.end(),
								 [title](QMenu* menu){ return menu->title() == title; });
	if(dir_menu == m_json_dirs.end()) return;
	QDirIterator it(dir, {"*.json"}, QDir::Files);
	if(!it.hasNext()) {
		removeJsonDir(dir);
		return;
	}
	for(QAction* action : (*dir_menu)->actions()) {
		if(action->text().endsWith(".json")) {
			(*dir_menu)->removeAction(action);
		}
	}
	while(it.hasNext()) {
		QString file = it.next();
		auto *cur = new QAction(file.right(file.size()-(dir.size() + 1)));
		connect(cur, &QAction::triggered, [this, file](){
			loadJSON(file);
		});
		(*dir_menu)->addAction(cur);
	}
}

void MainWindow::addJsonDir(const QString& dir) {
	if(std::find_if(m_json_dirs.begin(), m_json_dirs.end(),
					[dir](QMenu* menu){return menu->title() == dir;}) != m_json_dirs.end()) return;
	statusBar()->showMessage("Add JSON directory " + dir, 10000);
	auto dir_menu = new QMenu(dir.startsWith(":") ? "Integrated" : dir, m_config);
	m_json_dirs.push_back(dir_menu);
	m_config->addMenu(dir_menu);
	if(!dir.startsWith(":")) {
		auto *remove_dir = new QAction("Remove Directory");
		connect(remove_dir, &QAction::triggered, [this, dir](){
			removeJsonDir(dir);
		});
		dir_menu->addAction(remove_dir);
		auto *reload_dir = new QAction("Reload Directory");
		connect(reload_dir, &QAction::triggered, [this, dir](){
			loadJsonDirEntries(dir);
		});
		dir_menu->addAction(reload_dir);
		dir_menu->addSeparator();
	}
	loadJsonDirEntries(dir);
}

void MainWindow::createDropdown() {
	m_config = menuBar()->addMenu("Config");
	auto load_config_file = new QAction("Load JSON file");
	load_config_file->setShortcut(QKeySequence(QKeySequence::Open));
	connect(load_config_file, &QAction::triggered, [this](){
		QString path = QFileDialog::getOpenFileName(parentWidget(), "Select JSON file",
				QDir::currentPath(), "JSON files (*.json)");
		m_central->loadJSON(path);
	});
	m_config->addAction(load_config_file);
	auto save_config = new QAction("Save to JSON file");
	save_config->setShortcut(QKeySequence(QKeySequence::Save));
	connect(save_config, &QAction::triggered, [this](){
		QString path = QFileDialog::getSaveFileName(parentWidget(), "Select JSON file",
				QDir::currentPath(), "JSON files (*.json)");
		if(!path.endsWith(".json")) path.append(".json");
		saveJSON(path);
	});
	m_config->addAction(save_config);
	auto clear_breadboard = new QAction("Clear breadboard");
	clear_breadboard->setShortcut(QKeySequence(QKeySequence::Delete));
	connect(clear_breadboard, &QAction::triggered, m_central, &Central::clearBreadboard);
	m_config->addAction(clear_breadboard);
	auto load_config_dir = new QAction("Add JSON directory");
	load_config_dir->setShortcut(QKeySequence("CTRL+A"));
	connect(load_config_dir, &QAction::triggered, [this](){
		QString path = QFileDialog::getExistingDirectory(parentWidget(), "Open Directory",
				QDir::currentPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		addJsonDir(path);
	});
	m_config->addAction(load_config_dir);
	m_config->addSeparator();
	addJsonDir(":/conf");

	m_lua = menuBar()->addMenu("LUA");
	auto add_lua_dir = new QAction("Add directory");
	connect(add_lua_dir, &QAction::triggered, [this](){
		QString path = QFileDialog::getExistingDirectory(parentWidget(), "Open Directory",
						QDir::currentPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		m_central->loadLUA(path.toStdString(), false);
	});
	m_lua->addAction(add_lua_dir);
	auto overwrite_luas = new QAction("Add directory (Overwrite integrated)");
	connect(overwrite_luas, &QAction::triggered, [this](){
		QString path = QFileDialog::getExistingDirectory(parentWidget(), "Open Directory",
						QDir::currentPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		m_central->loadLUA(path.toStdString(), true);
	});
	m_lua->addAction(overwrite_luas);

	QMenu* window = menuBar()->addMenu("Window");
	auto debug = new QAction("Debug Mode");
	debug->setShortcut(QKeySequence(Qt::Key_Space));
	connect(debug, &QAction::triggered, [this](){
		m_debug_label->setText(m_central->toggleDebug() ? "Debug" : "");
	});
	window->addAction(debug);
	auto embedded_options = new QAction("GPIO Pins");
	connect(embedded_options, &QAction::triggered, m_central, &Central::openEmbeddedOptions);
	window->addAction(embedded_options);
	window->addSeparator();
	auto quit = new QAction("Quit");
	quit->setShortcut(QKeySequence(QKeySequence::Quit));
	connect(quit, &QAction::triggered, [this](){
		m_central->destroyConnection();
		QApplication::quit();
	});
	window->addAction(quit);

	m_debug_label = new QLabel();
	statusBar()->addPermanentWidget(m_debug_label);
	m_connection_label = new QLabel("Disconnected");
	statusBar()->addPermanentWidget(m_connection_label);
}

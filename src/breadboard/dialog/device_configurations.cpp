#include "device_configurations.h"

#include <QVBoxLayout>

DeviceConfigurations::DeviceConfigurations(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Device Configurations");

    connect(this, &QDialog::finished, this, &DeviceConfigurations::hide);

    m_tabs = new QTabWidget;
    m_conf = new ConfigDialog;
    connect(m_conf, &ConfigDialog::configChanged, this, &DeviceConfigurations::configChanged);
    m_tabs->addTab(m_conf, "Config");
    m_keys = new KeybindingDialog;
    connect(m_keys, &KeybindingDialog::keysChanged, this, &DeviceConfigurations::keysChanged);
    m_tabs->addTab(m_keys, "Keybinding");

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_tabs);
    setLayout(layout);
}

void DeviceConfigurations::setConfig(DeviceID device, const Config &config) {
    m_tabs->setTabVisible(0, true);
    m_conf->setConfig(device, config);
}

void DeviceConfigurations::hideConfig() {
    m_tabs->setTabVisible(0, false);
}

void DeviceConfigurations::setKeys(DeviceID device, const Keys& keys) {
    m_tabs->setTabVisible(1, true);
    m_keys->setKeys(device, keys);
}

void DeviceConfigurations::hideKeys() {
    m_tabs->setTabVisible(1, false);
}

void DeviceConfigurations::hide(int) {
    m_conf->removeConfig();
    QWidget::hide();
}

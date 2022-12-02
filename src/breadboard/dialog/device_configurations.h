#pragma once

#include "config.h"
#include "keybinding.h"

#include <QDialog>
#include <QTabWidget>

class DeviceConfigurations : public QDialog {
    Q_OBJECT

    QTabWidget *m_tabs;
    ConfigDialog *m_conf;
    KeybindingDialog *m_keys;

    void hide(int);

public:
    DeviceConfigurations(QWidget *parent);
    void setConfig(DeviceID device, const Config& config);
    void hideConfig();
    void setKeys(DeviceID device, const Keys& keys);
    void hideKeys();

signals:
    void keysChanged(DeviceID device_id, Keys keys);
    void configChanged(DeviceID device, Config config);
};
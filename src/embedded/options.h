#pragma once

#include "types.h"

#include <gpio-client.hpp>

#include <QDialog>
#include <QFormLayout>
#include <QCheckBox>

class PinOptions : public QDialog {
Q_OBJECT

    QFormLayout *m_layout;

    GPIOPinLayout m_pins_input;
    std::list<std::pair<gpio::PinNumber, IOF>> m_pins_output;

    void addPin(gpio::PinNumber global, const GPIOPin& pin);

public:
    PinOptions(QWidget *parent);
    void setPins(const GPIOPinLayout& pins);
    void removePins();

public slots:
    void accept() override;
    void reject() override;

signals:
    void pinsChanged(std::list<std::pair<gpio::PinNumber, IOF>> iofs);
};
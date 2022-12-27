#pragma once

#include <QWidget>
#include <QMap>

class Overlay : public QWidget {

    QMap<QPoint,QPoint> m_cables;

    void paintEvent(QPaintEvent*) override;

public:
    Overlay(QWidget* parent);
    void setCables(QMap<QPoint,QPoint> cables);
};

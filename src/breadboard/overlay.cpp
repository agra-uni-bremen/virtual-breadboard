#include "overlay.h"

#include <QPainter>
#include <utility>

Overlay::Overlay(QWidget *parent) : QWidget(parent) {
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setFocusPolicy(Qt::NoFocus);
}

void Overlay::setCables(QMap<QPoint,QPoint> cables) {
	this->m_cables = std::move(cables);
}

void Overlay::paintEvent(QPaintEvent *) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(QPen(Qt::red, 3));

	QMap<QPoint,QPoint>::const_iterator cable = m_cables.constBegin();
	while (cable != m_cables.constEnd()) {
		painter.drawLine(cable.key(), cable.value());
		++cable;
	}

	painter.end();
}

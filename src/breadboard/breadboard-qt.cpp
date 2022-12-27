#include "breadboard.h"

#include <QPainter>
#include <QMimeData>
#include <QDrag>
#include <QToolTip>

using namespace std;

/* User input */

void Breadboard::keyPressEvent(QKeyEvent *e) {
	if(!m_debugmode) {
		switch (e->key()) {
		case Qt::Key_0: {
			uint8_t until = 6;
			for (uint8_t i = 0; i < 8; i++) {
                m_embedded->setBit(i, i < until ? gpio::Tristate::HIGH : gpio::Tristate::LOW);
			}
			break;
		}
		case Qt::Key_1: {
			for (uint8_t i = 0; i < 8; i++) {
                m_embedded->setBit(i, gpio::Tristate::LOW);
			}
			break;
		}
		default:
			for(auto const& [id, device] : m_devices) {
				if(device->m_input) {
					Keys device_keys = device->m_input->getKeys();
					if(device_keys.find(e->key()) != device_keys.end()) {
						m_lua_access.lock();
						device->m_input->onKeypress(e->key(), true);
						m_lua_access.unlock();
						writeDevice(id);
					}
				}
			}
			break;
		}
		update();
	}
}

void Breadboard::keyReleaseEvent(QKeyEvent *e)
{
	if(!m_debugmode) {
		for(auto const& [id, device] : m_devices) {
			if(device->m_input) {
				Keys device_keys = device->m_input->getKeys();
				if(device_keys.find(e->key()) != device_keys.end()) {
					m_lua_access.lock();
					device->m_input->onKeypress(e->key(), false);
					m_lua_access.unlock();
					writeDevice(id);
				}
			}
		}
		update();
	}
}

void Breadboard::mousePressEvent(QMouseEvent *e) {
	for(auto const& [id, device] : m_devices) {
		QImage buffer = device->getBuffer();
		unsigned scale = device->getScale();
		if(!scale) scale = 1;
		QRect buffer_bounds = getDistortedGraphicBounds(buffer, scale);
		if(buffer_bounds.contains(e->pos()) && e->button() == Qt::LeftButton)  {
			if(m_debugmode) { // Move
				QPoint hotspot = e->pos() - buffer_bounds.topLeft();

				QByteArray itemData;
				QDataStream dataStream(&itemData, QIODevice::WriteOnly);
				dataStream << QString::fromStdString(id) << hotspot;

				buffer = buffer.scaled(buffer_bounds.size());

				auto *mimeData = new QMimeData;
				mimeData->setData(DRAG_TYPE_DEVICE, itemData);
				auto *drag = new QDrag(this);
				drag->setMimeData(mimeData);
				drag->setPixmap(QPixmap::fromImage(buffer));
				drag->setHotSpot(hotspot);

				drag->exec(Qt::MoveAction);
			}
			else { // Input
				if(device->m_input) {
					m_lua_access.lock();
					device->m_input->onClick(true);
					m_lua_access.unlock();
					writeDevice(id);
				}
			}
			return;
		}
	}
	update();
}

void Breadboard::mouseReleaseEvent(QMouseEvent *e) {
	for(auto const& [id, device] : m_devices) {
		if(e->button() == Qt::LeftButton) {
			if(!m_debugmode) {
				if(device->m_input) {
					m_lua_access.lock();
					device->m_input->onClick(false);
					m_lua_access.unlock();
					writeDevice(id);
				}
			}
		}
	}
	update();
}

void Breadboard::mouseMoveEvent(QMouseEvent *e) {
	bool device_hit = false;
	for(auto const& [id, device] : m_devices) {
		QRect device_bounds = getDistortedGraphicBounds(device->getBuffer(), device->getScale());
		if(device_bounds.contains(e->pos())) {
			QCursor current_cursor = cursor();
			current_cursor.setShape(Qt::PointingHandCursor);
			setCursor(current_cursor);
			string tooltip = "<b>"+device->getClass()+"</b><br><\br>"+id;
			QToolTip::showText(mapToGlobal(e->pos()), QString::fromStdString(tooltip), this, device_bounds);
			device_hit = true;
		}
	}
	if(!device_hit) {
		QCursor current_cursor = cursor();
		current_cursor.setShape(Qt::ArrowCursor);
		setCursor(current_cursor);
	}
}

/* Drag and Drop */

void Breadboard::dragMoveEvent(QDragMoveEvent *e) {
	if((e->mimeData()->hasFormat(DRAG_TYPE_DEVICE) || e->mimeData()->hasFormat(DRAG_TYPE_CABLE)) && (isBreadboard() ? isOnRaster(e->pos()) : true)) {
		e->acceptProposedAction();
	} else {
		e->ignore();
	}
}

void Breadboard::dragEnterEvent(QDragEnterEvent *e)  {
	if((e->mimeData()->hasFormat(DRAG_TYPE_DEVICE) || e->mimeData()->hasFormat(DRAG_TYPE_CABLE))) {
		e->acceptProposedAction();
	} else {
		e->ignore();
	}
}

void Breadboard::dropEvent(QDropEvent *e) {
    if(e->mimeData()->hasFormat(DRAG_TYPE_DEVICE)) {
        QByteArray itemData = e->mimeData()->data(DRAG_TYPE_DEVICE);
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QString q_id;
        QPoint hotspot;
        dataStream >> q_id >> hotspot;

        if(moveDevice(q_id.toStdString(), e->pos(), hotspot)) {
            e->acceptProposedAction();
        } else {
            e->ignore();
        }

        e->acceptProposedAction();
    }
    else if(e->mimeData()->hasFormat(DRAG_TYPE_CABLE)) {
        QByteArray itemData = e->mimeData()->data(DRAG_TYPE_CABLE);
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        quint8 q_pin;
        dataStream >> q_pin;
        auto pin = (gpio::PinNumber) q_pin;

        const auto [row, index] = getRasterPosition(e->pos());
        if(isValidRasterRow(row) && isValidRasterIndex(index)) {
            addPinToRow(row, index, pin, "cable");
            updateOverlay();
            e->acceptProposedAction();
        }
        else {
            cerr << "[Breadboard] Attempted to add cable in invalid position" << endl;
            e->ignore();
        }
    }
    else {
        cerr << "[Breadboard] New device position invalid: Invalid Mime data type." << endl;
        e->ignore();
    }
}

/* PAINT */

void Breadboard::resizeEvent(QResizeEvent*) {
	updateBackground();
    updateOverlay();
}

inline bool operator<(const QPoint &p1, const QPoint &p2)
{
    if(p1.x() != p2.x()) {
        return p1.x() < p2.x();
    }
    return p1.y() < p2.y();
}

void Breadboard::updateOverlay() {
    if(!isBreadboard()) return;
    QMap<QPoint,QPoint> qt_cables;
    for(const auto& [row, content] : m_raster) {
        for(const auto& pin : content.pins) {
            const QPoint pos_bb = this->mapToParent(getAbsolutePosition(row, pin.index)+
                    getDistortedPosition(QPoint(iconSizeMinimum()/2,iconSizeMinimum()/2)));
            const QPoint pos_embedded = m_embedded->mapToParent(m_embedded->getDistortedPositionPin(pin.global_pin)+
                    m_embedded->getDistortedPosition(QPoint(m_embedded->iconSizeMinimum()/2, m_embedded->iconSizeMinimum()/2)));
            qt_cables.insert(pos_bb, pos_embedded);
        }
    }
    m_overlay->setCables(qt_cables);
    m_overlay->update();
}

void Breadboard::paintEvent(QPaintEvent*) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	if(isBreadboard()) {
		painter.save();
		QColor dark("#101010");
		dark.setAlphaF(0.5);
		painter.setBrush(QBrush(dark));
		for(Row row=0; row<BB_ROWS; row++) {
			for(Index index=0; index<BB_INDEXES; index++) {
				painter.drawRect(QRect(getAbsolutePosition(row,index),
						getDistortedSize(QSize(iconSizeMinimum(),iconSizeMinimum()))));
			}
		}
        QColor red("red");
        red.setAlphaF(0.5);
        painter.setBrush(QBrush(red));
        for(const auto& [row, content] : m_raster) {
            for(const auto& pin : content.pins) {
                painter.drawRect(QRect(getAbsolutePosition(row, pin.index),
                                       getDistortedSize(QSize(iconSizeMinimum(), iconSizeMinimum()))));
            }
        }
		painter.restore();
	}

	if(m_debugmode) {
		QColor red("red");
		red.setAlphaF(0.5);
		painter.setBrush(QBrush(red));
	}

	// Graph Buffers
	for (auto& [id, device] : m_devices) {
		QImage buffer = device->getBuffer();
		QRect graphic_bounds = getDistortedGraphicBounds(buffer, device->getScale());
		painter.drawImage(graphic_bounds.topLeft(), buffer.scaled(graphic_bounds.size()));
		if(m_debugmode) {
			painter.drawRect(graphic_bounds);
		}
	}

	painter.end();
}

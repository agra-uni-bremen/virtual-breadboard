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
				emit(setBit(i, i < until ? gpio::Tristate::HIGH : gpio::Tristate::LOW));
			}
			break;
		}
		case Qt::Key_1: {
			for (uint8_t i = 0; i < 8; i++) {
				emit(setBit(i, gpio::Tristate::LOW));
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
				mimeData->setData(DEVICE_DRAG_TYPE, itemData);
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
	if(e->mimeData()->hasFormat(DEVICE_DRAG_TYPE) && (isBreadboard()?isOnRaster(e->pos()):true)) {
		e->acceptProposedAction();
	} else {
		e->ignore();
	}
}

void Breadboard::dragEnterEvent(QDragEnterEvent *e)  {
	if(e->mimeData()->hasFormat(DEVICE_DRAG_TYPE)) {
		e->acceptProposedAction();
	} else {
		e->ignore();
	}
}

void Breadboard::dropEvent(QDropEvent *e) {
	if(e->mimeData()->hasFormat(DEVICE_DRAG_TYPE)) {
		QByteArray itemData = e->mimeData()->data(DEVICE_DRAG_TYPE);
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);

		QString q_id;
		QPoint hotspot;
		dataStream >> q_id >> hotspot;

		if(moveDevice(m_devices.at(q_id.toStdString()).get(), e->pos(), hotspot)) {
			e->acceptProposedAction();
		} else {
			e->ignore();
		}

		e->acceptProposedAction();
	} else {
		cerr << "[Breadboard] New device position invalid: Invalid Mime data type." << endl;
		e->ignore();
	}
}

/* PAINT */

void Breadboard::resizeEvent(QResizeEvent*) {
	updateBackground();
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

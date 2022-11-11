#include "breadboard.h"
#include "raster.h"

#include <QPainter>
#include <QMimeData>
#include <QDrag>
#include <QToolTip>

using namespace std;

/* User input */

void Breadboard::keyPressEvent(QKeyEvent *e) {
	if(!debugmode) {
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
			for(auto const& [id, device] : devices) {
				if(device->input) {
					Keys device_keys = device->input->getKeys();
					if(device_keys.find(e->key()) != device_keys.end()) {
						lua_access.lock();
						device->input->onKeypress(e->key(), true);
						lua_access.unlock();
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
	if(!debugmode) {
		for(auto const& [id, device] : devices) {
			if(device->input) {
				Keys device_keys = device->input->getKeys();
				if(device_keys.find(e->key()) != device_keys.end()) {
					lua_access.lock();
					device->input->onKeypress(e->key(), false);
					lua_access.unlock();
					writeDevice(id);
				}
			}
		}
		update();
	}
}

void Breadboard::mousePressEvent(QMouseEvent *e) {
	for(auto const& [id, device] : devices) {
		if(device->graph && getGraphicBounds(device->graph->getBuffer(),
				device->graph->getScale()).contains(e->pos())) {
			if(e->button() == Qt::LeftButton)  {
				if(debugmode) { // Move
					QPoint hotspot = e->pos() - device->graph->getBuffer().offset();

					QByteArray itemData;
					QDataStream dataStream(&itemData, QIODevice::WriteOnly);
					dataStream << QString::fromStdString(id) << hotspot;

					QImage buffer = device->graph->getBuffer();
					unsigned scale = device->graph->getScale();
					if(!scale) scale = 1;
					buffer = buffer.scaled(buffer.width()*scale, buffer.height()*scale);

					QMimeData *mimeData = new QMimeData;
					mimeData->setData(DEVICE_DRAG_TYPE, itemData);
					QDrag *drag = new QDrag(this);
					drag->setMimeData(mimeData);
					drag->setPixmap(QPixmap::fromImage(buffer));
					drag->setHotSpot(hotspot);

					drag->exec(Qt::MoveAction);
				}
				else { // Input
					if(device->input) {
						lua_access.lock();
						device->input->onClick(true);
						lua_access.unlock();
						writeDevice(id);
					}
				}
				return;
			}
		}
	}
	update();
}

void Breadboard::mouseReleaseEvent(QMouseEvent *e) {
	for(auto const& [id, device] : devices) {
		if(e->button() == Qt::LeftButton) {
			if(!debugmode) {
				if(device->input) {
					lua_access.lock();
					device->input->onClick(false);
					lua_access.unlock();
					writeDevice(id);
				}
			}
		}
	}
	update();
}

void Breadboard::mouseMoveEvent(QMouseEvent *e) {
	bool device_hit = false;
	for(auto const& [id, device] : devices) {
		QRect device_bounds = getGraphicBounds(device->graph->getBuffer(), device->graph->getScale());
		if(device->graph && device_bounds.contains(e->pos())) {
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
	if(e->mimeData()->hasFormat(DEVICE_DRAG_TYPE) && (isBreadboard()?bb_isOnRaster(e->pos()):true)) {
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

		if(moveDevice(devices.at(q_id.toStdString()).get(), e->pos(), hotspot)) {
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
				QPoint top_left = bb_getAbsolutePosition(row, index);
				painter.drawRect(top_left.x(), top_left.y(), BB_ICON_SIZE, BB_ICON_SIZE);
			}
		}
		painter.restore();
	}

	if(debugmode) {
		QColor red("red");
		red.setAlphaF(0.5);
		painter.setBrush(QBrush(red));
	}

	// Graph Buffers
	for (auto& [id, device] : devices) {
		if(device->graph) {
			QImage buffer = device->graph->getBuffer();
			painter.drawImage(buffer.offset(), buffer.scaled(buffer.width()*device->graph->getScale(),
					buffer.height()*device->graph->getScale()));
			if(debugmode) {
				painter.drawRect(getGraphicBounds(buffer, device->graph->getScale()));
			}
		}
	}

	painter.end();
}

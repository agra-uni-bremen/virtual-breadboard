#include "breadboard.h"

#include <iostream>
#include <cmath>

/* Device */

Breadboard::Row Breadboard::getNewRowNumber() {
	Row row = 0;
	if(isBreadboard()) {
		std::cerr << "[Breadboard Raster] New row numbers can only be generated in custom view mode" << std::endl;
		return invalidRasterRow();
	}
	if(m_raster.size() < std::numeric_limits<Row>::max()) {
		row = m_raster.size();
	}
	else {
		std::set<unsigned> used_row_numbers;
		for(const auto& [known_row, content] : m_raster) {
			used_row_numbers.insert(known_row);
		}
		for(unsigned known_row : used_row_numbers) {
			if(known_row > row)
				break;
			row++;
		}
	}
	return row;
}

Breadboard::Row Breadboard::getRowForDevicePin(const DeviceID& device_id, Device::PIN_Interface::DevicePin device_pin) {
	Row row = invalidRasterRow();
	for(const auto& [device_pin_row, content] : m_raster) {
		auto exists = find_if(content.devices.begin(), content.devices.end(),
							  [device_id, device_pin](const DeviceConnection& content_obj){
								  return content_obj.id == device_id && content_obj.pin == device_pin;
							  });
		if(exists != content.devices.end()) {
			row = device_pin_row;
			break;
		}
	}
	return row;
}

DeviceRow Breadboard::getDeviceRow(QPoint pos_on_device) {
	return getMinimumPosition(pos_on_device).x() / iconSizeMinimum();
}

DeviceIndex Breadboard::getDeviceIndex(QPoint pos_on_device) {
	return getMinimumPosition(pos_on_device).y() / iconSizeMinimum();
}

std::pair<DeviceRow,DeviceIndex> Breadboard::getDeviceRasterPosition(QPoint pos_on_device) {
	return {getDeviceRow(pos_on_device), getDeviceIndex(pos_on_device)};
}

QPoint Breadboard::getDeviceAbsolutePosition(DeviceRow row, DeviceIndex index) {
	return getDistortedPosition(QPoint(row*iconSizeMinimum(),index*iconSizeMinimum()));
}

/* Breadboard */

Breadboard::Row Breadboard::invalidRasterRow() { return std::numeric_limits<Row>::max(); }
Breadboard::Index Breadboard::invalidRasterIndex() { return std::numeric_limits<Index>::max(); }

bool Breadboard::isValidRasterRow(Row row) {
	if(isBreadboard()) return row < BB_ROWS;
	return row < invalidRasterRow();
}
bool Breadboard::isValidRasterIndex(Index index) {
	if(isBreadboard()) return index < BB_INDEXES;
	return index < invalidRasterIndex();
}

Breadboard::Index Breadboard::getNextIndex(Row row) {
	auto row_content = m_raster.find(row);
	if((row_content == m_raster.end()) || (isBreadboard() && row_content->second.devices.size()+row_content->second.pins.size() >= BB_INDEXES)) {
		std::cerr << "[Breadboard] Could not find row " << row << std::endl;
		return invalidRasterIndex();
	}
	std::set<Index> indexes;
	for (const auto& pin : row_content->second.pins) {
		indexes.insert(pin.index);
	}
	Index index = 0;
	for(Index known_index : indexes) {
		if(known_index > index) break;
		index++;
	}
	return index;
	// TODO device position...
}

QRect Breadboard::getRasterBounds() {
	return getDistortedRect(BB_ROW_X,BB_ROW_Y,BB_ONE_ROW*iconSizeMinimum(),
							((BB_INDEXES*2)+1)*iconSizeMinimum());
}

bool Breadboard::isOnRaster(QPoint pos) {
	return getDistortedRect(BB_ROW_X,BB_ROW_Y,BB_ONE_ROW*iconSizeMinimum(),
							BB_INDEXES*iconSizeMinimum()).contains(pos)
		   || getDistortedRect(BB_ROW_X,BB_ROW_Y+(BB_INDEXES)*iconSizeMinimum(),
							   BB_ONE_ROW*iconSizeMinimum(),BB_INDEXES*iconSizeMinimum()).contains(pos);
}

Breadboard::Row Breadboard::getRow(QPoint pos) {
	if(!isOnRaster(pos)) {
		std::cerr << "[Breadboard Raster] Could not calculate row number: position is not on raster." << std::endl;
		return invalidRasterRow();
	}
	QPoint rel_pos_min = (getMinimumPosition(pos) - QPoint(BB_ROW_X, BB_ROW_Y));
	return (rel_pos_min.x() / iconSizeMinimum()) + (rel_pos_min.y() >= BB_INDEXES*iconSizeMinimum() ? BB_ONE_ROW : 0);
}

Breadboard::Index Breadboard::getIndex(QPoint pos) {
	if(!isOnRaster(pos)) {
		std::cerr << "[Breadboard Raster] Could not calculate index number: position is not on raster." << std::endl;
		return invalidRasterIndex();
	}
	Index index = (getMinimumPosition(pos) - QPoint(BB_ROW_X, BB_ROW_Y)).y()/iconSizeMinimum();
	return index<BB_INDEXES?index:index-BB_INDEXES-1;
}

std::pair<Breadboard::Row, Breadboard::Index> Breadboard::getRasterPosition(QPoint pos) {
	return {getRow(pos), getIndex(pos)};
}

QPoint Breadboard::getAbsolutePosition(Row row, Index index) {
	return getDistortedPosition(QPoint(BB_ROW_X+iconSizeMinimum()*(row%BB_ONE_ROW),
									   BB_ROW_Y+iconSizeMinimum()*index+(iconSizeMinimum()*6)*(row<BB_ONE_ROW?0:1)));
}

/* General Size / Position Calculations */

unsigned Breadboard::iconSizeMinimum() {
	return minimumWidth()/BB_ONE_ROW;
}

QRect Breadboard::getDistortedRect(unsigned x, unsigned y, unsigned width, unsigned height) {
	return {getDistortedPosition(QPoint(x,y)),
			getDistortedSize(QSize(width,height))};
}

QRect Breadboard::getDistortedGraphicBounds(const QImage& buffer, unsigned scale) {
	return getDistortedRect(buffer.offset().x(), buffer.offset().y(),
							buffer.width()*scale, buffer.height()*scale);
}

QPoint Breadboard::getDistortedPosition(QPoint pos) {
	return {(pos.x()*width())/minimumWidth(), (pos.y()*height())/minimumHeight()};
}

QSize Breadboard::getDistortedSize(QSize minimum) {
	return {(minimum.width()*width())/minimumWidth(), (minimum.height()*height())/minimumHeight()};
}

QPoint Breadboard::getMinimumPosition(QPoint pos) {
	return {(int)std::ceil((pos.x()*minimumWidth())/(float)width()),
			(int)std::ceil((pos.y()*minimumHeight())/(float)height())};
}

#include "breadboard.h"

#include <iostream>
#include <cmath>

/* Device */

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

QRect Breadboard::getRasterBounds() {
    return getDistortedRect(BB_ROW_X,BB_ROW_Y,BB_ONE_ROW*iconSizeMinimum(),
                            ((BB_INDEXES*2)+1)*iconSizeMinimum());
}

bool Breadboard::isOnRaster(QPoint pos) {
    return getDistortedRect(BB_ROW_X,BB_ROW_Y,BB_ONE_ROW*iconSizeMinimum(),
                            BB_INDEXES*iconSizeMinimum()).contains(pos)
           || getDistortedRect(BB_ROW_X,BB_ROW_Y+(BB_INDEXES+1)*iconSizeMinimum(),
                               BB_ONE_ROW*iconSizeMinimum(),BB_INDEXES*iconSizeMinimum()).contains(pos);
}

Breadboard::Row Breadboard::getRow(QPoint pos) {
    if(!isOnRaster(pos)) {
        std::cerr << "[Breadboard Raster] Could not calculate row number: position is not on raster." << std::endl;
        return BB_ROWS;
    }
    QPoint rel_pos_min = (getMinimumPosition(pos) - QPoint(BB_ROW_X, BB_ROW_Y));
    return (rel_pos_min.x() / iconSizeMinimum()) + (rel_pos_min.y() >= BB_INDEXES*iconSizeMinimum() ? BB_ONE_ROW : 0);
}

Breadboard::Index Breadboard::getIndex(QPoint pos) {
    if(!isOnRaster(pos)) {
        std::cerr << "[Breadboard Raster] Could not calculate index number: position is not on raster." << std::endl;
        return BB_INDEXES;
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

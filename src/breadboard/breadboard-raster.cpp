#include "breadboard.h"

#include <iostream>

/* Device */

DeviceRow Breadboard::getDeviceRow(QPoint pos) {
    return getMinimumPosition(pos).x() / iconSizeMinimum();
}

DeviceIndex Breadboard::getDeviceIndex(QPoint pos) {
    return getMinimumPosition(pos).y() / iconSizeMinimum();
}

std::pair<DeviceRow,DeviceIndex> Breadboard::getDeviceRasterPosition(QPoint pos) {
    return {getDeviceRow(pos),getDeviceIndex(pos)};
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

Row Breadboard::getRow(QPoint pos) {
    if(!isOnRaster(pos)) {
        std::cerr << "[Breadboard Raster] Could not calculate row number: position is not on raster." << std::endl;
        return BB_ROWS;
    }
    QPoint rel_pos_min = (getMinimumPosition(pos) - QPoint(BB_ROW_X, BB_ROW_Y));
    return (rel_pos_min.x() / iconSizeMinimum()) + (rel_pos_min.y() >= BB_INDEXES*iconSizeMinimum() ? BB_ONE_ROW : 0);
}

Index Breadboard::getIndex(QPoint pos) {
    if(!isOnRaster(pos)) {
        std::cerr << "[Breadboard Raster] Could not calculate index number: position is not on raster." << std::endl;
        return BB_INDEXES;
    }
    Index index = (getMinimumPosition(pos) - QPoint(BB_ROW_X, BB_ROW_Y)).y()/iconSizeMinimum();
    return index<BB_INDEXES?index:index-BB_INDEXES-1;
}

std::pair<Row, Index> Breadboard::getRasterPosition(QPoint pos) {
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
    unsigned pos_x = pos.x();
    unsigned pos_y = pos.y();
    unsigned minWidth = minimumWidth();
    int i_width = width();
    unsigned width = (unsigned)i_width;
    unsigned minHeight = minimumHeight();
    int i_height = height();
    unsigned height = (unsigned)i_height;
    // unsigned x, y, q. ceil(x/y) = q = (x+y-1)/y
    unsigned q_x = ((pos_x*minWidth)+width-1)/width;
    unsigned q_y = ((pos_y*minHeight)+height-1)/height;
    return {(int)q_x, (int)q_y};
}

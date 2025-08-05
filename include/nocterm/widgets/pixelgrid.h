/**
 * @file pixelgrid.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_PIXELGRID_H
#define NOCTERM_PIXELGRID_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_PIXELGRID(x) ((nocterm_pixelgrid_t*)x)

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct{
        bool printed;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
}nocterm_pixelgrid_cell_t[2];

typedef struct nocterm_pixelgrid_t{
    nocterm_widget_t widget;
    uint64_t pixel_height;
    uint64_t pixel_width;
    uint16_t cell_height;
    uint16_t cell_width;
    nocterm_pixelgrid_cell_t* cells;
}nocterm_pixelgrid_t;

/**
 * @brief Creates a new pixelgrid widget.
 * 
 * @param row 
 * @param col 
 * @param pixel_height 
 * @param pixel_width 
 * @return nocterm_pixelgrid_t* 
 */
nocterm_pixelgrid_t* nocterm_pixelgrid_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint32_t pixel_height, uint16_t pixel_width);

/**
 * @brief Constructs a pixelgrid widget.
 * 
 * @param pixelgrid 
 * @param row 
 * @param col 
 * @param pixel_height 
 * @param pixel_width 
 * @return int 
 */
int nocterm_pixelgrid_constructor(nocterm_pixelgrid_t* pixelgrid, nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint32_t pixel_height, uint16_t pixel_width);

/**
 * @brief Destructs a pixelgrid widget.
 * 
 * @param pixelgrid 
 * @return int 
 */
int nocterm_pixelgrid_destructor(nocterm_pixelgrid_t* pixelgrid);

/**
 * @brief Deletes a pixelgrid widget.
 * 
 * @param pixelgrid 
 * @return int 
 */
int nocterm_pixelgrid_delete(nocterm_pixelgrid_t* pixelgrid);

/**
 * @brief Prints a pixel at a specified location on a pixelgrid widget.
 * 
 * @param pixelgrid 
 * @param pixel_row 
 * @param pixel_col 
 * @param red 
 * @param green 
 * @param blue 
 * @return int 
 */
int nocterm_pixelgrid_print(nocterm_pixelgrid_t* pixelgrid, uint32_t pixel_row, uint16_t pixel_col, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Erases a pixel at a specified location on a pixelgrid widget.
 * 
 * @param pixelgrid 
 * @param pixel_row 
 * @param pixel_col 
 * @return int 
 */
int nocterm_pixelgrid_erase(nocterm_pixelgrid_t* pixelgrid, uint32_t pixel_row, uint16_t pixel_col);

/**
 * @brief Clears all pixels on a pixelgrid widget.
 * 
 * @param pixelgrid 
 * @return int 
 */
int nocterm_pixelgrid_clear(nocterm_pixelgrid_t* pixelgrid);

#ifdef __cplusplus
    }
#endif

#endif

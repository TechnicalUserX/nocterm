#include <nocterm/widgets/pixelgrid.h>

nocterm_pixelgrid_t *nocterm_pixelgrid_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint32_t pixel_height, uint16_t pixel_width){

    nocterm_pixelgrid_t *new_pixelgrid = (nocterm_pixelgrid_t *)malloc(sizeof(nocterm_pixelgrid_t));

    if(new_pixelgrid == NULL){
        return NULL;
    }

    memset(new_pixelgrid, 0x0, sizeof(nocterm_pixelgrid_t));

    if(nocterm_pixelgrid_constructor(new_pixelgrid, row, col, pixel_height, pixel_width) == NOCTERM_FAILURE){
        free(new_pixelgrid);
        return NULL;
    }

    return new_pixelgrid;
}

int nocterm_pixelgrid_constructor(nocterm_pixelgrid_t *pixelgrid, nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint32_t pixel_height, uint16_t pixel_width){


    if(pixelgrid == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_dimension_size_t cell_height = (nocterm_dimension_size_t)(((pixel_height % 2 == 0) ? pixel_height : pixel_height + 1) / 2);
    nocterm_dimension_size_t cell_width = (nocterm_dimension_size_t)pixel_width;

    pixelgrid->cells = (nocterm_pixelgrid_cell_t *)malloc(sizeof(nocterm_pixelgrid_cell_t) * cell_height * cell_width);
    
    if(pixelgrid->cells == NULL){
        free(pixelgrid);
        return NOCTERM_FAILURE;
    }

    memset(pixelgrid->cells, 0x0, sizeof(nocterm_pixelgrid_cell_t) * cell_height * cell_width);

    pixelgrid->pixel_height = pixel_height;
    pixelgrid->pixel_width = pixel_width;
    pixelgrid->cell_height = cell_height;
    pixelgrid->cell_width = cell_width;

    if(nocterm_widget_constructor(NOCTERM_WIDGET(pixelgrid), (nocterm_dimension_t){row, col, cell_height, cell_width}, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_pixelgrid_destructor(nocterm_pixelgrid_t *pixelgrid){
    if(pixelgrid == NULL){
        return NOCTERM_SUCCESS;
    }

    free(pixelgrid->cells);

    return NOCTERM_SUCCESS;
}

int nocterm_pixelgrid_delete(nocterm_pixelgrid_t *pixelgrid){

    if(nocterm_pixelgrid_destructor(pixelgrid) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(pixelgrid)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(pixelgrid);

    return NOCTERM_SUCCESS;
}

int nocterm_pixelgrid_print(nocterm_pixelgrid_t *pixelgrid, uint32_t pixel_row, uint16_t pixel_col, uint8_t red, uint8_t green, uint8_t blue){

    if(pixelgrid == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(pixel_row >= pixelgrid->pixel_height || pixel_col >= pixelgrid->pixel_width){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    uint16_t cell_row = pixel_row / 2;
    uint16_t cell_col = pixel_col;
    uint16_t cell_inner_position = pixel_row % 2;
    uint16_t cell_inner_position_other = (cell_inner_position + 1) % 2;

    nocterm_attribute_t attr = {
        .color.rgb.fg = true,
        .color.rgb.codes.fg = {red, green, blue},
    };

    if(pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position_other].printed){
        // Other pixel is also printed, we must retrieve its codes
        attr.color.rgb.bg = true;
        attr.color.rgb.codes.bg.red = pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position_other].red;
        attr.color.rgb.codes.bg.green = pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position_other].green;
        attr.color.rgb.codes.bg.blue = pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position_other].blue;
    }

    if(!cell_inner_position){
        // Upper
        nocterm_char_t pixel_up = {
            .bytes_size = 3,
            .is_utf8 = true};
        memcpy(pixel_up.bytes, "▀", 3);

        nocterm_widget_update(NOCTERM_WIDGET(pixelgrid), cell_row, cell_col, pixel_up, attr);
    }else{
        // Lower
        nocterm_char_t pixel_down = {
            .bytes_size = 3,
            .is_utf8 = true};
        memcpy(pixel_down.bytes, "▄", 3);

        nocterm_widget_update(NOCTERM_WIDGET(pixelgrid), cell_row, cell_col, pixel_down, attr);
    }

    pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position].printed = true;
    pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position].red = red;
    pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position].green = green;
    pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position].blue = blue;

    return NOCTERM_SUCCESS;
}

int nocterm_pixelgrid_erase(nocterm_pixelgrid_t *pixelgrid, uint32_t pixel_row, uint16_t pixel_col){


    if(pixelgrid == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(pixel_row >= pixelgrid->pixel_height || pixel_col >= pixelgrid->pixel_width){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    uint16_t cell_row = pixel_row / 2;
    uint16_t cell_col = pixel_col;
    uint16_t cell_inner_position = pixel_row % 2;
    uint16_t cell_inner_position_other = (cell_inner_position + 1) % 2;

    if(pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position].printed == false){
        return NOCTERM_SUCCESS;
    }

    nocterm_attribute_t attr = {0};

    bool other_exists = false;

    if(pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position_other].printed){
        // Other pixel is also printed, we must retrieve its codes
        attr.color.rgb.fg = true;
        attr.color.rgb.codes.fg.red = pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position_other].red;
        attr.color.rgb.codes.fg.green = pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position_other].green;
        attr.color.rgb.codes.fg.blue = pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position_other].blue;
        other_exists = true;
    }

    if(other_exists){

        if(!cell_inner_position_other){
            // Other is upper
            nocterm_char_t pixel_up = {
                .bytes_size = 3,
                .is_utf8 = true};
            memcpy(pixel_up.bytes, "▀", 3);

            nocterm_widget_update(NOCTERM_WIDGET(pixelgrid), cell_row, cell_col, pixel_up, attr);
        }else{
            // Other is lower
            nocterm_char_t pixel_down = {
                .bytes_size = 3,
                .is_utf8 = true};
            memcpy(pixel_down.bytes, "▄", 3);

            nocterm_widget_update(NOCTERM_WIDGET(pixelgrid), cell_row, cell_col, pixel_down, attr);
        }
    }else{
        // Clear that cell
        nocterm_widget_update(NOCTERM_WIDGET(pixelgrid), cell_row, cell_col, NOCTERM_CHAR_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);
    }

    pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position].printed = false;
    pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position].red = 0;
    pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position].green = 0;
    pixelgrid->cells[pixelgrid->cell_width * cell_row + cell_col][cell_inner_position].blue = 0;

    return NOCTERM_SUCCESS;
}

int nocterm_pixelgrid_clear(nocterm_pixelgrid_t* pixelgrid){

    if(pixelgrid == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_widget_clear(NOCTERM_WIDGET(pixelgrid));

    memset(pixelgrid->cells, 0x0, sizeof(nocterm_pixelgrid_cell_t) * pixelgrid->cell_height * pixelgrid->cell_width);

    return NOCTERM_SUCCESS;
}

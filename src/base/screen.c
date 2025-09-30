#include <nocterm/base/screen.h>

nocterm_dimension_size_t nocterm_screen_height = 0;
nocterm_dimension_size_t nocterm_screen_width = 0;

NOCTERM_INTERNAL nocterm_screen_ownership_t* nocterm_screen_ownership = NULL;

NOCTERM_INTERNAL void nocterm_screen_ownership_reset(void){
    if(nocterm_screen_ownership){
        memset(nocterm_screen_ownership, 0x0, sizeof(nocterm_screen_ownership_t) * nocterm_screen_height * nocterm_screen_width);
    }
}
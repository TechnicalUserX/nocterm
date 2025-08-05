#include <nocterm/base/attribute.h>

int nocterm_attribute_set(nocterm_attribute_t attribute){
    // Requires write()
     
    char attribute_buffer[NOCTERM_ATTRIBUTE_BUFFER_SIZE] = {0};

    // Size matters in 32-bit systems, this is 4 bytes; in 64-bit, this is 8 bytes
    // You are playing with fire!!!
    size_t remaining = NOCTERM_ATTRIBUTE_BUFFER_SIZE - 1;

    if(attribute.clear){
        size_t literal_size = strlen(NOCTERM_ATTRIBUTE_CLEAR);
        if(remaining >= literal_size){
            strncat(attribute_buffer,NOCTERM_ATTRIBUTE_CLEAR, remaining);
            remaining -= literal_size;        
        }else{
            errno = ENOMEM;
            return NOCTERM_FAILURE;
        }
    }

    if(attribute.underline){
        size_t literal_size = strlen(NOCTERM_ATTRIBUTE_UNDERLINE);
        if(remaining >= literal_size){
            strncat(attribute_buffer,NOCTERM_ATTRIBUTE_UNDERLINE, remaining);
            remaining -= literal_size;            
        }else{
            errno = ENOMEM;
            return NOCTERM_FAILURE;
        }
    }

    if(attribute.bold){
        size_t literal_size = strlen(NOCTERM_ATTRIBUTE_BOLD);
        if(remaining >= literal_size){
            strncat(attribute_buffer,NOCTERM_ATTRIBUTE_BOLD, remaining);
            remaining -= literal_size;            
        }else{
            errno = ENOMEM;
            return NOCTERM_FAILURE;
        }
    }

    if(attribute.italic){
        size_t literal_size = strlen(NOCTERM_ATTRIBUTE_ITALIC);
        if(remaining >= literal_size){
            strncat(attribute_buffer,NOCTERM_ATTRIBUTE_ITALIC, remaining);
            remaining -= literal_size;            
        }else{
            errno = ENOMEM;
            return NOCTERM_FAILURE;
        }
    }

    if(attribute.inverse){
        size_t literal_size = strlen(NOCTERM_ATTRIBUTE_INVERSE);
        if(remaining >= literal_size){
            strncat(attribute_buffer,NOCTERM_ATTRIBUTE_INVERSE, remaining);
            remaining -= literal_size;            
        }else{
            errno = ENOMEM;
            return NOCTERM_FAILURE;
        }
    }

    if(attribute.strikethrough){
        size_t literal_size = strlen(NOCTERM_ATTRIBUTE_STRIKETHROUGH);
        if(remaining >= literal_size){
            strncat(attribute_buffer, NOCTERM_ATTRIBUTE_STRIKETHROUGH, remaining);
            remaining -= literal_size;            
        }else{
            errno = ENOMEM;
            return NOCTERM_FAILURE;
        }
    }

    if(attribute.color.ansi.fg || attribute.color.ansi.bg){ // Classic ANSI or 256 color mode

        if(attribute.color.ansi.fg){
            char temp_buffer[128] = {0};
            int written = snprintf(temp_buffer, sizeof(temp_buffer),"\033[3%"  PRIu8 "m", attribute.color.ansi.codes.fg);
            if(remaining >= written){
                strncat(attribute_buffer,temp_buffer,written);
            }else{
                errno = ENOMEM;
                return NOCTERM_FAILURE;
            }                
        }

        if(attribute.color.ansi.bg){
            char temp_buffer[128] = {0};
            int written = snprintf(temp_buffer, sizeof(temp_buffer),"\033[4%"  PRIu8 "m", attribute.color.ansi.codes.bg);
            if(remaining >= written){
                strncat(attribute_buffer,temp_buffer,written);
            }else{
                errno = ENOMEM;
                return NOCTERM_FAILURE;
            }                

        }

    }else if(attribute.color.c256.fg || attribute.color.c256.bg){

    
        if(attribute.color.c256.fg){
            char temp_buffer[128] = {0};
            int written = snprintf(temp_buffer, sizeof(temp_buffer),"\033[38;5;%"  PRIu8 "m", attribute.color.c256.codes.fg);
            if(remaining >= written){
                strncat(attribute_buffer,temp_buffer,written);
            }else{
                errno = ENOMEM;
                return NOCTERM_FAILURE;
            }   
        }

        if(attribute.color.c256.bg){
            char temp_buffer[128] = {0};
            int written = snprintf(temp_buffer, sizeof(temp_buffer),"\033[48;5;%"  PRIu8 "m", attribute.color.c256.codes.bg);
            if(remaining >= written){
                strncat(attribute_buffer,temp_buffer,written);
            }else{
                errno = ENOMEM;
                return NOCTERM_FAILURE;
            }
        }


    }else{ // Truecolor attempt

        if(attribute.color.rgb.fg){
            char temp_buffer[128] = {0};
            // #define NOCTERM_ATTRIBUTE_RGB(r,g,b)     "\033[38;2;"#r";"#g";"#b"m"
            int written = snprintf(temp_buffer, sizeof(temp_buffer), "\033[38;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8 "m", attribute.color.rgb.codes.fg.red, attribute.color.rgb.codes.fg.green, attribute.color.rgb.codes.fg.blue);
            if(remaining >= written){
                strncat(attribute_buffer,temp_buffer,written);
            }else{
                errno = ENOMEM;
                return NOCTERM_FAILURE;
            }
        }

        if(attribute.color.rgb.bg){
            char temp_buffer[128] = {0};
            // #define NOCTERM_ATTRIBUTE_RGB(r,g,b)     "\033[38;2;"#r";"#g";"#b"m"
            int written = snprintf(temp_buffer, sizeof(temp_buffer), "\033[48;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8 "m", attribute.color.rgb.codes.bg.red, attribute.color.rgb.codes.bg.green, attribute.color.rgb.codes.bg.blue);
            if(remaining >= written){
                strncat(attribute_buffer,temp_buffer,written);
            }else{
                errno = ENOMEM;
                return NOCTERM_FAILURE;
            }
        }

    }

    // Last character is guaranteed to be zero, so strlen() is safe
    return nocterm_io_write(attribute_buffer, strlen(attribute_buffer));
}

int nocterm_attribute_clear(void){
    nocterm_attribute_t attr = {.clear = true};
    return nocterm_attribute_set(attr);
}

#include <nocterm/common/nocterm.h>
#include <nocterm/base/io.h>
#include <nocterm/base/key.h>
#include <nocterm/base/signal.h>


int nocterm_key_capture(nocterm_key_t* k){
    // Not thread-safe

    if(k == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_key_t k_temp = {0};
    memset(&k_temp, 0x0, sizeof(k_temp));

    uint8_t initial_byte = 0x00;

    if(nocterm_io_read(&initial_byte, 1) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if((initial_byte & 0b11111000) == 0b11110000){
        // 1111 0XXX
        // 4 Byte UTF-8 Character
        k_temp.type = NOCTERM_KEY_TYPE_PRINTABLE;
        k_temp.buffer_length = 4;
        k_temp.buffer[0] = initial_byte;

        for(uint8_t i = 1; i <= 3; i++){
            uint8_t temp_byte = 0x00;
            if (nocterm_io_read(&temp_byte, 1) == NOCTERM_FAILURE){
                errno = EIO;
                return NOCTERM_FAILURE;
            }
            if((temp_byte & 0b11000000) != 0b10000000){
                // It is not a continuation byte
                errno = EAGAIN;
                return NOCTERM_FAILURE;
            }
            k_temp.buffer[i] = temp_byte;
        }
        *k = k_temp;
        return NOCTERM_SUCCESS;        
    }else if((initial_byte & 0b11110000) == 0b11100000){
        // 1110 XXXX
        // 3 Byte UTF-8 Character
        k_temp.type = NOCTERM_KEY_TYPE_PRINTABLE;
        k_temp.buffer_length = 3;
        k_temp.buffer[0] = initial_byte;
        for(uint8_t i = 1; i <= 2; i++){
            uint8_t temp_byte = 0x00;
            if (nocterm_io_read(&temp_byte, 1) == NOCTERM_FAILURE){
                errno = EIO;
                return NOCTERM_FAILURE;
            }
            if((temp_byte & 0b11000000) != 0b10000000){
                // It is not a continuation byte
                errno = EAGAIN;
                return NOCTERM_FAILURE;
            }
            k_temp.buffer[i] = temp_byte;
        }
        *k = k_temp;
        return NOCTERM_SUCCESS;  
    }else if((initial_byte & 0b11100000) == 0b11000000){
        // 110X XXXX
        // 2 Byte UTF-8 Character
        k_temp.type = NOCTERM_KEY_TYPE_PRINTABLE;
        k_temp.buffer_length = 2;
        k_temp.buffer[0] = initial_byte;
        uint8_t temp_byte = 0x00;
        if (nocterm_io_read(&temp_byte, 1) == NOCTERM_FAILURE){
            errno = EIO;
            return NOCTERM_FAILURE;
        }
        if((temp_byte & 0b11000000) != 0b10000000){
            // It is not a continuation byte
            errno = EAGAIN;
            return NOCTERM_FAILURE;
        }
        k_temp.buffer[1] = temp_byte;
        *k = k_temp;
        return NOCTERM_SUCCESS;
    }else if((initial_byte & 0b11000000) == 0b10000000){
        // 10XX XXXXX
        // Invalid first byte
        errno = EAGAIN;
        return NOCTERM_FAILURE;
    }

    if(initial_byte > 31 && initial_byte != 127){
        // Plain old ASCII 7-bit
        k_temp.buffer[0] = initial_byte;
        k_temp.buffer_length = 1;
        k_temp.type = NOCTERM_KEY_TYPE_PRINTABLE;
        *k = k_temp;
        return NOCTERM_SUCCESS;
    }

    // Control character but not ESC
    if(initial_byte != '\e'){
        k_temp.buffer[0] = initial_byte;
        k_temp.buffer_length = 1;
        k_temp.type = NOCTERM_KEY_TYPE_CONTROL;
        *k = k_temp;
        return NOCTERM_SUCCESS;
    }

    // By default, it is 1, because we already read the initial byte
    uint64_t byte_sequence_size = 1; 

    while(1){
        bool available = false;
        struct timeval tv = {NOCTERM_KEY_ESCSEQ_INTERVAL/1000, NOCTERM_KEY_ESCSEQ_INTERVAL};
        if(nocterm_io_read_available(&available, tv) == NOCTERM_FAILURE){
            errno = EIO;
            return NOCTERM_FAILURE;
        }

        if(available == false){
            break;
        }

        if(byte_sequence_size == NOCTERM_KEY_BUFFER_SIZE){
            errno = ENOMEM;
            return NOCTERM_FAILURE;
        }

        char subsequent_byte = 0;

        if(nocterm_io_read(&subsequent_byte, 1) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
        
        k_temp.buffer[byte_sequence_size] = subsequent_byte;
        byte_sequence_size++;
    }

    k_temp.buffer[0] = initial_byte;

    if(byte_sequence_size == 1){
        // Just ESC character
        k_temp.buffer_length = 1;
        k_temp.type = NOCTERM_KEY_TYPE_CONTROL;
        *k = k_temp;
        return NOCTERM_SUCCESS;
    }else{
        // Complete escape sequence
        k_temp.buffer_length = byte_sequence_size;
        k_temp.type = NOCTERM_KEY_TYPE_ESCSEQ;
        *k = k_temp;
        return NOCTERM_SUCCESS;
    }
}

nocterm_key_event_t nocterm_key_translate(nocterm_key_t* key){

    if(key == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(key->type == NOCTERM_KEY_TYPE_PRINTABLE){
        return NOCTERM_KEY_EVENT_PRINTABLE;
    }

    if(key->type == NOCTERM_KEY_TYPE_CONTROL){
        switch(key->buffer[0]){
            case '\t':
                return NOCTERM_KEY_EVENT_TAB;
            case '\n':
            case '\r':
                return NOCTERM_KEY_EVENT_ENTER;
            case 127:
                return NOCTERM_KEY_EVENT_BACKSPACE;
            case 27:
                return NOCTERM_KEY_EVENT_ESCAPE;
            case 8:
                return NOCTERM_KEY_EVENT_BS; // Ctrl + H (ASCII Backspace)
            case 12:
                return NOCTERM_KEY_EVENT_FF; // Ctrl + L (ASCII Formfeed)
            case 3:
                return NOCTERM_KEY_EVENT_BREAK; // Ctrl + C
            case 4:
                return NOCTERM_KEY_EVENT_EOF; // Ctrl + D (ASCII End of Transmission)
        }
    }

    if(key->type == NOCTERM_KEY_TYPE_ESCSEQ){

        if(memcmp(key->buffer,"\033[A", 3) == 0){
            return NOCTERM_KEY_EVENT_UP;
        }else if(memcmp(key->buffer,"\033[B", 3) == 0){
            return NOCTERM_KEY_EVENT_DOWN;
        }else if(memcmp(key->buffer,"\033[C", 3) == 0){
            return NOCTERM_KEY_EVENT_RIGHT;
        }else if(memcmp(key->buffer,"\033[D", 3) == 0){
            return NOCTERM_KEY_EVENT_LEFT;
        }else if(memcmp(key->buffer,"\033[3~", 4) == 0){
            return NOCTERM_KEY_EVENT_DELETE;
        }else if(memcmp(key->buffer,"\033[Z", 4) == 0){
            return NOCTERM_KEY_EVENT_SHIFT_TAB;
        }

    }

    return NOCTERM_KEY_EVENT_UNDEFINED;
}

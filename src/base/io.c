#include <nocterm/base/io.h>
#include <nocterm/base/char.h>

pthread_mutex_t nocterm_io_stdin_lock;

pthread_mutex_t nocterm_io_stdout_lock;

int nocterm_io_init(void){

    if(pthread_mutex_init(&nocterm_io_stdin_lock, NULL) != 0){
        return NOCTERM_FAILURE;
    }

    if(pthread_mutex_init(&nocterm_io_stdout_lock, NULL) != 0){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_io_end(void){
    if(pthread_mutex_destroy(&nocterm_io_stdin_lock) != 0){
        return NOCTERM_FAILURE;
    }

    if(pthread_mutex_destroy(&nocterm_io_stdout_lock) != 0){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_io_print_at(nocterm_dimension_size_t row, nocterm_dimension_size_t col, const char* format, ...){
    if(format == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    char buffer[NOCTERM_IO_BUFFER_SIZE] = {0};

    va_list nocterm_io_print_va_list;
    va_start(nocterm_io_print_va_list, format);
    int written = vsnprintf(buffer, NOCTERM_IO_BUFFER_SIZE, format, nocterm_io_print_va_list);
    va_end(nocterm_io_print_va_list);
    
    return nocterm_io_write_at(row, col, buffer, written);
}

int nocterm_io_print(const char* format, ...){
    if(format == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    char buffer[NOCTERM_IO_BUFFER_SIZE] = {0};

    va_list nocterm_io_print_va_list;
    va_start(nocterm_io_print_va_list, format);
    int written = vsnprintf(buffer, NOCTERM_IO_BUFFER_SIZE, format, nocterm_io_print_va_list);
    va_end(nocterm_io_print_va_list);

    return nocterm_io_write(buffer, written);
}

int nocterm_io_read_available(bool* available, struct timeval timeout){

    if(available == NULL){
        return NOCTERM_FAILURE;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    int buffer_available = select(1, &fds, NULL, NULL, &timeout);

    if (buffer_available > 0){
        *available = true;
        return NOCTERM_SUCCESS;
    }else if(buffer_available == 0){
        *available = false;
        return NOCTERM_SUCCESS;
    }else{
        errno = EIO;
        return NOCTERM_FAILURE;
    }

}

int nocterm_io_read(void* buffer, uint64_t buffer_size){

    if(buffer == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(buffer_size == 0){
        // If buffer_size is zero, function has no effect
        return NOCTERM_SUCCESS;
    }

    if(pthread_mutex_lock(&nocterm_io_stdin_lock) != 0){
        return NOCTERM_FAILURE;
    }

    uint64_t toread = buffer_size;
    uint64_t haveread = 0;
    uint8_t* buffer_casted = (uint8_t*)buffer;


    while(toread){
        ssize_t r = read(STDIN_FILENO, buffer_casted + (uint64_t)haveread, toread);
        if(r == -1){
            // Not always an error
            if(errno == EINTR){
                // Interrupt (signal) detected
                // Let the loop handle the signal
                continue;
            }else{
                errno = EIO;
                if(pthread_mutex_unlock(&nocterm_io_stdin_lock) != 0){
                    return NOCTERM_FAILURE;
                }
                return NOCTERM_FAILURE;
            }
        }else if(r == 0){
            // NOT YET IMPLEMENTED
            if(pthread_mutex_unlock(&nocterm_io_stdin_lock) != 0){
                return NOCTERM_FAILURE;
            }
            return NOCTERM_SUCCESS;
        }else{
            haveread += r;
            toread -= (uint64_t)r;
        }

    }

    if(pthread_mutex_unlock(&nocterm_io_stdin_lock) != 0){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_io_consume(void){

    while(1){
        bool available = false;
        struct timeval tv = {0, 0};
        if(nocterm_io_read_available(&available, tv) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
        if(available){
            char dummy = 0;
            if(nocterm_io_read(&dummy, sizeof(dummy)) == NOCTERM_FAILURE){
                return NOCTERM_FAILURE;
            }
        }else{
            break;
        }

    }

    return NOCTERM_SUCCESS;
}

int nocterm_io_write(void* buffer, uint64_t buffer_size){

    if(buffer == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(buffer_size == 0){
        // If buffer_size is zero, function has no effect
        return NOCTERM_SUCCESS;
    }

    if(pthread_mutex_lock(&nocterm_io_stdout_lock) != 0){
        return NOCTERM_FAILURE;
    }

    uint64_t towrite = buffer_size;
    uint64_t written = 0;
    uint8_t* buffer_casted = (uint8_t*)buffer;

    while(towrite){
        ssize_t r = write(STDOUT_FILENO, buffer_casted + (uint64_t)written, towrite);
        if(r == -1){
            errno = EIO;
            if(pthread_mutex_unlock(&nocterm_io_stdout_lock) != 0){
                return NOCTERM_FAILURE;
            }
            return NOCTERM_FAILURE;
        }else if(r == 0){
            // NOT YET IMPLEMENTED
            if(pthread_mutex_unlock(&nocterm_io_stdin_lock) != 0){
                return NOCTERM_FAILURE;
            }
            fsync(STDOUT_FILENO);
            return NOCTERM_SUCCESS;
        }else{
            written += r;
            towrite -= (uint64_t)r;
        }

    }

    if(pthread_mutex_unlock(&nocterm_io_stdout_lock) != 0){
        return NOCTERM_FAILURE;
    }
    fsync(STDOUT_FILENO);
    return NOCTERM_SUCCESS;
}

int nocterm_io_write_at(nocterm_dimension_size_t row, nocterm_dimension_size_t col, void* buffer, uint64_t buffer_size){

    row++; col++;
    char extended_buffer[NOCTERM_IO_BUFFER_SIZE] = {0};
    int towrite = snprintf(extended_buffer, NOCTERM_IO_BUFFER_SIZE, "\033[%" PRIu16 ";%" PRIu16 "H", row, col);

    if(towrite < 0){
        return NOCTERM_FAILURE;
    }

    if(buffer != NULL && buffer_size != 0){

        if((uint64_t)towrite + buffer_size >= NOCTERM_IO_BUFFER_SIZE){
            errno = ENOMEM;
            return NOCTERM_FAILURE;
        }

        memcpy(extended_buffer + towrite, buffer, buffer_size);
    }

    return nocterm_io_write(extended_buffer, towrite + buffer_size);
}

int nocterm_io_clear(void){
    return nocterm_io_write("\033[2J\033[3J", 8);
}

int nocterm_io_put_char(nocterm_char_t c){
    return nocterm_io_write(c.bytes,c.bytes_size);
}

int nocterm_io_put_char_at(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_char_t c){
    return nocterm_io_write_at(row, col, c.bytes, c.bytes_size);
}

int nocterm_io_erase_char(void){
    return nocterm_io_write(" ", 1);
}

int nocterm_io_erase_char_at(nocterm_dimension_size_t row, nocterm_dimension_size_t col){
    return nocterm_io_write_at(row,col," ", 1);
}

int nocterm_io_cursor_visible(bool visible){
    if(visible){
        return nocterm_io_write("\033[?25h",6);
    }else{
        return nocterm_io_write("\033[?25l",6);
    }
}

int nocterm_io_cursor_move(nocterm_dimension_size_t row, nocterm_dimension_size_t col){
    return nocterm_io_write_at(row, col, NULL, 0);    
}


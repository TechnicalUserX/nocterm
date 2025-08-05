#include <nocterm/base/char.h>
#include <stdio.h>

// Returns text in nocterm_char_t string format with null terminator
// Expects a large enough src that is null terminated
uint64_t nocterm_char_string_from_stream(nocterm_char_t* dest, uint64_t dest_size, const char* src, uint64_t src_size){
    
    if(dest == NULL){
        errno = EINVAL;
        return 0;
    }

    if(src_size == 0 || src == NULL){
        // No bytes copied, not an error
        return 0;
    }

    if(dest_size == 0){
        errno = ENOMEM;
        return 0;
    }

    uint64_t src_position = 0;
    uint64_t dest_remaining = dest_size;
    mbstate_t mbstate = {0};
    uint64_t dest_position = 0;
    uint64_t parsed_length = 0;

    while(dest_remaining > 0 && src_size > 0){

        size_t multibyte_size = mbrlen(&src[src_position], src_size, &mbstate);

        if(multibyte_size == -1){
            // Illegal sequence
            // Malformed src, no valid conversion available
            return 0;
        }

        if(multibyte_size == 0){
            // No multibyte character left, null terminator detected
            break;
        }

        nocterm_char_t temp = {0};
        memcpy(temp.bytes, &src[src_position], multibyte_size);
        temp.bytes_size = multibyte_size;

        if(multibyte_size > 1){
            temp.is_utf8 = true;
        }else{
            // There is no single byte UTF8 character like 10xxxxxx,
            // this pattern is invalid for first byte
            temp.is_utf8 = false;
        }

        memcpy(&dest[dest_position], &temp, sizeof(nocterm_char_t));

        src_position += (uint64_t)multibyte_size;
        dest_remaining--;
        src_size -= multibyte_size;
        dest_position++;
        parsed_length++;
    }

    if(dest_remaining == 0){
        errno = ENOMEM;
        return 0;
    }    

    dest[dest_position] = NOCTERM_CHAR_NULL; // Null terminator 

    return parsed_length; // Return successfull character length
}

// Returns a text in char array with null terminator
// Expects a large enough src that is null terminated
uint64_t nocterm_char_string_to_stream(char* dest, uint64_t dest_size, const nocterm_char_t* src, uint64_t src_size){
    
    if(dest == NULL){
        errno = EINVAL;
        return 0;
    }

    if(src_size == 0 || src == NULL){
        return 0;
    }

    if(dest_size == 0){
        errno = ENOMEM;
        return 0;
    }

    uint64_t dest_remaining = dest_size;
    uint64_t dest_position = 0;
    uint64_t parsed_length = 0;

    for(uint64_t i = 0; i < src_size; i++){

        if(dest_remaining < src[i].bytes_size){
            errno = ENOMEM;
            return 0;
        }

        if(nocterm_char_is_null(src[i])){
            // Null terminator detected
            break;
        }

        memcpy(&dest[dest_position],src[i].bytes, src[i].bytes_size);
        dest_position += src[i].bytes_size;
        dest_remaining -= src[i].bytes_size;
        parsed_length++;
    }

    if(dest_remaining < 1){
        errno = ENOMEM;
        return 0;
    }

    dest[dest_position] = '\0';

    return parsed_length;
}

bool nocterm_char_is_null(nocterm_char_t ch){
    if(memcmp(&ch, &NOCTERM_CHAR_NULL, sizeof(nocterm_char_t)) == 0){
        // Null terminator detected
        return true;
    }else{
        return false;
    }
}

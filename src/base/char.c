#include <nocterm/base/char.h>
#include <stdio.h>

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

        if(multibyte_size == (size_t)-1 || multibyte_size == (size_t)-2){
            // (size_t)-1: illegal sequence — malformed src, no valid conversion.
            // (size_t)-2: a valid but incomplete multibyte sequence (src ends
            // mid-codepoint). Either way there is no complete character to copy;
            // bailing here prevents using the huge (size_t)-N as a memcpy length.
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

    dest[dest_position] = nocterm_char_from_ascii('\0'); // Null terminator 

    return parsed_length; // Return successfull character length
}

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

nocterm_char_t nocterm_char_from_ascii(char ch){
    nocterm_char_t result = {0};
    result.bytes[0] = (uint8_t)ch;
    result.bytes_size = 1;
    result.is_utf8 = false;
    return result;
}

nocterm_char_t nocterm_char_from_wchar(wchar_t ch){
    nocterm_char_t result = {0};
    mbstate_t mbstate = {0};

    // Convert wchar_t to UTF-8
    char buffer[4] = {0};
    int bytes_written = wcrtomb(buffer, ch, &mbstate);
    if(bytes_written == -1){
        // Conversion failed, return empty char
        return NOCTERM_CHAR_EMPTY;
    }
    memcpy(result.bytes, buffer, bytes_written);
    result.bytes_size = (uint8_t)bytes_written;
    result.is_utf8 = (bytes_written > 1) ? true : false;
    return result;
}

bool nocterm_char_is_null(nocterm_char_t ch){
    nocterm_char_t null = nocterm_char_from_ascii('\0');

    if(null.bytes_size == ch.bytes_size && ch.is_utf8 == false && ch.bytes[0] == '\0'){
        return true;
    }else{
        return false;
    }
}

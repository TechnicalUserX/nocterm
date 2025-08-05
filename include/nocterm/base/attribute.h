/**
 * @file attribute.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_ATTRIBUTE_H
#define NOCTERM_ATTRIBUTE_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/io.h>

#define NOCTERM_ATTRIBUTE_EMPTY (nocterm_attribute_t){0}


#define NOCTERM_ATTRIBUTE_BUFFER_SIZE 512

#define NOCTERM_ATTRIBUTE_BOLD           "\033[1m"
#define NOCTERM_ATTRIBUTE_DIM            "\033[2m"
#define NOCTERM_ATTRIBUTE_ITALIC         "\033[3m"
#define NOCTERM_ATTRIBUTE_UNDERLINE      "\033[4m"
#define NOCTERM_ATTRIBUTE_BLINK          "\033[5m"
#define NOCTERM_ATTRIBUTE_FAST_BLINK     "\033[6m"
#define NOCTERM_ATTRIBUTE_INVERSE        "\033[7m"
#define NOCTERM_ATTRIBUTE_HIDDEN         "\033[8m"
#define NOCTERM_ATTRIBUTE_STRIKETHROUGH  "\033[9m"
#define NOCTERM_ATTRIBUTE_BRIGHT         NOCTERM_ATTRIBUTE_BOLD

#define NOCTERM_ATTRIBUTE_CLEAR          "\033[0m"

#define NOCTERM_ATTRIBUTE_BLACK          "\033[30m"
#define NOCTERM_ATTRIBUTE_RED            "\033[31m"
#define NOCTERM_ATTRIBUTE_GREEN          "\033[32m"
#define NOCTERM_ATTRIBUTE_YELLOW         "\033[33m"
#define NOCTERM_ATTRIBUTE_BLUE           "\033[34m"
#define NOCTERM_ATTRIBUTE_MAGENTA        "\033[35m"
#define NOCTERM_ATTRIBUTE_CYAN           "\033[36m"
#define NOCTERM_ATTRIBUTE_WHITE          "\033[37m"

#define NOCTERM_ATTRIBUTE_256COLOR(c)    "\033[38;5;"#c"m"
#define NOCTERM_ATTRIBUTE_256COLOR_BG(c) "\033[48;5;"#c"m"

#define NOCTERM_ATTRIBUTE_RGB(r,g,b)     "\033[38;2;"#r";"#g";"#b"m"
#define NOCTERM_ATTRIBUTE_RGB_BG(r,g,b)  "\033[48;2;"#r";"#g";"#b"m"

#define NOCTERM_ATTRIBUTE_BLACK_BG       "\033[40m"
#define NOCTERM_ATTRIBUTE_RED_BG         "\033[41m"
#define NOCTERM_ATTRIBUTE_GREEN_BG       "\033[42m"
#define NOCTERM_ATTRIBUTE_YELLOW_BG      "\033[43m"
#define NOCTERM_ATTRIBUTE_BLUE_BG        "\033[44m"
#define NOCTERM_ATTRIBUTE_MAGENTA_BG     "\033[45m"
#define NOCTERM_ATTRIBUTE_CYAN_BG        "\033[46m"
#define NOCTERM_ATTRIBUTE_WHITE_BG       "\033[47m"
#define NOCTERM_ATTRIBUTE_GRAY_BG        "\033[100m"
#define NOCTERM_ATTRIBUTE_LRED_BG        "\033[101m"
#define NOCTERM_ATTRIBUTE_LGREEN_BG      "\033[102m"
#define NOCTERM_ATTRIBUTE_LYELLOW_BG     "\033[103m"
#define NOCTERM_ATTRIBUTE_LBLUE_BG       "\033[104m"
#define NOCTERM_ATTRIBUTE_LPURPLE_BG     "\033[105m"
#define NOCTERM_ATTRIBUTE_TEAL_BG        "\033[106m"

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_attribute_color_rgb_codes_generic_t{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
}nocterm_attribute_color_rgb_codes_generic_t;

typedef struct nocterm_attribute_color_rgb_codes_t{

    nocterm_attribute_color_rgb_codes_generic_t fg;
    nocterm_attribute_color_rgb_codes_generic_t bg;

}nocterm_attribute_color_rgb_codes_t;

typedef struct nocterm_attribute_color_ansi_codes_t{
    uint8_t fg;
    uint8_t bg;
}nocterm_attribute_color_ansi_codes_t;

typedef struct nocterm_attribute_color_c256_codes_t{
    uint8_t fg;
    uint8_t bg;
}nocterm_attribute_color_c256_codes_t;

typedef struct nocterm_attribute_t{
    uint8_t underline:1;
    uint8_t bold:1;
    uint8_t italic:1;
    uint8_t inverse:1;
    uint8_t strikethrough:1;
    uint8_t clear:1;

    // First 16 are classic colors, remaining ones are 256 color mode
    // Check capability for 256 color (Lookup-table) mode
    struct nocterm_attribute_color_t{
        // Classic ANSI colors
        struct{
            uint8_t fg:1;
            uint8_t bg:1;
            nocterm_attribute_color_ansi_codes_t codes;
        }ansi; 
        // 256 Color
        struct{
            uint8_t fg:1;
            uint8_t bg:1;
            nocterm_attribute_color_c256_codes_t codes;          
        }c256; 
        // Truecolor
        struct{
            uint8_t fg:1;
            uint8_t bg:1;
            nocterm_attribute_color_rgb_codes_t codes;
        }rgb;
    }color;

}nocterm_attribute_t;


/**
 * @brief Sets attributes of the current terminal.
 * 
 * @param attribute Terminal formatting attribute
 * @return int
 */
int nocterm_attribute_set(nocterm_attribute_t attribute);

/**
 * @brief Clears the attributes of the terminal.
 * 
 * @return int 
 */
int nocterm_attribute_clear(void);

#ifdef __cplusplus
    }
#endif

#endif

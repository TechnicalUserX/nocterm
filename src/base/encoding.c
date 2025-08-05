#include <nocterm/base/encoding.h>

nocterm_encoding_t nocterm_encoding_get(void) {

    const char *codeset = nl_langinfo(CODESET);
    if (!codeset) {
        return NOCTERM_ENCODING_UNKNOWN;
    }

    if (strcasecmp(codeset, "UTF-8") == 0 || strcasecmp(codeset, "utf8") == 0) {
        return NOCTERM_ENCODING_UTF8;
    }

    if (strcmp(codeset, "ANSI_X3.4-1968") == 0) {
        return NOCTERM_ENCODING_ASCII;
    }

    return NOCTERM_ENCODING_UNKNOWN;
}


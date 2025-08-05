#include <nocterm/base/capability.h>

nocterm_capability_t nocterm_capability_get(void){

    const char *colorterm = getenv("COLORTERM");

    if(colorterm && (strcmp(colorterm, "truecolor") == 0 || strcmp(colorterm, "24bit") == 0)){
        return NOCTERM_CAPABILITY_TRUECOLOR;
    }

    const char *term = getenv("TERM");

    if(term == NULL){
        return NOCTERM_CAPABILITY_UNKNOWN;
    }else{
        if (strcmp(term, "xterm-256color") == 0){
            return NOCTERM_CAPABILITY_XTERM256;
        }else if (strcmp(term, "xterm") == 0){
            return NOCTERM_CAPABILITY_XTERM;
        }else{
            return NOCTERM_CAPABILITY_UNKNOWN;
        }
    }
}
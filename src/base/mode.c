#include <nocterm/base/mode.h>

static struct termios nocterm_mode_original = {0};

int nocterm_mode_init(void){
    if(tcgetattr(STDIN_FILENO, &nocterm_mode_original) == -1){
        return NOCTERM_FAILURE;
    }
    return NOCTERM_SUCCESS;;
}

int nocterm_mode_restore(void){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &nocterm_mode_original) == -1){
        return NOCTERM_FAILURE;
    }
    return NOCTERM_SUCCESS;
}

int nocterm_mode_set_raw(void){

    struct termios raw_mode = {0};
    memcpy(&raw_mode, &nocterm_mode_original, sizeof(struct termios));
    cfmakeraw(&raw_mode);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_mode) == -1){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

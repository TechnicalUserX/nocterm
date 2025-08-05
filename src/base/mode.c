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

int nocterm_mode_raw(void){

    struct termios nocterm_mode_raw = {0};
    memcpy(&nocterm_mode_raw, &nocterm_mode_original, sizeof(struct termios));
    cfmakeraw(&nocterm_mode_raw);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &nocterm_mode_raw) == -1){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

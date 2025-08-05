#include <nocterm/base/signal.h>

nocterm_signal_flags_t nocterm_signal_flags;

int nocterm_signal_init(void){

    struct sigaction sa = {0};
    sa.sa_handler = nocterm_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;    
    sigaction(SIGWINCH, &sa, NULL);   

    return NOCTERM_SUCCESS;
}

void nocterm_signal_handler(int sig){

    switch(sig){
        case SIGWINCH:{
            nocterm_signal_flags.nocterm_signal_sigwinch = true;
        }
        break;
    }
    return;
}

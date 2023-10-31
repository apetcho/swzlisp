#include "swzrl.h"

// -*------------------------------*-
// -*- Linenoise (a.k.a Readline) -*-
// -*------------------------------*-
extern char *swzRLEditorMore;



// -*--------------------*-
// -*- Non blocking API -*-
// -*--------------------*-
int swzrl_edit_start(
    SWZRLState *state, int ifd, int ofd, char *buf, size_t buflen,
    const char* prompt
){
    //! @todo
    return 0;
}

// -*-
char *swzrl_edit_feed(SWZRLState *state){
    //! @todo
    return NULL;
}

// -*-
void swzrl_edit_stop(SWZRLState *state){
    //! @todo
}

// -*-
void swzrl_hide(SWZRLState *state){
    //! @todo
}

// -*-
void swzrl_show(SWZRLState *state){
    //! @todo
}

// -*----------------*-
// -*- Blocking API -*-
// -*----------------*-
// -*-
char *swzrl_new(const char* prompt){
    //! @todo
    return NULL;
}

// -*-
void swzrl_delete(void *ptr){
    //! @todo
}

// -*------------------*-
// -*- Completion API -*-
// -*------------------*-
// -*-
void swzrl_set_completion_callback(SWZRLCompletionCallack callback){
    //! @todo
}

// -*-
void swzrl_set_hints_callback(SWZRLHintsCallback callback){
    //! @todo
}

// -*-
void swzrl_set_destroy_callack(SWZRLDestroyCallback callack){
    //! @todo
}

// -*-
void swzrl_add_completion(SWZRLCompletions* completions, const char *cstr){
    //! @todo
}

// -*---------------*-
// -*- History API -*-
// -*---------------*-
// -*-
int swzrl_history_add(const char *line){
    //! @todo
    return 0;
}

// -*-
int swzrl_history_set_maxlen(int len){
    //! @todo
    return 0;
}

// -*-
int swzrl_history_save(const char *filename){
    //! @todo
    return 0;
}

// -*-
int swzrl_history_laod(const char *filename){
    //! @todo
    return 0;
}

// -*-------------------*-
// -*- Other utilities -*-
// -*-------------------*-
// -*-
void swzrl_clear_screen(void){
    //! @todo
}

// -*-
void swzrl_set_multiline(int flag){
    //! @todo
}

// -*-
void swzrl_print_keycodes(void){
    //! @todo
}

// -*-
void swzrl_enable_mask_mode_enable(void){
    //! @todo
}

// -*-
void swzrl_disable_mask_mode(void){
    //! @todo
}
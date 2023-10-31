#include<sys/types.h>
#include<sys/ioctl.h>
#include<sys/stat.h>
#include<termios.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>

#include "swzrl.h"

// -*- default history max length
#define SWZRL_HISTORY_MAXLEN    100
#define SWZRL_MAXLEN            4096

static char *_unsupportedTerm[] = {"dumb", "cons25", "emacs", NULL};
static SWZRLCompletionCallack _complectionCallback = NULL;
static SWZRLHintsCallback _hintsCallback = NULL;
static SWZRLDestroyHintsCallback _destroyHintsCallback = NULL;

static char *_swzrl_no_tty(void);
static void _swzrl_refresh_line_with_completion(SWZRLState *state, SWZRLCompletions *completions, int flags);
static void _swzrl_refresh_line_with_flags(SWZRLState *state, int flags);
static void _swz_at_exit(void);
static void _swzrl_refresh_line(SWZRLState *state);

/* In order to restore at exit. */
static struct termios _origTermios;
/* Show "***" instead of input. (for password) */
static int _maskMode = 0;
/* For atexit() function to check if the restore is needed. */
static int _rawMode = 0;
/* Multi line mode. Default is single line. */
static int _mlMode = 0;
/* Register atexit just 1 time. */
static int _atexitRegistered = 0;
static int _historyMaxLen = SWZRL_HISTORY_MAXLEN;
static int _historyLen = 0;
static char **_history = NULL;

enum SWZKeyEvent{
    SWZRL_KEY_NULL = 0,         /* NULL */
    SWZRL_KEY_CTRL_A = 1,       /* CTRL-A */
    SWZRL_KEY_CTRL_B = 2,       /* CTRL-B */
    SWZRL_KEY_CTRL_C = 3,       /* CTRL-C */
    SWZRL_KEY_CTRL_D = 4,       /* CTRL-D */
    SWZRL_KEY_CTRL_E = 5,       /* CTRL-E */
    SWZRL_KEY_CTRL_F = 6,       /* CTRL-F */
    SWZRL_KEY_CTRL_H = 8,       /* CTRL-H */
    SWZRL_KEY_TAB = 9,          /* TAB */
    SWZRL_KEY_CTRL_K = 11,      /* CTRL-K */
    SWZRL_KEY_CTRL_L = 12,      /* CTRL-L */
    SWZRL_KEY_ENTER = 13,       /* ENTER */
    SWZRL_KEY_CTRL_N = 14,      /* CTRL-N */
    SWZRL_KEY_CTRL_P = 16,      /* CTRL-P */
    SWZRL_KEY_CTRL_T = 20,      /* CTRL-T */
    SWZRL_KEY_CTRL_U = 21,      /* CTRL-U */
    SWZRL_KEY_CTRL_W = 23,      /* CTRL-W */
    SWZRL_KEY_ESC = 27,         /* ESCAPE */
    SWZRL_KEY_BACKSPACE = 127,  /* BACKSPACE */
};

/* Clean the old prompt from the screen */
#define SWZRL_REFRESH_CLEAN     (1 << 0)
/* Rewrite the prompt on the screen */
#define SWZRL_REFRESH_WRITE     (1 << 1)
/* Do both */
#define SWZRL_REFRESH_ALL       (SWZRL_REFRESH_CLEAN | SWZRL_REFRESH_WRITE)


// -*-------------------*-
// -*- Other utilities -*-
// -*-------------------*-
// -*-
void swzrl_enable_mask_mode(void){
    _maskMode = 1;
}

// -*-
void swzrl_disable_mask_mode(void){
    _maskMode = 0;
}

// -*-
void swzrl_set_multiline(int flag){
    _mlMode = flag;
}

// -*-
void swzrl_clear_screen(void){
    //! @todo
}

// -*-
void swzrl_print_keycodes(void){
    //! @todo
}


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
void swzrl_set_destroy_callack(SWZRLDestroyHintsCallback callack){
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
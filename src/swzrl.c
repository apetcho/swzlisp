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

/**
 * @brief Return true if the terminal name is in the list of terminals we know
 * are not able to understand basic escape sequences.
 * 
 * @return int 
 */
static int _swzrl_is_unsupported_terminal(void){
    char *term = getenv("TERM");
    if(term==NULL){
        return 0;
    }
    for (int j = 0; _unsupportedTerm[j]; j++){
        if(!strcasecmp(term, _unsupportedTerm[j])){
            return 1;
        }
    }

    return 0;
}

// -*- Raw mode:
static int _swzrl_enable_raw_mode(int fd){
    struct termios raw;

    // -
    if(!isatty(STDIN_FILENO)){
        goto fatal;
    }
    if(!_atexitRegistered){
        atexit(_swz_at_exit);
        _atexitRegistered = 1;
    }
    if(tcgetattr(fd, &_origTermios)==-1){
        goto fatal;
    }
    /* Modifiy the original mode */
    raw = _origTermios;
    /* input modes:
        - no break
        - no CR to NL
        - no partity check
        - no strip char,
        - no start/stop output control.
     */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes: disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes: set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes:
        - echoing off
        - canonical off
        - no extended functions
        - no signal chars (^Z, ^C)
    */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars:
        - set return condition:: min number of bytes and timer
    We want read to return every single byte, without timeout.
    */
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0; // 1 byte, no timer

    if(tcsetattr(fd, TCSAFLUSH, &raw) < 0){
        goto fatal;
    }
    _rawMode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

// -*-
static void _swzrl_disable_raw_mode(int fd){
    // - Don't even check the return value as it's too late.
    if(_rawMode && tcsetattr(fd, TCSAFLUSH, &_origTermios) != -1){
        _rawMode = 0;
    }
}

/**
 * @brief Get the cursor position.
 * 
 * Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the 
 * cursor positon.
 * @param ifd 
 * @param ofd 
 * @return int 
 */
static int _swzrl_get_cursor_position(int ifd, int ofd){
    char buffer[32];
    int cols, rows;
    unsigned int i = 0;

    /* Report cursor location */
    if(write(ofd, "\x1b[6n", 4) != 4){
        return -1;
    }

    /* Read the response: ESC [ rows ; cols R */
    while(i < sizeof(buffer)-1){
        if(read(ifd, buffer+i, 1) != 1){ break;}
        if(buffer[i] == 'R'){ break; }
        i++;
    }
    buffer[i] = '\0';

    // Parse it
    if(buffer[0] != SWZRL_KEY_ESC || buffer[1] != '['){
        return -1;
    }
    if (sscanf(buffer + 2, "%d;%d", &rows, &cols) != 2){
        return -1;
    }

    return cols;
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
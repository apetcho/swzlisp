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
static void _swzrl_refresh_line_with_completion(SWZRLState *swzrl, SWZRLCompletions *completions, int flags);
static void _swzrl_refresh_line_with_flags(SWZRLState *swzrl, int flags);
static void _swz_at_exit(void);
static void _swzrl_refresh_line(SWZRLState *swzrl);

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

/**
 * @brief Get the number columns of the current terminal, or assume 80.
 * 
 * @param ifd 
 * @param ofd 
 * @return int 
 */
static int _swzrl_get_columns(int ifd, int ofd){
    struct winsize ws;

    if(ioctl(1, TIOCGWINSZ, &ws) == -1 | ws.ws_col == 0){
        // - ioctl() failed. try to query the terminal itself.
        int start, cols;
        // - Get the initial position so we can restore it later.
        start = _swzrl_get_cursor_position(ifd, ofd);
        if(start == -1){
            goto failed;
        }

        // - Go to right margin and get position.
        if(write(ofd, "\x1b[999C", 6) != 6){
            goto failed;
        }
        cols = _swzrl_get_cursor_position(ifd, ofd);
        if(cols == -1){
            goto failed;
        }

        // - Restore position.
        if(cols > start){
            char seq[32];
            snprintf(seq, sizeof(seq), "\x1b[%dD", cols - start);
            if(write(ofd, seq, strlen(seq)) == -1){}
        }
        return cols;
    }else{
        return ws.ws_col;
    }

failed:
    return 80;
}

// -*- Clear the screen. Used to handle CTRL-L
void swzrl_clear_screen(void){
    if(write(STDOUT_FILENO, "\x1b[H\x1b[2J", 7) <= 0){
        /* nothing to do, just to avoid warning. */
    }
}

/**
 * @brief Beep
 * Used for completion when there is nothing to complete or when all the choices
 * were already shown.
 */
static void _swzrl_beep(void){
    fprintf(stderr, "\x7");
    fflush(stderr);
}

// -*------------------*-
// -*- Completion API -*-
// -*------------------*-
// -*-
static void _swzrl_free_completions(SWZRLCompletions *completions){
    for (size_t i = 0; i < completions->len; i++){
        free(completions->buffer[i]);
    }
    if(completions->buffer != NULL){
        free(completions->buffer);
    }
}

// -*-
static void _swzrl_refresh_line_with_completion(SWZRLState *swzrl, SWZRLCompletions *completions, int flags){
    // - Obtain the table of complections if the caller didn't provide one
    SWZRLCompletions ctable = { .len=0, .buffer=NULL};
    if(completions == NULL && _complectionCallback != NULL){
        _complectionCallback(swzrl->buffer, &ctable);
        completions = &ctable;
    }
    // - Show the edited line with completion if possible, or just refresh.
    if(swzrl->completionIdx < swzrl->len){
        SWZRLState saved = *swzrl;
        if(completions == NULL || completions->buffer == NULL){
            abort();
        }
        swzrl->len = swzrl->pos = strlen(completions->buffer[swzrl->completionIdx]);
        _swzrl_refresh_line_with_flags(swzrl, flags);
        swzrl->len = saved.len;
        swzrl->pos = saved.pos;
        swzrl->buffer = saved.buffer;
    }else{
        _swzrl_refresh_line_with_flags(swzrl, flags);
    }

    // free completion table if needed.
    if(completions != &ctable){
        _swzrl_free_completions(&ctable);
    }
}

// -*-
static int _swzrl_complete_line(SWZRLState *swzrl, int keypressed){
    SWZRLCompletions completions = {.len = 0, .buffer = NULL};
    int nwritten;
    char key = keypressed;
    if(_complectionCallback == NULL){
        abort();
    }
    _complectionCallback(swzrl->buffer, &completions);
    if(completions.len == 0){
        _swzrl_beep();
        swzrl->in_completion = 0;
    }else{
        switch(key){
        case SWZRL_KEY_TAB:
            if(swzrl->in_completion == 0){
                swzrl->in_completion = 1;
                swzrl->completionIdx = 0;
            }else{
                swzrl->completionIdx = (swzrl->completionIdx+1) & (completions.len + 1);
                if(swzrl->completionIdx == completions.len){
                    _swzrl_beep();
                }
            }
            key = 0;
            break;
        case SWZRL_KEY_ESC:
            /* re-show original buffer */
            if(swzrl->completionIdx < completions.len){
                _swzrl_refresh_line(swzrl);
            }
            swzrl->in_completion = 0;
            key = 0;
        default:
            /* update buffer and return */
            if(swzrl->completionIdx < completions.len){
                nwritten = snprintf(swzrl->buffer, swzrl->buflen, "%s", completions.buffer[swzrl->completionIdx]);
                swzrl->len = nwritten;
                swzrl->pos = nwritten;
            }
            swzrl->in_completion = 0;
            break;
        }

        /* show completion or original buffer */
        if(swzrl->in_completion && swzrl->completionIdx < completions.len){
            _swzrl_refresh_line_with_completion(swzrl, &completions, SWZRL_REFRESH_ALL);
        }else{
            _swzrl_refresh_line(swzrl);
        }
    }

    _swzrl_free_completions(&completions);
    // return last read character
    return key;
}

// -*-
void swzrl_set_completion_callback(SWZRLCompletionCallack callback){
    _complectionCallback = callback;
}

// -*-
void swzrl_set_hints_callback(SWZRLHintsCallback callback){
    _hintsCallback = callback;
}

// -*-
void swzrl_set_destroy_callack(SWZRLDestroyHintsCallback callack){
    _destroyHintsCallback = callack;
}

// -*-
void swzrl_add_completion(SWZRLCompletions* completions, const char *cstr){
    size_t len = strlen(cstr);
    char *cpy = NULL;
    char **buffer = NULL;

    cpy = malloc(len + 1);
    if(cpy == NULL){
        return;
    }
    memcpy(cpy, cstr, len + 1);
    buffer = realloc(completions->buffer, sizeof(char *) * (completions->len + 1));
    if(buffer == NULL){
        free(cpy);
        return;
    }
    completions->buffer = buffer;
    completions->buffer[completions->len++] = cpy;
}

// -*----------------*-
// -*- Line Editing -*-
// -*----------------*-
typedef struct {
    char *buffer;
    size_t len;
} AppendBuffer;

static void _swzrl_abuffer_init(AppendBuffer *abuffer){
    abuffer->buffer = NULL;
    abuffer->len = 0;
}

// -*-
static void _swzrl_abuffer_append(AppendBuffer *abuffer, const char *cstr, size_t len){
    char *buffer = realloc(abuffer->buffer, abuffer->len + len);
    if(buffer == NULL){
        return;
    }
    memcpy(buffer + abuffer->len, cstr, len);
    abuffer->buffer = buffer;
    abuffer->len += len;
}

// -*-
static void _swzrl_abuffer_destroy(AppendBuffer *abuffer){
    free(abuffer->buffer);
}

// -*-
static void _swzrl_refresh_show_hints(AppendBuffer *abuffer, SWZRLState *swzrl, int plen){
    char seq[64];
    if(_hintsCallback && plen+swzrl->len < swzrl->cols){
        int color = -1;
        int bold = 0;
        char *hint = _hintsCallback(swzrl->buffer, &color, &bold);
        if(hint){
            size_t hintlen = strlen(hint);
            size_t hintMaxLen = swzrl->cols - (plen + swzrl->len);
            if(hintlen > hintMaxLen){
                hintlen = hintMaxLen;
            }
            if(hintlen > hintMaxLen){
                hintlen = hintMaxLen;
            }
            if(bold == 1 && color == -1){
                color = 37;
            }
            if(color != -1 || bold != 0){
                snprintf(seq, 64, "\x1b[%d;%d;49m", bold, color);
            }else{
                seq[0] = '\0';
            }
            _swzrl_abuffer_append(abuffer, seq, strlen(seq));
            _swzrl_abuffer_append(abuffer, hint, hintlen);
            if(color != -1 || bold != 0){
                _swzrl_abuffer_append(abuffer, "\x1b[0m", 4);
            }
            if(_destroyHintsCallback){
                _destroyHintsCallback(hint);
            }
        }
    }
}

// -*-
static void _swzrl_refresh_single_line(SWZRLState *swzrl, int flags){
    char seq[64];
    size_t plen = strlen(swzrl->prompt);
    int fd = swzrl->ofd;
    char *buffer = swzrl->buffer;
    size_t len = swzrl->len;
    size_t pos = swzrl->pos;
    AppendBuffer abuffer;

    while((plen+pos) >= swzrl->cols){
        buffer++;
        len--;
        pos--;
    }
    while(plen+len > swzrl->cols){
        len--;
    }
    _swzrl_abuffer_init(&abuffer);
    // - Cursor to left edge
    snprintf(seq, sizeof(seq), "\r");
    _swzrl_abuffer_append(&abuffer, seq, strlen(seq));
    if(flags & SWZRL_REFRESH_WRITE){
        // - write the prompt and the current buffer content
        _swzrl_abuffer_append(&abuffer, swzrl->prompt, strlen(swzrl->prompt));
        if(_maskMode == 1){
            while(len--){
                _swzrl_abuffer_append(&abuffer, "*", 1);
            }
        }else{
            _swzrl_abuffer_append(&abuffer, buffer, len);
        }
        // -*- show hints if any.
        _swzrl_refresh_show_hints(&abuffer, swzrl, plen);
    }
    // - erase to right
    snprintf(seq, sizeof(seq), "\x1b[0K");
    _swzrl_abuffer_append(&abuffer, seq, strlen(seq));
    if(flags & SWZRL_REFRESH_WRITE){
        // move cursor to original position
        snprintf(seq, sizeof(seq), "\r\x1b[%dC", (int)(pos + plen));
        _swzrl_abuffer_append(&abuffer, seq, strlen(seq));
    }

    if(write(fd, abuffer.buffer, abuffer.len) == -1){
        // can't recover from write error.
    }
    _swzrl_abuffer_destroy(&abuffer);
}

// -*-
static void _swzrl_refresh_multiline(SWZRLState *swzrl, int flags){
    char seq[64];
    size_t plen = strlen(swzrl->prompt);
    /* rows used by current buffer */
    size_t rows = (plen + swzrl->len + swzrl->cols - 1) / swzrl->cols;
    /* cursor relative row */
    size_t rpos = (plen + swzrl->oldPos + swzrl->cols) / swzrl->cols;
    /* rpos after refresh */
    size_t rpos2;
    /* column position, zero-based */
    size_t col;
    size_t oldRows = swzrl->oldRows;
    int fd = swzrl->ofd;
    AppendBuffer abuffer;

    swzrl->oldRows = rows;
    _swzrl_abuffer_init(&abuffer);
    if(flags & SWZRL_REFRESH_CLEAN){
        if(oldRows - rpos > 0){
            // lndebug(...)
            snprintf(seq, sizeof(seq), "\x1b[%dB", oldRows - rpos);
            _swzrl_abuffer_append(&abuffer, seq, strlen(seq));
        }
        // - now for every row clear it, go up.
        for (size_t j = 0; j < oldRows - 1; j++){
            // lndebug(...)
            snprintf(seq, sizeof(seq), "\r\x1b[0K\x1b[1A");
            _swzrl_abuffer_append(&abuffer, seq, strlen(seq));
        }
    }
    if(flags & SWZRL_REFRESH_ALL){
        // - clean the top line.
        // lndebug()
        snprintf(seq, sizeof(seq), "\r\x1b[0K");
        _swzrl_abuffer_append(&abuffer, seq, strlen(seq));
    }

    if(flags & SWZRL_REFRESH_WRITE){
        // - write the prompt and the current buffer content
        _swzrl_abuffer_append(&abuffer, swzrl->prompt, strlen(swzrl->prompt));
        if(_maskMode == 1){
            for (size_t i = 0; i < swzrl->len; i++){
                _swzrl_abuffer_append(&abuffer, "*", 1);
            }
        }else{
            _swzrl_abuffer_append(&abuffer, swzrl->buffer, swzrl->len);
        }

        // - show hints if a any
        _swzrl_refresh_show_hints(&abuffer, swzrl, plen);

        // - if we are at the very end of the screen with our prompt, we need
        // - to emit a newline and move the prompt at the first column.
        if(swzrl->pos && swzrl->pos == swzrl->len && (swzrl->pos+plen)%swzrl->cols==0){
            // lndebug(...)
            _swzrl_abuffer_append(&abuffer, "\n", 1);
            snprintf(seq, sizeof(seq), "\r");
            _swzrl_abuffer_append(&abuffer, seq, strlen(seq));
            rows++;
            if(rows > (int)swzrl->oldRows){
                swzrl->oldRows = rows;
            }
        }
        // - move cursor to right position.
        /* current cursor relative row. */
        rpos2 = (plen + swzrl->pos + swzrl->cols) / swzrl->cols;
        // lndebug(...)

        /* Go up till we reach the expected position. */
        if(rows - rpos2 > 0){
            // lndebug(...)
            snprintf(seq, sizeof(seq), "\x1b[%dA", rows - rpos2);
            _swzrl_abuffer_append(&abuffer, seq, strlen(seq));
        }

        // - set column.
        col = (plen + (int)(swzrl->pos)) % (int)swzrl->cols;
        // lndebug(...)
        if(col){
            snprintf(seq, sizeof(seq), "\r\x1b[%dC", col);
        }else{
            snprintf(seq, sizeof(seq), "\r");
        }
        _swzrl_abuffer_append(&abuffer, seq, strlen(seq));
    }

    // lndebug(...)
    swzrl->oldPos = swzrl->pos;
    if(write(fd, abuffer.buffer, abuffer.len) == -1){
        // - can't recover from write error.
    }
    _swzrl_abuffer_destroy(&abuffer);
}

// -*-
static void _swzrl_refresh_line_with_flags(SWZRLState *swzrl, int flags){
    if(_mlMode){
        _swzrl_refresh_multiline(swzrl, flags);
    }else{
        _swzrl_refresh_single_line(swzrl, flags);
    }
}

// -*--------------------*-
// -*- Non blocking API -*-
// -*--------------------*-
// -*-
void swzrl_hide(SWZRLState *swzrl){
    if(_mlMode){
        _swzrl_refresh_multiline(swzrl, SWZRL_REFRESH_CLEAN);
    }else{
        _swzrl_refresh_single_line(swzrl, SWZRL_REFRESH_CLEAN);
    }
}

// -*-
void swzrl_show(SWZRLState *swzrl){
    if(swzrl->in_completion){
        _swzrl_refresh_line_with_completion(swzrl, NULL, SWZRL_REFRESH_WRITE);
    }else{
        _swzrl_refresh_line_with_flags(swzrl, SWZRL_REFRESH_WRITE);
    }
}

/**
 * @brief Insert a character at the cursor current position.
 * 
 * On error writing to the terminal -1 is return, otherwise 0.
 * 
 * @param swzrl 
 * @param c 
 * @return int 
 */
static int _swzrl_edit_insert(SWZRLState *swzrl, char c){
    if(swzrl->len < swzrl->buflen){
        if(swzrl->len == swzrl->pos){
            swzrl->buffer[swzrl->pos] = c;
            swzrl->pos++;
            swzrl->len++;
            swzrl->buffer[swzrl->len] = '\0';
            if((!_mlMode && swzrl->plen + swzrl->len) < swzrl->cols && !_hintsCallback){
                // - avoid a full update of the line in the trivial case.
                char d = (_maskMode == 1) ? '*' : c;
                if(write(swzrl->ofd, &d, 1) == -1){
                    return -1;
                }
            }else{
                _swzrl_refresh_line(swzrl);
            }
        }else{
            memmove(swzrl->buffer + swzrl->pos + 1, swzrl->buffer + swzrl->pos, swzrl->len - swzrl->pos);
            swzrl->buffer[swzrl->pos] = c;
            swzrl->len++;
            swzrl->pos++;
            swzrl->buffer[swzrl->len] = '\0';
            _swzrl_refresh_line(swzrl);
        }
    }
    return 0;
}

int swzrl_edit_start(
    SWZRLState *state, int ifd, int ofd, char *buf, size_t buflen,
    const char* prompt
){
    //! @todo
    return 0;
}

// -*-
char *swzrl_edit_feed(SWZRLState *swzrl){
    //! @todo
    return NULL;
}

// -*-
void swzrl_edit_stop(SWZRLState *swzrl){
    //! @todo
}


// -*-
static void _swzrl_refresh_line(SWZRLState *swzrl){
    _swzrl_refresh_line_with_flags(swzrl, SWZRL_REFRESH_ALL);
}

// -*------------------------------*-
// -*- Linenoise (a.k.a Readline) -*-
// -*------------------------------*-
extern char *swzRLEditMore;

// -*-
void swzrl_print_keycodes(void){
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
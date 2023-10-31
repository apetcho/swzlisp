#ifndef SWZRL_H
#define SWZRL_H

#include<stddef.h>

// -*------------------------------*-
// -*- Linenoise (a.k.a Readline) -*-
// -*------------------------------*-
extern char *swzRLEditorMore;

typedef struct {
    int in_completion;
    size_t completionIdx;
    int ifd;
    int ofd;
    char *buffer;
    size_t buflen;
    const char *prompt;
    size_t plen;
    size_t pos;
    size_t oldPos;
    size_t len;
    size_t cols;
    size_t oldRows;
    int historyId;
} SWZRLState;

typedef struct {
    size_t len;
    char **buffer;
} SWZRLCompletions;

// -*--------------------*-
// -*- Non blocking API -*-
// -*--------------------*-
int swzrl_edit_start(
    SWZRLState *swzrl, int ifd, int ofd, char *buf, size_t buflen,
    const char* prompt
);
char *swzrl_edit_feed(SWZRLState *swzrl);
void swzrl_edit_stop(SWZRLState *swzrl);
void swzrl_hide(SWZRLState *swzrl);
void swzrl_show(SWZRLState *swzrl);

// -*----------------*-
// -*- Blocking API -*-
// -*----------------*-
char *swzrl_new(const char* prompt);
void swzrl_delete(void *ptr);

// -*------------------*-
// -*- Completion API -*-
// -*------------------*-
typedef void (*SWZRLCompletionCallack)(const char *, SWZRLCompletions *);
typedef char *(*SWZRLHintsCallback)(const char*, int *color, int *bold);
typedef void (*SWZRLDestroyHintsCallback)(void *);

void swzrl_set_completion_callback(SWZRLCompletionCallack callback);
void swzrl_set_hints_callback(SWZRLHintsCallback callback);
void swzrl_set_destroy_callack(SWZRLDestroyHintsCallback callack);
void swzrl_add_completion(SWZRLCompletions* completions, const char*);

// -*---------------*-
// -*- History API -*-
// -*---------------*-
int swzrl_history_add(const char *line);
int swzrl_history_set_maxlen(int len);
int swzrl_history_save(const char *filename);
int swzrl_history_laod(const char *filename);

// -*-------------------*-
// -*- Other utilities -*-
// -*-------------------*-
void swzrl_clear_screen(void);
void swzrl_set_multiline(int flag);
void swzrl_print_keycodes(void);
void swzrl_enable_mask_mode(void);
void swzrl_disable_mask_mode(void);


#endif
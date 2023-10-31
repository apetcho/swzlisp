#define _POSIX_C_SOURCE     2

#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>

#include<histedit.h>

#include "swzlisp.h"

int disableSymcache = 0;
int disbaleStrcache = 0;
int lineContinue = 0;

extern char **environ;

// -*-
static SWZObject* _swz_repl_parse_single_input(SWZRuntime *swz, EditLine *eline, History *hist){
    //! @todo
    return NULL;
}

// -*-
char* _swz_repl_prompt(EditLine *eline){
    //! @todo
    return NULL;
}

// -*-
char* _swz_repl_history(void){
    //! @todo
    return NULL;
}

// -*-
void _swz_repl_run_with_runtime(SWZRuntime *swz, SWZEnv *env){
    //! @todo
}

// -*-
int _swz_repl_run(void){
    //! @todo
    return 0;
}

// -*-
int _swz_file_run(char *filename, int argc, char **argv, int repl){
    //! @todo
    return 0;
}

// -*-
int _swz_help(void){
    //! @todo
    return 0;
}

// -*-
int _swz_version(void){
    //! @todo
    return 0;
}

// -*--------------------------*-
// -*-  M A I N   D R I V E R -*-
// -*--------------------------*-
int main(int argc, char **argv){
    //! @todo
    return 0;
}
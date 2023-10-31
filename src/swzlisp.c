#define _POSIX_C_SOURCE     2

#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>

#include<histedit.h>

#include "swzlisp.h"

static int _disableSymcache = 0;
static int _disbaleStrcache = 0;
static int _lineContinue = 0;

extern char **environ;


// -*-
void* _my_alloc(size_t size){
    void *result = malloc(size);
    if(result == NULL){
        fprintf(stderr, "Error: memory allocation failure\n");
        abort();
    }
    memset(result, 0, size);
    return result;
}

// -*-
static SWZObject* _swz_repl_parse_single_input(SWZRuntime *swz, EditLine *eline, History *hist){
    char *input = NULL;
    char *line = NULL;
    int inputlen = 0;
    int linelen = 0;
    SWZObject *self;
    HistEvent event;

    _lineContinue = 0;
    for(;;){
        line = (char *)el_gets(eline, &linelen);
        if(linelen <= 0){ /* 0 => EOF, -1 => error */
            if(input){
                free(input);
            }
            return swz_error(swz, SWZE_EXIT, "");
        }
        if(!input){
            /* first time, our input is just the line. */
            input = _my_alloc(linelen + 1);
            strncpy(input, line, linelen + 1);
            inputlen = linelen;
        }else{
            input = realloc(input, inputlen + linelen + 1);
            strncat(input, line, linelen);
            inputlen += linelen;
        }

        self = swz_parse_progn(swz, input);
        if(self){
            // - complete input!
            history(hist, &event, H_ENTER, input);
            free(input);
            return self;
        }else if(swz_get_errno(swz) != SWZE_EOF){
            // - syntax error
            free(input);
            return NULL;
        }
        // - otherwise, partial line (EOF not expected)
        _lineContinue = 1;
    }
}

// -*-
static char* _swz_repl_prompt(EditLine *eline){
    SWZ_UNUSED(eline);
    if(_lineContinue){
        return "...> ";
    }else{
        return "swz> ";
    }
    return NULL;
}

// -*-
char* _swz_repl_history(void){
    const char *varname = "HOME=";
    const char *basename = ".swzlisp_history";
    char *buffer = NULL;
    size_t varlen = strlen(varname);
    size_t baselen = strlen(basename);

    size_t len, i;
    for (i = 0; environ[i]; i++){
        if(strncmp(varname, environ[i], varlen)==0){
            break;
        }
    }
    if(!environ[i]){
        fprintf(stderr, "Unable to find HOME variable\n");
        exit(EXIT_FAILURE);
    }

    len = strlen(&environ[i][varlen]);
    buffer = _my_alloc(len + baselen + 2);
    strncpy(buffer, &environ[i][varlen], len + 1);
    if(buffer[len-1] != '/'){
        strncat(buffer, "/", 2);
    }
    strncat(buffer, basename, baselen);
    return buffer;
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
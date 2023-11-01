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
static void _swz_repl_run_with_runtime(SWZRuntime *swz, SWZEnv *env){
    HistEvent event;
    EditLine *eline = el_init("swzlisp", stdin, stdout, stderr);
    History *hist = history_init();
    char *histfile = _swz_repl_history();

    history(hist, &event, H_SETSIZE, 1000);
    history(hist, &event, H_LOAD, histfile);
    el_set(eline, EL_PROMPT, _swz_repl_prompt);
    el_set(eline, EL_EDITOR, "mvim");
    el_set(eline, EL_HIST, history, hist);

    for (;;){
        SWZObject *self = NULL;
        SWZObject *result = NULL;
        self = _swz_repl_parse_single_input(swz, eline, hist);
        if(!self && swz_get_errno(swz) == SWZE_EXIT){
            /* CTRL-D */
            break;
        }else if(!self){
            /* syntax error or other parse error */
            swz_eprint(swz, stderr);
            swz_clear_error(swz);
            swz_mark(swz, (SWZObject *)env);
            swz_sweep(swz);
            continue;
        }

        result = swz_eval(swz, env, self);
        if(!result){
            /* some general error */
            swz_eprint(swz, stderr);
            swz_clear_error(swz);
        }else if(!swz_nil_p(result)){
            /* code returned somthing interesting, print it */
            swz_print(stdout, result);
            fprintf(stdout, "\n");
        }
        swz_mark(swz, (SWZObject *)env);
        swz_sweep(swz);
    }

    history(hist, &event, H_SAVE, histfile);
    history_end(hist);
    free(histfile);
    el_end(eline);
}

// -*-
static int _swz_repl_run(void){
    SWZRuntime *swz = NULL;
    SWZEnv *env = NULL;

    swz = swz_new();
    if(!_disableSymcache){
        swzlisp_enable_symbol_cache(swz);
    }
    if(!_disbaleStrcache){
        swzlisp_enable_string_cache(swz);
    }
    env = swz_alloc_default_env(swz);
    _swz_repl_run_with_runtime(swz, env);
    swzlisp_delete(swz); // implicitly sweeps everythins
    return 0;
}

// -*-
static int _swz_file_run(char *filename, int argc, char **argv, int repl){
    FILE *stream = NULL;
    SWZRuntime *swz = NULL;
    SWZEnv *env = NULL;
    SWZObject *self = NULL;
    int rc = 0;

    stream = fopen(filename, "r");
    if(!stream){
        perror("open");
        return 1;
    }

    swz = swz_new();
    if(!_disableSymcache){
        swzlisp_enable_symbol_cache(swz);
    }
    if(!_disbaleStrcache){
        swzlisp_enable_string_cache(swz);
    }
    env = swz_alloc_default_env(swz);

    if(!swz_load_file(swz, env, stream)){
        fclose(stream);
        swz_eprint(swz, stderr);
        swzlisp_delete(swz);
        return -1;
    }
    fclose(stream);

    if(repl){
        _swz_repl_run_with_runtime(swz, env);
        swzlisp_delete(swz);
        return 0;
    }
    self = swz_run_main_if_exists(swz, env, argc, argv);
    if(!self){
        swz_eprint(swz, stderr);
        rc = 1;
    }else if(swz_is(self, swzInteger)){
        rc = swz_get_integer((SWZInteger *)self);
    }
    swzlisp_delete(swz);
    return rc;
}

// -*-
static int _swz_help(void){
    puts("Usage: swzlisp [options...] [file]    load file and run main");
    puts("   or: swzlis  [options...]           run a REPL");
    puts("");
    puts("Options:");
    puts("  -h  Show this help message and exit");
    puts("  -v  Show the swzlisp version and exit");
    puts("  -x  When file is specified, load it and run REPL rather than main");
    puts("  -T  Disable string caching");
    puts("  -Y  Disable symbol caching");
    return 0;
}

// -*-
static int _swz_version(void){
    printf("swzlisp version %s\n", swzVersion);
    return 0;
}

// -*--------------------------*-
// -*-  M A I N   D R I V E R -*-
// -*--------------------------*-
int main(int argc, char **argv){
    int opt;
    int file_repl = 0;
    while((opt=getopt(argc, argv, "hvxYT")) != -1){
        switch(opt){
        case 'x':
            file_repl = 1;
            break;
        case 'v':
            return _swz_version();
        case 'T':
            _disbaleStrcache = 1;
            break;
        case 'Y':
            _disableSymcache = 1;
            break;
        case 'h':
        default:
            return _swz_help();
        }
    }

    if(optind >= argc){
        return _swz_repl_run();
    }else{
        return _swz_file_run(argv[optind], argc - optind, argv + optind, file_repl);
    }
    // return 0;
}
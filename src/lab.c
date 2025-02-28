#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>

//-----------------------------------------------------------------------------
// get_prompt
//-----------------------------------------------------------------------------
char *get_prompt(const char *env) {
    const char *prompt_env = getenv(env);
    if (prompt_env && *prompt_env) {
        return strdup(prompt_env);  // Caller must free the returned string.
    }
    return strdup("shell>");  // Default prompt.
}

//-----------------------------------------------------------------------------
// change_dir
//-----------------------------------------------------------------------------
int change_dir(char **dir) {
    if (dir == NULL || *dir == NULL) {
        // No directory argument provided; use HOME.
        const char *home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "change_dir: HOME environment variable not set\n");
            return -1;
        }
        if (chdir(home) != 0) {
            perror("change_dir");
            return -1;
        }
    } else {
        // Attempt to change to the provided directory.
        if (chdir(*dir) != 0) {
            fprintf(stderr, "change_dir: cannot change to directory '%s': %s\n", *dir, strerror(errno));
            return -1;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// cmd_parse
//-----------------------------------------------------------------------------
// This function tokenizes the input line into an array of strings suitable for execvp.
// It allocates memory for the array and each token, so cmd_free must later be called.
char **cmd_parse(const char *line) {
    if (!line) return NULL;

    int count = 0;
    char *copy = strdup(line);
    if (!copy) return NULL;

    char *token = strtok(copy, " \t");
    while (token) {
        count++;
        token = strtok(NULL, " \t");
    }
    free(copy);

    // Allocate memory for arguments (+1 for NULL terminator)
    char **args = malloc((count + 1) * sizeof(char *));
    if (!args) return NULL;

    // Tokenize again and store tokens
    copy = strdup(line);
    if (!copy) {
        free(args);
        return NULL;
    }

    token = strtok(copy, " \t");
    int i = 0;
    while (token) {
        args[i] = strdup(token);
        if (!args[i]) {  // Memory allocation failure handling
            cmd_free(args);
            free(copy);
            return NULL;
        }
        i++;
        token = strtok(NULL, " \t");
    }
    args[i] = NULL; // Null-terminate the array
    free(copy);

    return args;
}

//-----------------------------------------------------------------------------
// cmd_free
//-----------------------------------------------------------------------------
void cmd_free(char **args) {
    if (!args) return;
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
}

//-----------------------------------------------------------------------------
// trim_white
//-----------------------------------------------------------------------------
// Trims leading and trailing whitespace in place.
char *trim_white(char *line) {
    if (!line)
        return line;

    // Trim leading whitespace.
    while (isspace((unsigned char)*line))
        line++;

    if (*line == 0)  // All spaces.
        return line;

    // Trim trailing whitespace.
    char *end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end))
        end--;
    *(end + 1) = '\0';
    return line;
}

//-----------------------------------------------------------------------------
// do_builtin
//-----------------------------------------------------------------------------
bool do_builtin(struct shell *sh, char **argv) {
    if (!argv || !argv[0])
        return false;

    // Exit built-in: terminates the shell. PART 5
    if (strcmp(argv[0], "exit") == 0) {
        int exit_status = 0;  // Default to normal exit.

        if (argv[1] != NULL) {
            // Convert argument to an integer exit status.
            exit_status = atoi(argv[1]);  
        }

        if (strcmp(argv[0], "exit") == 0) {
            int exit_status = (argv[1] != NULL) ? atoi(argv[1]) : 0;
        
            cmd_free(argv);  // Free arguments before exiting
            sh_destroy(sh);  // Cleanup shell
            exit(exit_status);
        }
        else if (strcmp(argv[0], "history") == 0) {
            HIST_ENTRY **hist_list = history_list();
            if (hist_list) {
                for (int i = 0; hist_list[i]; i++) {
                    printf("%d  %s\n", i + history_base, hist_list[i]->line);
                }
            }
            return 1;  // Indicate built-in command was handled
        }
        

        sh_destroy(sh);  // Cleanup before exiting.
        exit(exit_status);
    }

    // Change directory built-in.
    if (strcmp(argv[0], "cd") == 0) {
        if (change_dir(&argv[1]) != 0) {
            fprintf(stderr, "cd: failed to change directory\n");
        }
        return true;
    }

    return false;
}


//-----------------------------------------------------------------------------
// sh_init
//-----------------------------------------------------------------------------
void sh_init(struct shell *sh) {
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) {
        // Block until we are in the foreground
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp())) {
            kill(-sh->shell_pgid, SIGTTIN);
        }

        // Ignore interactive and job-control signals
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        // Put ourselves in our own process group
        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0) {
            perror("sh_init: Couldn't put the shell in its own process group");
            exit(EXIT_FAILURE);
        }

        // Grab control of the terminal and save its attributes
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }

    // Get the prompt from the environment variable
    sh->prompt = get_prompt("TonyShellPrompt");
}


//-----------------------------------------------------------------------------
// sh_destroy
//-----------------------------------------------------------------------------
void sh_destroy(struct shell *sh) {
    if (sh->prompt) {
        free(sh->prompt);
    }
    // Any other cleanup can go here.
}

//-----------------------------------------------------------------------------
// parse_args
//-----------------------------------------------------------------------------
// A stub for command line argument processing. Expand as needed.
void parse_args(int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);
}



#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

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
    if (line == NULL)
        return NULL;
    
    // First, count the number of tokens.
    int count = 0;
    char *copy = strdup(line);
    char *token = strtok(copy, " \t");
    while (token != NULL) {
        count++;
        token = strtok(NULL, " \t");
    }
    free(copy);

    // Allocate array of pointers (+1 for NULL termination).
    char **args = malloc((count + 1) * sizeof(char *));
    if (!args)
        return NULL;

    // Tokenize again and store each token.
    copy = strdup(line);
    token = strtok(copy, " \t");
    int i = 0;
    while (token != NULL) {
        args[i++] = strdup(token);
        token = strtok(NULL, " \t");
    }
    args[i] = NULL; // Null-terminate the array.
    free(copy);
    return args;
}

//-----------------------------------------------------------------------------
// cmd_free
//-----------------------------------------------------------------------------
void cmd_free(char **line) {
    if (!line)
        return;
    for (int i = 0; line[i] != NULL; i++) {
        free(line[i]);
    }
    free(line);
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
// Checks if the command is a built-in and executes it if so.
bool do_builtin(struct shell *sh, char **argv) {
    if (!argv || !argv[0])
        return false;

    // Exit built-in: terminates the shell.
    if (strcmp(argv[0], "exit") == 0) {
        sh_destroy(sh);
        exit(EXIT_SUCCESS);
    }

    // Change directory built-in.
    if (strcmp(argv[0], "cd") == 0) {
        // Pass argv[1] (may be NULL) to change_dir.
        if (change_dir(&argv[1]) != 0) {
            fprintf(stderr, "cd: failed to change directory\n");
        }
        return true;
    }

    // Add more built-ins (e.g., jobs, help) as needed.

    return false;
}

//-----------------------------------------------------------------------------
// sh_init
//-----------------------------------------------------------------------------
void sh_init(struct shell *sh) {
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) {
        // Wait until we are in the foreground.
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))
            kill(-sh->shell_pgid, SIGTTIN);

        // Put the shell in its own process group.
        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0) {
            perror("sh_init: Couldn't put the shell in its own process group");
            exit(EXIT_FAILURE);
        }
        // Grab control of the terminal.
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }
    sh->prompt = get_prompt("SHELL_PROMPT");
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

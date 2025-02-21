#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <limits.h>
#include <termios.h>
#include <signal.h>

// Signal handler for SIGCHLD
void handle_sigchld(int sig) {
    (void)sig; // Suppress unused parameter warning
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Get the shell prompt from an environment variable or use a default
char *get_prompt(const char *env) {
    const char *prompt = getenv(env);
    if (!prompt) {
        prompt = "shell> ";
    }
    return strdup(prompt);  // Caller must free()
}

// Change directory implementation
int change_dir(char **dir) {
    if (!dir[1]) {
        struct passwd *pw = getpwuid(getuid());
        if (!pw) {
            perror("getpwuid failed");
            return -1;
        }
        return chdir(pw->pw_dir); // Change to home directory
    } 
    return chdir(dir[1]); // Change to specified directory
}

// Parse command line into arguments
char **cmd_parse(const char *line) {
    if (!line) return NULL;

    int arg_count = 0;
    char **args = malloc((sysconf(_SC_ARG_MAX) / 2) * sizeof(char *));
    if (!args) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(strdup(line), " ");
    while (token) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL; // NULL-terminated array

    return args;
}

// Free allocated command memory
void cmd_free(char **line) {
    if (!line) return;
    for (int i = 0; line[i] != NULL; i++) {
        free(line[i]);  // Free each token allocated by strdup
    }
    free(line);  // Free the array itself
}


// Trim leading and trailing whitespace
char *trim_white(char *line) {
    if (!line) return NULL;

    // Trim leading whitespace
    while (isspace((unsigned char)*line)) line++;

    // Trim trailing whitespace
    char *end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) *end-- = '\0';

    return line;
}

bool do_builtin(struct shell *sh, char **argv)
{
    UNUSED(sh);

    if (argv[0] == NULL)
    {
        return false;
    }

    // Handle "exit" command
    if (strcmp(argv[0], "exit") == 0)
    {
        exit(0);
    }

    // Check if last argument is "&" (background process)
    int i = 0;
    bool background = false;

    while (argv[i] != NULL)
    {
        i++;
    }

    if (i > 0 && strcmp(argv[i - 1], "&") == 0)
    {
        background = true;
        argv[i - 1] = NULL; // Remove "&" from command
    }

    // Fork and execute the command
    pid_t pid = fork();

    if (pid == 0) // Child process
    {
        setpgid(0, 0); // Set a new process group for background processes
        execvp(argv[0], argv);
        perror("exec failed");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) // Parent process
    {
        if (!background)
        {
            int status;
            waitpid(pid, &status, 0); // Wait for foreground processes
        }
        else
        {
            printf("[Background process started] PID: %d\n", pid);
        }
    }
    else
    {
        perror("fork failed");
    }

    return true;
}

// Initialize the shell environment
void sh_init(struct shell *sh)
{
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive)
    {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))
            kill(-sh->shell_pgid, SIGTTIN);

        sh->shell_pgid = getpid();
        setpgid(sh->shell_pgid, sh->shell_pgid);
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, handle_sigchld); // Handle background processes
    }

    sh->prompt = get_prompt("SHELL_PROMPT");
}


// Destroy shell and free memory
void sh_destroy(struct shell *sh) {
    if (sh->prompt) {
        free(sh->prompt);
    }
}

// Parse command-line arguments (if needed)
void parse_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)
        {
            printf("Shell version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
            exit(0);
        }
    }
}

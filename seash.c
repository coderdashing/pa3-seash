#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>

void usage_error() {
    fprintf(stderr, "seash: Usage error\n");
}

int write_history(const char *filepath, const char *command) {
    FILE *f = fopen(filepath, "a");
    if (f == NULL || ferror(f)) {
        perror("open");
        exit(1);
    }
    fprintf(f, "%s\n", command);
    fclose(f);
    return 0;
}

int read_history(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (f == NULL || ferror(f)) {
        perror("open");
        exit(1);
    }
    char buffer[2048];
    int i = 1;
    while (fgets(buffer, sizeof(buffer), f) != NULL) {
        printf("%d %s", i, buffer);
        i++;
    }
    fclose(f);
    return 0;
}

int run_cd(int cmd_argc, char *path, char *lastcd, int *has_prev_cd) {
    if (cmd_argc != 2 || path == NULL) {
        usage_error();
        return 1;
    }

    // FIXX#4 (MEDIUM - Built-in `cd` and `cd -`):
    // 1. Save current directory BEFORE calling chdir():
    //    `char curr[1024]; getcwd(curr, sizeof(curr));`
    // 2. If `strcmp(path, "-") == 0`:
    //    - If `!*has_prev_cd` (no prior `cd` in this session):
    //      print `printf("%s\n", curr);` and return 0.
    //    - Else: call `chdir(lastcd)`.
    //      * If `!= 0`: call `perror("cd");` and return 1.
    //      * If `== 0`: print `printf("%s\n", lastcd);`, copy `curr` into `lastcd` (`strcpy(lastcd, curr)`), and return 0.
    // 3. Otherwise (normal path): call `chdir(path)`.
    //    - If `!= 0`: call `perror("cd");` and return 1.
    //    - If `== 0`: copy `curr` into `lastcd`, set `*has_prev_cd = 1;`, and return 0.
    return 0;
}

int run_exit(int cmd_argc) {
    if (cmd_argc != 1) {
        usage_error();
        return 1;
    }
    exit(EXIT_SUCCESS);
}

int fork_run_command(const char command[], char *args[], int isbg) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        execvp(args[0], args);
        perror(command);
        exit(EXIT_FAILURE);
    } else if (isbg == 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char buffer[1001];
    FILE *input = stdin;
    int isShell = 0;

    if (argc == 2) {
        isShell = 1;
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror(argv[1]);
            exit(1);
        }
    } else if (argc > 2) {
        fprintf(stderr, "Usage: ./seash [script]\n");
        exit(1);
    }

    int lastexit = 0;
    int has_prev_cmd = 0;
    char lastcd[1001] = "";
    int has_prev_cd = 0;
    char cwd[1024];

    char hist_path[1024];
    snprintf(hist_path, sizeof(hist_path), "%s/.seash_history", getpwuid(getuid())->pw_dir);

    while (1) {
        if (!isShell) {
            struct passwd *pw = getpwuid(getuid());
            if (getcwd(cwd, sizeof(cwd)) != NULL && pw != NULL) {
                printf("%s@🐚 %s$ ", pw->pw_name, cwd);
                fflush(stdout);
            }
        }

        if (fgets(buffer, sizeof(buffer), input) == NULL) {
            if (input != stdin) fclose(input);
            exit(EXIT_SUCCESS);
        }

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        char *cmd_argv[64];
        int cmd_argc = 0;
        char *token = strtok(buffer, " \t");
        while (token != NULL && cmd_argc < 63) {
            cmd_argv[cmd_argc++] = token;
            token = strtok(NULL, " \t");
        }
        cmd_argv[cmd_argc] = NULL;

        if (cmd_argc > 0) {
            const char *command = cmd_argv[0];

            // FIXX#5 (MEDIUM - Clean History Logging in Interactive Mode):
            // When `!isShell`, build a buffer `char clean_cmd[1001] = "";` by joining
            // `cmd_argv[0]` through `cmd_argv[cmd_argc - 1]` separated by single spaces `" "`
            // (so `"echo   a\t b"` becomes `"echo a b"`), then call:
            // `write_history(hist_path, clean_cmd);`

            if (strcmp(command, "$?") == 0) {
                // FIXX#8 (HIGH - Resubmission `$?` Command):
                // 1. If `!has_prev_cmd || cmd_argc != 1`: call `usage_error();`
                // 2. Else: print `printf("%d\n", lastexit);`
                // 3. IMPORTANT: Do NOT overwrite `lastexit` or `has_prev_cmd` here!
            }
            else if (strcmp(command, "exit") == 0) {
                lastexit = run_exit(cmd_argc);
                has_prev_cmd = 1;
            }
            else if (strcmp(command, "cd") == 0) {
                lastexit = run_cd(cmd_argc, (cmd_argc >= 2) ? cmd_argv[1] : NULL, lastcd, &has_prev_cd);
                has_prev_cmd = 1;
            }
            else if (strcmp(command, "history") == 0) {
                has_prev_cmd = 1;
                if (cmd_argc == 1) {
                    lastexit = read_history(hist_path);
                } else if (cmd_argc == 2 && strcmp(cmd_argv[1], "-c") == 0) {
                    FILE *f = fopen(hist_path, "w");
                    if (f == NULL) {
                        perror("open");
                        exit(1);
                    }
                    fclose(f);
                    lastexit = 0;
                } else {
                    usage_error();
                    lastexit = 1;
                }
            }
            else {
                int isbg = 0;
                if (cmd_argc > 1 && strcmp(cmd_argv[cmd_argc - 1], "&") == 0) {
                    isbg = 1;
                    cmd_argv[--cmd_argc] = NULL;
                }
                lastexit = fork_run_command(command, cmd_argv, isbg);
                has_prev_cmd = 1;
            }
        }
    }

    return 0;
}


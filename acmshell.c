#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ACMShell_RL_BUFSIZE 1024
#define ACMShell_TOK_BUFSIZE 64
#define ACMShell_TOK_DELIM " \t\n"
#define HISTORY_SIZE 10
#define MAX_ALIAS_COUNT 10
#define MAX_ALIAS_LENGTH 50

char *ACMShell_read_line()
{
    int bufsize = ACMShell_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer)
    {
        fprintf(stderr, "ACMShell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();

        if (c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            buffer[position] = c;
        }

        position++;

        if (position >= bufsize)
        {
            bufsize += ACMShell_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);

            if (!buffer)
            {
                fprintf(stderr, "ACMShell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **ACMShell_split_line(char *line)
{
    int bufsize = ACMShell_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "ACMShell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, ACMShell_TOK_DELIM);

    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += ACMShell_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));

            if (!tokens)
            {
                fprintf(stderr, "ACMShell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, ACMShell_TOK_DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}

int ByteShell_launch(char **args)
{
    pid_t pid;
    int status;

    pid = fork();

    if (pid == -1)
    {

        perror("ACMShell");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // Child process
        if (execvp(args[0], args) == -1)
        {

            perror("ACMShell");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // Parent process
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int ByteShell_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "ACMShell: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("ACMShell");
        }
    }
    return 1;
}

struct Node
{
    char *str;
    struct Node *next;
};

struct Node *head = NULL;
struct Node *cur = NULL;

void add_to_hist(char **args)
{
    if (head == NULL)
    {
        head = (struct Node *)malloc(sizeof(struct Node));
        head->str = (char *)malloc(1000);
        strcpy(head->str, args[0]);

        head->next = NULL;
        cur = head;
    }
    else
    {
        struct Node *ptr = (struct Node *)malloc(sizeof(struct Node));
        cur->next = ptr;
        ptr->str = (char *)malloc(1000);
        strcpy(ptr->str, args[0]);

        ptr->next = NULL;
        cur = ptr;
    }

    if (HISTORY_SIZE > 0)
    {
        int count = 0;
        struct Node *node = head;
        struct Node *prev = NULL;

        while (node != NULL)
        {
            count++;

            if (count > HISTORY_SIZE)
            {
                if (prev == NULL)
                {
                    head = node->next;
                    free(node->str);
                    free(node);
                    node = head;
                }
                else
                {
                    prev->next = node->next;
                    free(node->str);
                    free(node);
                    node = prev->next;
                }
            }
            else
            {
                prev = node;
                node = node->next;
            }
        }
    }
}

int display_history(char **args)
{
    struct Node *ptr = head;
    int i = 1;

    while (ptr != NULL)
    {
        printf("%d %s\n", i++, ptr->str);
        ptr = ptr->next;
    }

    return 1;
}
char *builtin_str[] = {
    "cd",
    "help",
    "history",
    "ls",
    "echo",
    "alias",
    "logout"};

int ACMShell_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int ACMShell_help(char **args)
{
    int i;
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built-in commands:\n");
    for (i = 0; i < ACMShell_num_builtins(); i++)
    {
        printf("%s\n", builtin_str[i]);
    }
    return 1;
}

typedef struct
{
    char alias[MAX_ALIAS_LENGTH];
    char command[MAX_ALIAS_LENGTH];
} Alias;

Alias alias_list[MAX_ALIAS_COUNT];
int alias_count = 0;

void ACMShell_alias(char **args)
{
    if (args[1] == NULL || args[2] == NULL)
    {
        printf("Invalid input\n");
        return;
    }

    if (alias_count >= MAX_ALIAS_COUNT)
    {
        printf("limit reached.\n");
        return;
    }

    strcpy(alias_list[alias_count].alias, args[1]);
    strcpy(alias_list[alias_count].command, args[2]);
    alias_count++;
}

char *resolve(char *command)
{
    for (int i = 0; i < alias_count; i++)
    {
        if (strcmp(command, alias_list[i].alias) == 0)
        {
            return alias_list[i].command;
        }
    }
    return command;
}

int ByteShell_execute(char **args)
{
    if (args[0] == NULL)
    {

        return 1;
    }

    char *command = resolve(args[0]);

    if (strcmp(command, "cd") == 0)
    {
        return ByteShell_cd(args);
    }

    if (strcmp(command, "history") == 0)
    {
        return display_history(args);
    }

    if (strcmp(command, "help") == 0)
    {
        return ACMShell_help(args);
    }

    if (strcmp(command, "alias") == 0)
    {
        ACMShell_alias(args);
        return 1;
    }

    if (strcmp(command, "logout") == 0)
    {
        return 0;
    }

    return ByteShell_launch(args);
}

int main()
{
    char *line;
    char **args;
    int status;

    do
    {
        printf("> ");
        line = ACMShell_read_line();
        args = ACMShell_split_line(line);
        add_to_hist(args);
        status = ByteShell_execute(args);

        free(line);
        free(args);
    } while (status);

    return 0;
}

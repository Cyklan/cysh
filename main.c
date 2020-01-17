#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void cysh_loop(void);
char *cysh_read_line(void);
char **cysh_split_line(char *line);
int cysh_launch(char **args);
int cysh_execute(char **args);

// function declaration for built in shell commands
int cysh_cd(char **args);
int cysh_help(char **args);
int cysh_exit(char **args);

char *builtin_str[] = {
    "cd",
    "help",
    "exit"};

int (*builtin_func[])(char **) = {
    &cysh_cd,
    &cysh_help,
    &cysh_exit};

int cysh_num_builtins()
{
  return sizeof(builtin_str) / sizeof(char *);
}

int main(int argc, char **argv)
{
  // Load config files and set up

  // execute command loop
  cysh_loop();

  return EXIT_SUCCESS;
}

// shell lifecycle loop
void cysh_loop(void)
{
  char *line;
  char **args;
  int status;
  char cwd[PATH_MAX];

  do
  {
    if (getcwd(cwd, sizeof(cwd)) == NULL)
      printf("getcwd error\n");

    printf("%s> ", cwd);
    // content of typed line
    line = cysh_read_line();

    // array of all arguments, including command
    args = cysh_split_line(line);

    // executing command with its arguments
    status = cysh_execute(args);
    // status = 0;

    free(line);
    free(args);

  } while (status);
}

// shell line input
#define CYSH_RL_BUFSIZE 1024
char *cysh_read_line(void)
{
  // preallocated memory buffer for input
  // if userinput exceeds memory buffer, we'll just allocate more!
  int buffersize = CYSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * buffersize);
  int c;

  if (!buffer)
  {
    fprintf(stderr, "cysh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    // read character
    c = getchar();

    // if c is EOF, replace it with null terminating character \0
    // also, return it! It's our line input

    if (c == EOF || c == '\n')
    {
      buffer[position] = '\0';
      return buffer;
    }
    else
    {
      buffer[position] = c;
    }

    position++;

    // if buffer is full, make buffer bigger! (╯°□°）╯︵ ┻━┻

    if (position >= buffersize)
    {
      buffersize += CYSH_RL_BUFSIZE;
      buffer = realloc(buffer, buffersize);

      if (!buffer)
      {
        fprintf(stderr, "cysh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

// command parser
#define CYSH_TOK_BUFSIZE 64
#define CYSH_TOK_DELIM " \t\r\n\a"
char **cysh_split_line(char *line)
{
  int buffersize = CYSH_TOK_BUFSIZE;
  int position = 0;
  char **tokens = malloc(buffersize * sizeof(char *));
  char *token;

  if (!tokens)
  {
    fprintf(stderr, "cysh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  // strtok trennt string anhand eines oder mehrerer delimeter
  // gibt pointer zu string und setzt \0 bei delimetern
  token = strtok(line, CYSH_TOK_DELIM);
  while (token != NULL)
  {
    // alle tokens werden in einem array gespeichert
    tokens[position] = token;
    position++;

    if (position >= buffersize)
    {
      buffersize += CYSH_TOK_BUFSIZE;
      tokens = realloc(tokens, buffersize * sizeof(char *));
      if (!tokens)
      {
        fprintf(stderr, "cysh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, CYSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int cysh_launch(char **args)
{
  __pid_t pid;
  // __pid_t wpid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // Child process
    if (execvp(args[0], args) == -1)
    {
      perror("cysh");
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    //forking error
    perror("cysh");
  }
  else
  {
    // parent process
    do
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int cysh_execute(char **args)
{
  if (args[0] == NULL)
  {
    return 1;
  }

  for (int i = 0; i < cysh_num_builtins(); i++)
  {
    if (strcmp(args[0], builtin_str[i]) == 0)
    {
      return (*builtin_func[i])(args);
    }
  }

  return cysh_launch(args);
}

int cysh_cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "cysh: expected argument to \"cd\"\n");
  }
  else
  {
    char *dir;
    if (args[1][strlen(args[1]) - 1] == *"/")
    {
      dir = args[1];
    }
    else
    {
      dir = strcat(args[1], "/");
    }

    if (chdir(dir) != 0)
    {
      perror("cysh");
    }
  }

  return 1;
}

int cysh_help(char **args)
{
  printf("Cyklan's CySH\n");
  printf("Type program names and arguments, and hit enter\n");
  printf("The follwing commands are built in:\n");

  for (int i = 0; i < cysh_num_builtins(); i++)
  {
    printf("\t%s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int cysh_exit(char **args)
{
  return 0;
}
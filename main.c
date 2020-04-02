#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define max_line 100
#define max_word 10
#define delim " "  // delimetar
typedef unsigned char uint_8;

uint_8 RI_flag = 0;  // redirection input

uint_8 RO_flag = 0;  // redirection output

uint_8 PI_flag = 0;  // pipline

char *F_INPUT = NULL;   // create my input file to communicate to program
char *F_OUTPUT = NULL;  // create my output file to communicate to program

// remove end of line
void remove_end_of_line(char line[max_line]) {
  int index = 0;
  while (line[index] != '\n' && index < max_line) {
    index++;
  }
  line[index] = '\0';
}

//#define LSH_RL_BUFSIZE 1024

// read line
char *get_line(char line[]) {
  /*int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
   line = malloc(sizeof(char) * bufsize);
  int c;

  if (!line) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      line[position] = '\0';
      return line;
    } else {
      line[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      line = realloc(line, bufsize);
      if (!line) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }*/

  printf("user.bash: ");
  char *ret = fgets(line, max_line, stdin);
  remove_end_of_line(ret);
  if (strcmp(line, "exit") == 0 || ret == NULL) {
    exit(0);
  }
  return ret;
}

// splitting line
char **spl_line(char line[]) {
  char **temp = (char **)malloc(max_word * sizeof(*temp));
  int index = 0;
  temp[index] = strtok(line, delim);

  while (temp[index] != NULL) {
    temp[++index] = strtok(NULL, delim);
  }
  return temp;
}

// check check_piping
int check_PIPERED(char *temp[]) {
  int index = 0;

  while (temp[index] != NULL) {
    if (strcmp(temp[index], "|") == 0) {
      PI_flag = 1;
      return index;
    } else if (strcmp(temp[index], ">") == 0) {
      RO_flag = 1;
      F_OUTPUT = temp[index + 1];
      return index;
    } else if (strcmp(temp[index], "<") == 0) {
      RI_flag = 1;
      F_INPUT = temp[index + 1];
      return index;
    }

    index++;
  }

  return index;
}
// line checking function
void check_line(char *temp[]) {
  uint_8 index = 0;
  uint_8 pip_count = 0;
  uint_8 RI_count = 0;
  uint_8 RO_count = 0;

  if (temp[index] == NULL) {
    printf("NO Command\n");
    return;
  }

  while (temp[index] != NULL) {
    if (strcmp(temp[index], "<")) {
      RI_count++;
    }
    if (strcmp(temp[index], ">")) {
      RO_count++;
    }
    if (strcmp(temp[index], "|")) {
      pip_count++;
    }

    index++;
  }

  // if ((RI_count + RO_count + pip_count) != 1) {
  //   printf("ERROR SEGMENTATION\n");
  //   temp[index] = NULL;
  // }
}

// get_process_line
int get_process_line(char *arg[], char *line, char *pip_args[]) {
  uint_8 P_pos;
  uint_8 index = 0;
  uint_8 jndex = 0;
  line = get_line(line);

  char **temp = spl_line(line);

  // check_line(temp);
  P_pos = check_PIPERED(temp);
  while (index < P_pos) {
    arg[index] = temp[index];
    index++;
  }
  arg[index] = NULL;
  index++;
  if (PI_flag == 1) {
    while (temp[index] != NULL) {
      pip_args[jndex] = temp[index];
      index++;
      jndex++;
    }
  }
  free(temp);
  return 1;
}

// pip_handling function
void pip_handling(char *arg[], char *pip_arg[], int pipefd[]) {
  pid_t pid;
  uint_8 index;

  pid = fork();
  if (pid == 0) {
    dup2(pipefd[1], 1);
    close(pipefd[0]);
    execvp(arg[0], arg);
    perror(arg[0]);
  } else {
    dup2(pipefd[0], 0);
    close(pipefd[1]);
    execvp(pip_arg[0], pip_arg);
    perror(pip_arg[0]);
  }
}

int main(void) {
  char line[max_line];
  char *arg[max_word];
  char *pip_args[max_word];
  int pipefd[2];  // pipe file discrebtor
  pipe(pipefd);
  while (get_process_line(arg, line, pip_args) == 1) {
    pid_t pid_ret = fork();
    pid_t pidW;
    // it is a function which copy all aprogram and return in parent and chiled
    if (pid_ret == 0) {
      // return 0 if in child_pid
      // in child_pid
      // function treplace stdin and put my file reference and return pointer to
      // my file (0)
      if (RI_flag == 1 && F_INPUT != NULL)
        dup2(open(F_INPUT, O_RDWR | O_CREAT, 0777), 0);
      // function treplace stdout and put my file reference and return pointer
      // to my file (0)
      if (RO_flag == 1 && F_OUTPUT != NULL)
        dup2(open(F_OUTPUT, O_RDWR | O_CREAT, 0777), 1);
      // function of pipe handiling
      if (PI_flag == 1) {
        pip_handling(arg, pip_args, pipefd);

        exit(0);
      }
      // it is a function which give the dierection of my program and return
      // output and close it
      execvp(arg[0], arg);

    } else {
      // else in parent
      // in parent_pid
      do {
        pidW = waitpid(pid_ret, 0, WUNTRACED);
      } while (!WIFEXITED(0) && !WIFSIGNALED(0));

      RO_flag = 0;
      RI_flag = 0;
      F_INPUT = NULL;
      F_OUTPUT = NULL;
      PI_flag = 0;
    }
  }
  return 0;
}
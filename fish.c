#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "cmdline.h"
#include "list.h"

#define BUFLEN 2048

#define YESNO(i) ((i) ? "Y" : "N")

void handler(int sig);

struct list pids; // list of background processes pid

int main() {
  struct line li;
  char buf[BUFLEN];

  line_init(&li);
  
  struct sigaction action;
  struct sigaction old;
  action.sa_handler = SIG_IGN;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    action.sa_handler = &handler;
    if (sigaction(SIGCHLD, &action, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    char *here = getcwd(NULL, 0);
    
    list_create(&pids);
    int **pipes = NULL;
  for (;;) {
    sigaddset(&action.sa_mask, SIGCHLD);
    if (sigprocmask(SIG_SETMASK, &(action.sa_mask), &(old.sa_mask)) == -1) {
        perror("sigprocmask");
        exit(1);
    }
    printf("%s> ", here);
    if (fgets(buf, BUFLEN, stdin) == NULL) {
        perror("fgets");
        exit(1);
    }
    
    if (sigprocmask(SIG_SETMASK, &(old.sa_mask), NULL) == -1) {
        perror("sigprocmask");
        exit(1);
    }  

    int err = line_parse(&li, buf);
    if (err) { 
      //the command line entered by the user isn't valid
      line_reset(&li);
      continue;
    }

    
    if (li.cmds[0].args[0] == NULL) {
        line_reset(&li);
        continue;
    }
    
    if (strcmp(li.cmds[0].args[0], "exit") == 0) {
        line_reset(&li);
        break;
    } else if (strcmp(li.cmds[0].args[0], "cd") == 0) {
        size_t len_dir = strlen("home") + strlen(getenv("USER")) + 1;
        char dir[len_dir];
        for (size_t i = 0; i < len_dir; ++i) {
            dir[i] = '\0';
        }
        if (li.cmds[0].nargs < 2) {
            strcpy(dir, "~");
        } else {
            strcpy(dir, li.cmds[0].args[1]);
        }
        if (strcmp(dir, "~") == 0) {
            char *user = getenv("USER");
            strcpy(dir, "/home/");
            strcat(dir, user);
        }
        if (chdir(dir) == -1) {
            perror("chdir");
            fprintf(stderr, "Couldn't change the directory to %s\n", dir);
            line_reset(&li);
            continue;
        }
        free(here);
        here = getcwd(NULL, 0);
    } else {
      if (li.ncmds >= 2) {
            pipes = calloc(li.ncmds - 1, sizeof(int *));
            for (size_t i = 0; i < li.ncmds - 1; ++i) {
                pipes[i] = calloc(2, sizeof(int));
                if (pipe(pipes[i]) == -1) {
                    perror("pipe");
                    exit(1);
                }
            }
      }
      if (sigprocmask(SIG_SETMASK, &(action.sa_mask), &(old.sa_mask)) == -1) {
        perror("sigprocmask");
        exit(1);
      }
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            line_reset(&li);
            continue;
        }
        
        if (pid == 0) {
            if (!li.background) {
                action.sa_handler = SIG_DFL;
                sigemptyset(&action.sa_mask);
                
                if (sigaction(SIGINT, &action, NULL) == -1) {
                    perror("sigaction");
                    exit(5);
                }
            }
            
            if (li.ncmds >= 2) {
                if (dup2(pipes[li.ncmds - 2][0], 0) == -1) {
                    perror("dup2");
                    exit(1);
                }
            }
            
            for (size_t i = 0; i < li.ncmds - 1; ++i) {
                if (close(pipes[i][0]) == -1) {
                    perror("close");
                    exit(1);
                }
                if (close(pipes[i][1]) == -1) {
                    perror("close");
                    exit(1);
                }
            }
            
            if (li.redirect_input) {
                int input = open(li.file_input, O_CREAT | O_RDONLY, S_IRWXU);
                if (input == -1) {
                    perror("open");
                    exit(1);
                }
                if (dup2(input, 0) == -1) {
                    perror("dup2");
                    exit(1);
                }
                if (close(input) == -1) {
                    perror("close");
                    exit(1);
                }
            } else if (li.background && li.ncmds < 2) {
                int nothing = open("/dev/null", O_CREAT | O_RDONLY, S_IRWXU);
                if (nothing == -1) {
                    perror("open");
                    exit(1);
                }
                if (dup2(nothing, 0) == -1) {
                    perror("dup2");
                    exit(1);
                }
                if (close(nothing) == -1) {
                    perror("close");
                    exit(1);
                }
            }
            
            if (li.redirect_output) {
                int output = open(li.file_output, O_CREAT | O_WRONLY, S_IRWXU);
                if (output == -1) {
                    perror("open");
                    exit(1);
                }
                if (dup2(output, 1) == -1) {
                    perror("dup2");
                    exit(1);
                }
                if (close(output) == -1) {
                    perror("close");
                    exit(1);
                }
            }
            execvp(li.cmds[li.ncmds - 1].args[0], li.cmds[li.ncmds - 1].args);
            perror("execvp");
            fprintf(stderr, "Command %s not found\n", li.cmds[li.ncmds - 1].args[0]);
            exit(1);
        }
        
        if (li.ncmds >= 2) {
            for (size_t i = 0; i < li.ncmds - 1; ++i) {
                list_add(&pids, fork());
                if (pids.first->value == -1) {
                    perror("fork");
                    line_reset(&li);
                    free(pids.first);
                    continue;
                }
                if (pids.first->value == 0) {
                    if (i == 0) {
                        if (li.redirect_input) {
                            int input = open(li.file_input, O_CREAT | O_RDONLY, S_IRWXU);
                            if (input == -1) {
                                perror("open");
                                exit(1);
                            }
                            if (dup2(input, 0) == -1) {
                                perror("dup2");
                                exit(1);
                            }
                            if (close(input) == -1) {
                                perror("close");
                                exit(1);
                            }
                        } else if (li.background) {
                            int nothing = open("/dev/null", O_CREAT | O_RDONLY, S_IRWXU);
                            if (nothing == -1) {
                                perror("open");
                                exit(1);
                            }
                            if (dup2(nothing, 0) == -1) {
                                perror("dup2");
                                exit(1);
                            }
                            if (close(nothing) == -1) {
                                perror("close");
                                exit(1);
                            }
                        }
                    }
                    
                    if (i != 0) {
                        if (dup2(pipes[i - 1][0], 0) == -1) {
                            perror("dup2");
                            exit(1);
                        }
                    }
                    if (close(pipes[i][0]) == -1) {
                        perror("close");
                        exit(1);
                    }
                    if (dup2(pipes[i][1], 1) == -1) {
                        perror("dup2");
                        exit(1);
                    }
                    if (close(pipes[i][1]) == -1) {
                        perror("close");
                        exit(1);
                    }
                    
                    for (size_t j = 0; j < li.ncmds - 1; ++j) {
                        if (i != j) {
                            if (close(pipes[j][0]) == -1) {
                                perror("close in loop");
                                exit(1);
                            }
                            if (close(pipes[j][1]) == -1) {
                                perror("close in loop");
                                exit(1);
                            }
                        }
                    }
                    execvp(li.cmds[i].args[0], li.cmds[i].args);
                    perror("execvp");
                    fprintf(stderr, "Command %s not found\n", li.cmds[i].args[0]);
                    exit(1);
                }
            }
           
        }
        
        for (size_t i = 0; i < li.ncmds - 1; ++i) {
            if (close(pipes[i][1]) == -1) {
                perror("close");
                exit(1);
            }
                    
            if (close(pipes[i][0]) == -1) {
                perror("close");
                exit(1);
            }
        }
        
        int status;
        
        if (!li.background) {
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                exit(1);
            }
            
            if (WIFEXITED(status)) {
                printf("Child process normally terminated (return code %d)\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Child process terminated by signal %d\n", WTERMSIG(status));
            }
        } else {
            list_add(&pids, pid);
        }
      }
      
      if (sigprocmask(SIG_SETMASK, &(old.sa_mask), NULL) == -1) {
        perror("sigprocmask");
        exit(1);
      }
      
      if (li.ncmds >= 2) {
        for (size_t i = 0; i < li.ncmds - 1; ++i) {
            free(pipes[i]);
        }
        free(pipes);
      }
      
      line_reset(&li);
 
  }
  free(here);
  return 0;
}

/* captures signals and terminates children in the background when they are over
int sig the captured signal's identifier
*/
void handler(int sig) {
    if (sig == SIGCHLD) {
        if (pids.first == NULL) {
            return;
        }
        struct node *current = pids.first;
        struct node *prev = pids.first;
        
        while (current != NULL) {
            if (waitpid(current->value, NULL, WNOHANG | WUNTRACED) != 0) {
                if (current == pids.first) {
                    pids.first = current->next;
                    free(current);
                    current = pids.first;
                } else {
                    prev->next = current->next;
                    free(current);
                    current = prev;
                }
            } else {
                prev = current;
                current = current->next;
            }
        }
    }
}


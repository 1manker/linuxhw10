/*************************************
 * wyshell.c
 * Author: Lucas Manker
 * Date: 30 April 2020
 *
 * scanner for 3750 wyshell project.
 * Parts of this code graciously provided by Dr. Buckner
 ************************************/

#include<stdio.h>
#include"wyscanner.h"
#include<sys/types.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/wait.h>

void execute(char **argv);
void executeWOorder(char ** argv);
void executePipe(char** argsFirst, char** argsSecond);


int main()
{
  char *tokens[]={ "QUOTE_ERROR", "ERROR_CHAR", "SYSTEM_ERROR",
                "EOL", "REDIR_OUT", "REDIR_IN", "APPEND_OUT",
                "REDIR_ERR", "APPEND_ERR", "REDIR_ERR_OUT", "SEMICOLON",
                "PIPE", "AMP", "WORD" };
  int rtn;
  char *rpt;
  char buf[1024];
  int command = 0;
  int redirCount = 0;
  int lastCharIn = 0;
  int lastCharOut = 0;
  int redirIcount = 0;
  int errorOnLine = 0;
  int expFile = 0;
  char** argsv[64];
  char* args[64];
  char* outargs[64];
  char* inargs[64];
  int pipes[64];
  int argsc = 0;
  int argsvc = 0;
  int ampFound = 0;
  int save_out;
  int save_in;
  int out;
  int in;
  printf("$> ");
  while(1) {
    errorOnLine = 0;
    redirCount = 0;
    redirIcount = 0;
    command = 0;
    rpt=fgets(buf,256,stdin);
    if(rpt == NULL) {
      if(feof(stdin)) {
	return 0;
      }
      else {
	perror("fgets from stdin");
	return 1;
      }
    }
    rtn=parse_line(buf);
    while(rtn !=  EOL && !errorOnLine){
      switch(rtn) {
        case WORD:
          if(expFile){
              expFile = 0;
          }
          if(lastCharOut){
              outargs[argsvc] = malloc(strlen(lexeme+1));
              for(int i = 0; i < strlen(lexeme); i++){
                  outargs[argsvc][i] = lexeme[i];
              }
              lastCharOut = 0;
              break;
          }
          if(lastCharIn){
              inargs[argsvc] = malloc(strlen(lexeme)+1);
              for(int i = 0; i < strlen(lexeme); i++){
                  inargs[argsvc][i] = lexeme[i];
              }
              lastCharIn = 0;
              break;
          }
          if(!command){
            args[argsc] = malloc(strlen(lexeme)+1);
            for(int i = 0; i < strlen(lexeme); i++){
                args[argsc][i] = lexeme[i];
            }
            args[argsc][strlen(lexeme)+1]='\0';
            command = 1;
            pipes[argsvc] = 0;
            argsc++;
            break;
          }
          else{
            args[argsc] = malloc(strlen(lexeme)+1);
            for(int i = 0; i < strlen(lexeme); i++){
                args[argsc][i] = lexeme[i];
            }
            args[argsc][strlen(lexeme)+1]='\0';            
            argsc++;
          }
          break;
        case SEMICOLON:
          if(expFile){
              printf("missing filename after redirection");
              errorOnLine = 1;
              break;
          }
          argsv[argsvc] = malloc(sizeof(args));
          for(int i = 0; i < argsc; i++){
              argsv[argsvc][i] = malloc(strlen(args[i])+1);
              for(int j = 0; j < strlen(args[i]); j++){
                argsv[argsvc][i][j] = args[i][j];
              }
              argsv[argsvc][i][strlen(args[i])+1] = '\0';
          }
          argsvc++;
          for(int i = 0; i < argsc; i++){
              args[i] = '\0';
          }
          argsc = 0;
          command = 0;
          redirCount = 0;
          redirIcount = 0;
          ampFound = 0;
          break;
        case PIPE:
          if(expFile){
              printf("missing filename after redirection");
              errorOnLine = 1;
              break;
          }
          if(!command || ampFound){
              printf("syntax error near '|'");
              errorOnLine = 1;
              break;
          }
          pipes[argsvc] = 1;
          argsv[argsvc] = malloc(sizeof(args));
          for(int i = 0; i < argsc; i++){
              argsv[argsvc][i] = malloc(strlen(args[i])+1);
              for(int j = 0; j < strlen(args[i]); j++){
                argsv[argsvc][i][j] = args[i][j];
              }
              argsv[argsvc][i][strlen(args[i])+1] = '\0';
          }
          argsvc++;
          for(int i = 0; i < argsc; i++){
              args[i] = '\0';
          }
          argsc = 0;
          command = 0;
          redirCount = 0;
          redirIcount = 0;
          break;
        case AMP:
          if(expFile){
              printf("missing filename after redirection");
              errorOnLine = 1;
              break;
          }
          ampFound = 1;
          break;
        case REDIR_OUT:
          if(!command){
              printf("error near '>'");
              errorOnLine = 1;
              break;
          }
          if(expFile){
              printf("missing filename after redirection");
              errorOnLine = 1;
              break;
          }
          if(redirCount >= 1){
              printf("Ambiguous output redirection");
              errorOnLine = 1;
              break;
          }
          else{
              redirCount = 1;
              expFile = 1;
              lastCharOut = 1;
              break;
          }
        case REDIR_IN:
          if(!command){
              printf("error near '<'");
              errorOnLine = 1;
              break;
          }
          if(expFile){
              printf("missing filename after redirection");
              errorOnLine = 1;
              break;
          }
          if(redirIcount >= 1){
              printf("Ambigous input redirection");
              errorOnLine = 1;
              break;
          }
          else{
              redirIcount = 1;
              expFile = 1;
              lastCharIn = 1;
              break;
          }
        case APPEND_OUT:
           if(expFile){
              printf("missing filename after redirection");
              errorOnLine = 1;
              break;
          }
          else{
              lastCharOut = 1;
          }
          break;
        case ERROR_CHAR:
          printf("error char: %d",error_char);
          errorOnLine = 1;
          break;
        case QUOTE_ERROR:
          printf("quote error");
          errorOnLine = 1;
          break;
        case APPEND_ERR:
          if(expFile){
              printf("missing filename after redirection");
              errorOnLine = 1;
              break;
          }
          break;
        case REDIR_ERR_OUT:
          if(expFile){
              printf("missing filename after redirection");
              errorOnLine = 1;
              break;
          }
          break;
        case REDIR_ERR:
           if(expFile){
              printf("missing filename after redirection");
              errorOnLine = 1;
              break;
          }
          break;
        case SYSTEM_ERROR:
          perror("system error");
          return(1);
        
        default:
          printf("%d: %s\n",rtn,tokens[rtn%96]);
      }
      rtn=parse_line(NULL);
    }
    if(rtn == EOL && !errorOnLine){
        if(expFile){
            printf("missing filename after redirection");
            errorOnLine = 1;
            argsc = 0;
            argsc = 0;
            command = 0;
            ampFound = 0;
            printf("\n$> ");
            for(int i = 0; i<= argsc; i++){
                args[i] = '\0';
            }
            for(int i = 0; i<= argsvc; i++){
                argsv[i] = '\0';
            }
        }
        else{
            args[argsc] = '\0';
            argsv[argsvc] = args;
            for(int i = 0; i <= argsvc; i++){
                if(outargs[i] != NULL){
                    out = open(outargs[i], O_RDWR|O_CREAT|O_APPEND, 0600);
                    save_out = dup(fileno(stdout));
                    dup2(out, fileno(stdout));
                }
                if(inargs[i] != NULL){
                    in = open(inargs[i], O_RDONLY);
                    save_in = dup(fileno(stdin));
                    dup2(in, 0);
                }
                if(pipes[i]){
                    if(outargs[i+1] != NULL){
                        out = open(outargs[i+1], O_RDWR|O_CREAT|O_APPEND, 0600);
                        save_out = dup(fileno(stdout));
                        dup2(out, fileno(stdout));
                    }
                    executePipe(argsv[i], argsv[i+1]);
                    if(outargs[i+1] != NULL){
                        fflush(stdout); close(out);
                        dup2(save_out, fileno(stdout));
                        close(save_out);
                    }
                    i++;
                }
                else{
                    execute(argsv[i]);
                }
                if(outargs[i] != NULL){
                    fflush(stdout); close(out);
                    dup2(save_out, fileno(stdout));
                    close(save_out);
                }
                fflush(stdin); close(in);
                dup2(save_in, fileno(stdin));
                close(save_in);
            }
            printf("$> ");
            for(int i = 0; i <= argsc; i++){
                args[i] = '\0';
            }
            for(int i = 0; i<= argsvc; i++){
                argsv[i] = '\0';
                outargs[i] = '\0';
                inargs[i] = '\0';
            }
            argsc = 0;
            argsvc = 0;
            ampFound = 0;
            redirCount = 0;
            redirIcount = 0;
        }
        command = 0;
    }
    else{
        command = 0;
        printf("\n$> ");
        for(int i = 0; i <= argsc; i++){
            args[i] = '\0';
        }
        for(int i = 0; i<= argsvc; i++){
            argsv[i] = '\0';
        }
        argsc = 0;
        argsvc = 0;
        ampFound = 0;
        redirCount = 0;
        redirIcount = 0;
    }
  }
}

void execute(char** args){
    pid_t pid;
    int stat;
    pid = fork();
    if(pid ==0){
        if(execvp(*args, args) < 0){
            printf("wyshell: %s: command not found\n",args[0]);
            exit(1);
        }
        exit(0);
    }
    if(pid > 0){
        wait(&stat);
    }
}

void executeWOorder(char** args){
    pid_t pid;
    pid = fork();
    if(pid == 0){
        if(execvp(*args, args) < 0){
            printf("wyshell: %s: command not found\n", args[0]);
            exit(1);
        }
        exit(0);
    }
}

void executePipe(char** argsFirst, char** argsSecond){
    int des_p[2];
    if(pipe(des_p) == -1){
        perror("Pipe has failed");
        exit(1);
    }
    if(fork() == 0){
        close(STDOUT_FILENO);
        dup(des_p[1]);
        close(des_p[0]);
        close(des_p[1]);
        execvp(*argsFirst, argsFirst);
        perror("error");
        exit(1);
    }
    if(fork() == 0){
        close(STDIN_FILENO);
        dup(des_p[0]);
        close(des_p[1]);
        close(des_p[0]);
        execvp(*argsSecond, argsSecond);
        perror("error");
        exit(1);
    }
    close(des_p[0]);
    close(des_p[1]);
    wait(0);
    wait(0);
    return;
}















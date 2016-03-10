
#include <string.h>
#include <stdio.h>

void main(int argc, char argv[])
  {
  char  line[100];
  char *args[100];
  int   num_args;
  int   i;

  while (1)
    {
    printf("Enter string to tokenize: ");
    fgets(line,sizeof(line),stdin);
    if(strcmp(line, "quit\n") == 0)
      break;
    num_args = 0;
    args[num_args] = strtok(line, " ");
    while (args[num_args] != NULL)
      {
      num_args++;
      args[num_args] = strtok(NULL, " ");
      }
    num_args--;
    printf("The tokenized string prints as: \n");
    for (i = 0; i <= num_args; i++) 
      {
      printf("args[%d]= ",i);
      printf(args[i]);
      printf("\n");
      }

    }
  }

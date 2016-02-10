#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "List.h"


void MakeList(List *list, char *str) {
   char **files = list->files;
   char temp[MAX_CHARS];
   int len, num;
   
   sscanf(str, " %s%n", temp, &num);
   str = str + num;
   len = strlen(temp);
   list->fname = malloc(len + 1);
   strcpy(list->fname, temp);
   
   while (sscanf(str, " %s%n", temp, &num) != EOF && 
    list->numFiles < NUM_FILES - 1) {
      str = str + num;
      len = strlen(temp);
      *files = malloc(len + 1);
      strcpy(*files, temp);
      list->numFiles++;
      files++;
   }
   files = NULL;
}

void AddTest(List *list, char *str, int testNum) {
   char **args;
   char temp[MAX_CHARS];
   int len, num;
   Node *test = list->testList;
   
   if (!test) {
      test = list->testList = calloc(sizeof(Node), 1);
   }
   else {
      while (test->next) {
         test = test->next;
      }
      test->next = calloc(sizeof(Node), 1);
      test = test->next;
   }
   list->numTests++;
   
   test->testNum = testNum;
   
   sscanf(str, " %s%n", temp, &num);
   str = str + num;
   len = strlen(temp);
   test->input = malloc(len + 1);
   strcpy(test->input, temp);
   
   sscanf(str, " %s%n", temp, &num);
   str = str + num;
   len = strlen(temp);
   test->output = malloc(len + 1);
   strcpy(test->output, temp);
   
   sscanf(str, " %d%n", &len, &num);
   str = str + num;
   test->time = len;
   
   args = test->args;
   while (sscanf(str, " %s%n", temp, &num) != EOF && 
    test->numArgs < NUM_ARGS - 1) {
      str = str + num;
      len = strlen(temp);
      *args = malloc(len + 1);
      strcpy(*args, temp);
      test->numArgs++;
      args++;
   }
   args = NULL;
}

void FreeList(List *list) {
   Node *temp;
   int i;
   
   free(list->fname);
   
   while (list->testList) {
      free(list->testList->input);
      free(list->testList->output);
      i = 0;
      while (list->testList->args[i]) {
         free(list->testList->args[i]);
         i++;
      }
      temp = list->testList;
      list->testList = temp->next;
      free(temp);
   }
   
   i = 0;
   while (list->files[i]) {
      free(list->files[i]);
      i++;
   }
   
   free(list);
}

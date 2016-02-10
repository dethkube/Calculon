#ifndef LIST_H
#define LIST_H

#define NUM_FILES 33
#define NUM_ARGS 33
#define MAX_CHARS 32

typedef struct Node {
   int testNum;
   char *input;
   char *output;
   int time;
   char *args[NUM_ARGS];
   int numArgs;
   struct Node *next;
} Node;
   

typedef struct List {
   char *fname;
   char *files[NUM_FILES];
   int numFiles;
   Node *testList;
   int numTests;
} List;

void MakeList(List *list, char *str);

void AddTest(List *list, char *str, int testNum);

void FreeList(List *list);

#endif

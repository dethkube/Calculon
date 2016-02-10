#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include "List.h"

#define MAX_STR_LEN 300
#define PERMISSIONS 0777
#define OUT_PERM 0644
#define TIMEOUT_ERR 193
#define T_MULTIPLE 10

typedef struct dirent DirEntry;

static void NextSuite(FILE *suite, List *list) {
   char test;
   char str[MAX_STR_LEN];
   int testNum = 0;
   
   fgets(str, MAX_STR_LEN, suite);
   
   MakeList(list, str);
   
   while (test = getc(suite)) {
      if (test == 'T') {
         testNum++;
         fgets(str, MAX_STR_LEN, suite);
         AddTest(list, str, testNum);
      }
      else {
         ungetc(test, suite);
         break;
      }
   }
}

static int CopyFiles(List *list, char *dirName) {
   char *file;
   int i, status, pid, dnull;
   Node *ptr = list->testList;
   
   dnull = open("/dev/null", O_WRONLY);
   
   for (i = 0; i < list->numFiles; i++) {
      file = list->files[i];
      
      if ((pid = fork()) < 0) {
         printf("Bad fork!\n");
      }
      else if (!pid) {
         dup2(dnull, 2);
         execl("/bin/cp", "cp", file, dirName, (char *)NULL);
      }
      else {
         wait(&status);
      }
      
      if (WEXITSTATUS(status)) {
         printf("Could not find required test file '%s'\n", list->files[i]);
         return 1;
      }
   }
   
   while (ptr) {
      file = ptr->input;
      
      if ((pid = fork()) < 0) {
         printf("Bad fork!\n");
      }
      else if (!pid) {
         dup2(dnull, 2);
         execl("/bin/cp", "cp", file, dirName, (char *)NULL);
      }
      else {
         wait(&status);
      }
      
      if (WEXITSTATUS(status)) {
         printf("Could not find required test file '%s'\n", file);
         return 1;
      }
      
      file = ptr->output;
      
      if ((pid = fork()) < 0) {
         printf("Bad fork!\n");
      }
      else if (!pid) {
         dup2(dnull, 2);
         execl("/bin/cp", "cp", file, dirName, (char *)NULL);
      }
      else {
         wait(&status);
      }
      
      if (WEXITSTATUS(status)) {
         printf("Could not find required test file '%s'\n", file);
         return 1;
      }
      
      ptr = ptr->next;
   }
   return 0;
}

static void RemoveFiles(char *dirName) {
   DIR *dir;
   DirEntry *dirEntry;
   int ndx;
   
   dir = opendir(dirName);
   chdir(dirName);
   
   for (ndx = 0; NULL != (dirEntry = readdir(dir)); ) {
      if (*dirEntry->d_name != '.') {
         remove(dirEntry->d_name);
      }
   }
   chdir("..");
   closedir(dir);
   remove(dirName);
}

static int BuildProgram(List *list) {
   DIR *dir;
   DirEntry *dirEntry;
   int ndx, status, dnull;
   char *argv[MAX_CHARS];
   char **arg = argv;
   
   dir = opendir("..");
   dnull = open("/dev/null", O_WRONLY);
   
   for (ndx = 0; NULL != (dirEntry = readdir(dir)); ) {
      if (strcmp(dirEntry->d_name, "Makefile") == 0) {
         if (!fork()) {
            execl("/bin/cp", "cp", "../Makefile", ".", (char *)NULL);
         }
         else {
            wait(NULL);
         }
         
         if (!fork()) {
            dup2(1, 2);
            dup2(dnull, 1);
            execl("/usr/bin/make", "make", list->fname, (char *)NULL);
         }
         else {
            wait(&status);
         }
         if (WEXITSTATUS(status)) {
            printf("Failed: make %s\n", list->fname);
            return 1;
         }
         else {
            return 0;
         }
      }
   }
   
   closedir(dir);
   
   *arg++ = "gcc";
   for (ndx = 0; ndx < list->numFiles; ndx++)
      *arg++ = list->files[ndx];
   *arg++ = "-o";
   *arg++ = list->fname;
   *arg = NULL;
   
   if (!fork()) {
      dup2(1, 2);
      execv("/usr/bin/gcc", argv);
   }
   else {
      wait(&status);
   }
   if (WEXITSTATUS(status)) {
      printf("Failed:");
      arg = argv;
      while (*arg)
         printf(" %s", *arg++);
      printf("\n");
      return 1;
   }
   close(dnull);
   return 0;
}

static void PrintErrors(char *fname, int testnum, int srErr, int dfErr) {
   int first = 0;
   
   printf("%s test %d failed:", fname, testnum);
   if (dfErr) {
      if (first)
         printf(",");
      else
         first = 1;
      printf(" diff failure");
   }
   if (srErr && srErr != TIMEOUT_ERR) {
      if (first)
         printf(",");
      else
         first = 1;
      printf(" runtime error");
   }
   if (srErr >= TIMEOUT_ERR && srErr % 2) {
      if (first)
         printf(",");
      else
         first = 1;
      printf(" timeout");
   }
   printf("\n");
}

static void RunTests(List *list) {
   Node *curTest = list->testList;
   int i, count, ndx = 0, passes = 0, srErr, dfErr, status, wfile, rfile;
   int dnull;
   char *argv[2 * MAX_CHARS], *fout = "test.output.temp";
   char tflag[MAX_CHARS], flagT[MAX_CHARS], fplace[MAX_CHARS];
   
   argv[ndx++] = "SafeRun";
   argv[ndx++] = "-p30";
   sprintf(fplace, "./%s", list->fname);
   dnull = open("/dev/null", O_WRONLY);
   
   for (i = 0; i < list->numTests; i++) {
      sprintf(tflag, "-t%d", curTest->time);
      sprintf(flagT, "-T%d", curTest->time * T_MULTIPLE);
      argv[ndx++] = tflag;
      argv[ndx++] = flagT;
      argv[ndx++] = fplace;
      for (count = 0; count < curTest->numArgs; count++)
         argv[ndx++] = curTest->args[count];
      argv[ndx] = NULL;
      
      wfile = open(fout, O_WRONLY | O_CREAT, OUT_PERM);
      rfile = open(curTest->input, O_RDONLY);
      
      if (!fork()) {
         dup2(rfile, 0);
         close(rfile);
         dup2(wfile, 1);
         dup2(wfile, 2);
         close(wfile);
         execv("/home/grade_cstaley/bin/SafeRun", argv);
      }
      else
         wait(&status);
      
      srErr = WEXITSTATUS(status);
      
      if (!fork()) {
         dup2(dnull, 1);
         dup2(dnull, 2);
         execl("/usr/bin/diff", "diff", fout, curTest->output, (char *)NULL);
      }
      else
         wait(&status);
         
      dfErr = WEXITSTATUS(status);
      
      if ((srErr + dfErr) == 0)
         passes++;
      else
         PrintErrors(list->fname, i + 1, srErr, dfErr);
      
      close(wfile);
      close(rfile);
      
      remove(fout);
      ndx = 2;
      curTest = curTest->next;
   }
   close(dnull);
   if (passes == list->numTests)
      printf("%s %d of %d tests passed.\n", list->fname, passes, passes);
}

int main(int argc, char **argv) {
   FILE *suite;
   List *list;
   char dirName[MAX_CHARS];
   
   sprintf(dirName, ".%d", getpid());
   
   if (!(suite = fopen(*++argv, "r"))) {
      printf("No suite found or no args\n");
      return 0;
   }
   
   while (getc(suite) == 'P') {
      list = calloc(sizeof(List), 1);
      NextSuite(suite, list);
      
      mkdir(dirName, PERMISSIONS);
      
      if (CopyFiles(list, dirName)) {
         return 1;
      }
      
      chdir(dirName);
      
      if (!BuildProgram(list)) {
         
         RunTests(list);
         
      }
      
      
      chdir("..");
      
      RemoveFiles(dirName);
      
      FreeList(list);
   }
   
   
   return 0;
}

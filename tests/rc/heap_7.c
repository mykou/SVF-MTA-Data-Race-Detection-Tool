/*
 * Simple alias check
 * Author: dye
 * Date: 13/08/2015
 */
#include "aliascheck.h"
#include "pthread.h"

typedef struct {
    int f1;
    int f2;
} S;


void *bar(void *args) {
  S *p = (S*)args;

  p->f1 = 0;
  RC_ACCESS(1, RC_ALIAS | RC_MHP);
  RC_ACCESS(1, RC_ALIAS | RC_MHP);

}



void foo() {
    S *p = (S*)malloc(sizeof(S));
    pthread_t tid;
    pthread_create(&tid, NULL, bar, p);
}


int main(int argc, char *argv[]) {
  pthread_t thread;
  int ret;

  foo();
  foo();

}

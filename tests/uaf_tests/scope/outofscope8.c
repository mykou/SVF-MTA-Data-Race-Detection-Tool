#include "../uaf.h"
void goo3(int*);
int* foo1() {//1st tier malloc wrapper
  int* p1 = (int*)malloc(1);//o
  goo3(p1);
  return p1;
}
int* foo2() {//2nd tier malloc wrapper
  int* p2 = foo1();//[c1]:o
  return p2;
}
int* foo3() {//3rd tier malloc wrapper
  int* p3 = foo2();//[c2,c1]:o
  return p3;
}
void bar1(int* q1) {//1st tier use wrapper
  uaf_use(q1);
}
void bar2(int *q2) {//2st tier use wrapper
  bar1(q2);
}
void bar3(int* q3) {//3rd tier use wrapper
  bar2(q3);
}
void goo1(int* r1) {//1st tier free wrapper
  free(r1);
}
void goo2(int* r2) {//2nd tier free wrapper
  goo1(r2);
}
void goo3(int* r3) {//3rd tier free wrapper
  goo2(r3);
}
int main() {//o is in scope. call malloc, use, and free in order
  int* x = foo3();//[c3,c2,c1]:o
  bar3(x);
  goo3(x);
  UAF_TN(26,17);
  return 0;
}


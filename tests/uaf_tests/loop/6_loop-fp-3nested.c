#include "../uaf.h"
int k = 100;
void foo(int n) {
  for (int i = 0; i < n; i++) {
     for (int j = 0; j <= i; j++) {
       for (int k = 0; k <= j; k++) {
         int* p = (int*)malloc(1);
         uaf_use(p);
         free(p);
       }
     }
  }
}
int main() {
  k = 90;
  foo(k);
  UAF_TN(9,8);
  return 0;
}

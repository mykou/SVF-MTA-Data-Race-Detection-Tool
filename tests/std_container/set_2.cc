/*
 * Simple alias check
 * Author: dye
 * Date: 14/12/2016
 */
#include "aliascheck.h"
#include <set>

using namespace std;

set<int*> g;


int main(int argc, char *argv[]) {

    int n;
    int *p = &n;

    g.insert(p);

    for (set<int*>::iterator it = g.begin(), ie = g.end(); it != ie; ++it) {

        int *q = *it;

        MAYALIAS(p, q);
    }

    return 0;
}

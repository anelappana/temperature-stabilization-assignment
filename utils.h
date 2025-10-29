#ifndef UTILS_H
#define UTILS_H
#include <stdbool.h>

struct msg {
    int   Index;
    float T;
    bool  done;   // central sets this true to tell clients to stop
};

// keep your function name; add 'done' (clients send false; server sets true)
struct msg prepare_message(int Index, float T, bool done);

#endif
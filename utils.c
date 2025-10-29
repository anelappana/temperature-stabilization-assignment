#include "utils.h"

struct msg prepare_message(int Index, float T, bool done)
{
    struct msg m;
    m.Index = Index;
    m.T = T;
    m.done = done;
    return m;
}
#include <queue>
#include <cstddef>

#include "queue.h"

static std::queue<void*> q;

void* queue_get()
{
    void* item = NULL;
    if(!q.empty())
    {
        item = q.front();
        q.pop();
    }
    return item;
}

void queue_put(void* item)
{
    q.push(item);
}

int queue_len()
{
    return q.size();
}


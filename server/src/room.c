#include "room.h"
#include <string.h>

void room_manager_init(RoomManager *manager)
{
    if (manager == NULL)
    {
        return;
    }

    manager->room_count = 0;
    for (size_t i = 0; i < MAX_ROOMS; i++)
    {
        manager->occupied[i] = false;
    }
}

Room *room_find(RoomManager *manager, const char *code)
{
    if (manager == NULL || code == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < MAX_ROOMS; i++)
    {
        if (manager->occupied[i] == false)
        {
            continue;
        }

        if (strcmp(manager->rooms[i].code, code) == 0)
        {
            return &manager->rooms[i];
        }
    }

    return NULL;
}
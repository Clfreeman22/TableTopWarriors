#include "room.h"

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
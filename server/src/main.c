#include <stdio.h>
#include "room.h"

int main(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room1 = room_create(&manager, 101);
    Room *room2 = room_create(&manager, 102);
    Room *room3 = room_create(&manager, 103);

    if (room1 != NULL)
    {
        printf("Room 1: %s\n", room1->code);
    }

    if (room2 != NULL)
    {
        printf("Room 2: %s\n", room2->code);
    }

    if (room3 != NULL)
    {
        printf("Room 3: %s\n", room3->code);
    }

    printf("Total rooms: %zu\n", manager.room_count);

    return 0;
}
#include <stdio.h>
#include <string.h>

#include "room.h"

int main(void)
{
    RoomManager manager;
    
    room_manager_init(&manager);

    manager.occupied[0] = true;
    manager.room_count = 1;

    strcpy(manager.rooms[0].code, "ABC123");
    Room* found = room_find(&manager, "ABC123");

    printf("TableTopWarrior Started\n");

    if (found != NULL)
    {
        printf("Found room: %s\n", found->code);
    }
    else
    {
        printf("Room not found!\n");
    }

    return 0;
}
#include <stdio.h>

#include "room.h"

int main(void)
{
    RoomManager manager;
    
    room_manager_init(&manager);

    printf("TableTopWarrior Started\n");
    printf("Rooms Available: %zu\n", manager.room_count);

    return 0;
}
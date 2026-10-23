#include <stdio.h>
#include "room.h"

int main(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room1 = room_create(&manager, 101);
    Room *room2 = room_create(&manager, 102);
    Room *room3 = room_create(&manager, 103);

    if (room1 == NULL || room2 == NULL || room3 == NULL)
    {
        printf("Failed to create rooms!\n");
        return 1;
    }

    printf("Initial rooms: %zu\n", manager.room_count);

    // Destroy the second room.
    bool destroyed = room_destroy(&manager, room2->code);

    printf("Room destroyed: %s\n", destroyed ? "Yes" : "No");
    printf("Remaining rooms: %zu\n", manager.room_count);

    // Check that the destroyed room cannot be found.
    Room *found = room_find(&manager, "000001");

    printf("Deleted room found: %s\n", found ? "Yes" : "No");

    // Create another room to test slot reuse.
    Room *room4 = room_create(&manager, 104);

    if (room4 != NULL)
    {
        printf("New room code: %s\n", room4->code);
    }

    printf("Final room count: %zu\n", manager.room_count);

    return 0;
}

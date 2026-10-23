#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "room.h"

void test_room_manager_init(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    assert(manager.room_count == 0);

    for (size_t i = 0; i < MAX_ROOMS; i++)
    {
        assert(manager.occupied[i] == false);
    }

    printf("PASS: room_manager_init\n");
}

void test_room_create(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room = room_create(&manager, 101);

    assert(room != NULL);
    assert(manager.room_count == 1);
    assert(room->status == ROOM_WAITING);
    assert(room->player_count == 1);
    assert(room->players[0].session_id == 101);
    assert(room->players[0].connected == true);
    assert(strlen(room->code) == ROOM_CODE_LENGTH);

    printf("PASS: room_create\n");
}

void test_room_find(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room = room_create(&manager, 101);
    assert(room != NULL);

    Room *found = room_find(&manager, room->code);

    assert(found == room);
    assert(room_find(&manager, "XXXXXX") == NULL);

    printf("PASS: room_find\n");
}

void test_room_join(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room = room_create(&manager, 101);
    assert(room != NULL);

    bool joined = room_join(&manager, room, 102);

    assert(joined == true);
    assert(room->player_count == 2);
    assert(room->players[1].session_id == 102);
    assert(room->players[1].connected == true);
    assert(room->status == ROOM_SELECTING);

    printf("PASS: room_join\n");
}

void test_room_full(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room = room_create(&manager, 101);
    assert(room != NULL);

    assert(room_join(&manager, room, 102) == true);

    // A third player should not be allowed.
    assert(room_join(&manager, room, 103) == false);
    assert(room->player_count == 2);

    printf("PASS: room_full\n");
}

void test_duplicate_player(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room1 = room_create(&manager, 101);
    Room *room2 = room_create(&manager, 102);

    assert(room1 != NULL);
    assert(room2 != NULL);

    // Player 101 already owns room1.
    assert(room_join(&manager, room2, 101) == false);

    // Player 101 cannot create a second room.
    assert(room_create(&manager, 101) == NULL);

    assert(manager.room_count == 2);

    printf("PASS: duplicate_player\n");
}

void test_room_destroy(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room1 = room_create(&manager, 101);
    Room *room2 = room_create(&manager, 102);

    assert(room1 != NULL);
    assert(room2 != NULL);

    char code[ROOM_CODE_LENGTH + 1];
    strcpy(code, room1->code);

    assert(room_destroy(&manager, code) == true);
    assert(manager.room_count == 1);
    assert(room_find(&manager, code) == NULL);

    // The other room must remain intact.
    assert(room_find(&manager, room2->code) == room2);

    // Destroying the same room again should fail.
    assert(room_destroy(&manager, code) == false);

    printf("PASS: room_destroy\n");
}

void test_room_reuse(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room1 = room_create(&manager, 101);
    assert(room1 != NULL);

    char old_code[ROOM_CODE_LENGTH + 1];
    strcpy(old_code, room1->code);

    assert(room_destroy(&manager, old_code) == true);

    // The destroyed room's slot should be reusable.
    Room *room2 = room_create(&manager, 102);

    assert(room2 != NULL);
    assert(room2 == room1);
    assert(manager.room_count == 1);
    assert(room2->players[0].session_id == 102);
    assert(room2->status == ROOM_WAITING);

    printf("PASS: room_reuse\n");
}

void test_max_rooms(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    for (size_t i = 0; i < MAX_ROOMS; i++)
    {
        Room *room = room_create(&manager, (uint64_t)(i + 1));
        assert(room != NULL);
    }

    assert(manager.room_count == MAX_ROOMS);

    // The manager should reject room 51.
    assert(room_create(&manager, 999) == NULL);
    assert(manager.room_count == MAX_ROOMS);

    printf("PASS: max_rooms\n");
}

void test_invalid_inputs(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    assert(room_create(NULL, 101) == NULL);
    assert(room_create(&manager, 0) == NULL);

    assert(room_find(NULL, "000000") == NULL);
    assert(room_find(&manager, NULL) == NULL);

    assert(room_join(NULL, NULL, 101) == false);
    assert(room_join(&manager, NULL, 101) == false);

    assert(room_destroy(NULL, "000000") == false);
    assert(room_destroy(&manager, NULL) == false);

    printf("PASS: invalid_inputs\n");
}

void test_host_leave(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room = room_create(&manager, 101);
    assert(room != NULL);

    char code[ROOM_CODE_LENGTH + 1];
    strcpy(code, room->code);

    assert(room_leave(&manager, room, 101) == true);
    assert(manager.room_count == 0);
    assert(room_find(&manager, code) == NULL);

    printf("PASS: host_leave\n");
}

void test_guest_leave(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room = room_create(&manager, 101);
    assert(room != NULL);

    assert(room_join(&manager, room, 102) == true);

    assert(room_leave(&manager, room, 102) == true);

    assert(manager.room_count == 1);
    assert(room->player_count == 1);
    assert(room->status == ROOM_WAITING);
    assert(room->players[1].session_id == 0);
    assert(room->players[1].connected == false);

    printf("PASS: guest_leave\n");
}

void test_guest_leave_during_match(void)
{
    RoomManager manager;
    room_manager_init(&manager);

    Room *room = room_create(&manager, 101);
    assert(room != NULL);

    assert(room_join(&manager, room, 102) == true);

    // Simulate an active match.
    room->status = ROOM_PLAYING;

    assert(room_leave(&manager, room, 102) == true);
    assert(room->status == ROOM_FINISHED);
    assert(room->players[1].connected == false);

    printf("PASS: guest_leave_during_match\n");
}

int main(void)
{
    test_room_manager_init();
    test_room_create();
    test_room_find();
    test_room_join();
    test_room_full();
    test_duplicate_player();
    test_room_destroy();
    test_room_reuse();
    test_max_rooms();
    test_invalid_inputs();
    test_host_leave();
    test_guest_leave();
    test_guest_leave_during_match();

    printf("\nAll room manager tests passed!\n");

    return 0;
}
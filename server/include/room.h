#ifndef ROOM_H
#define ROOM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_ROOMS 50
#define ROOM_CODE_LENGTH 6
#define PLAYERS_PER_ROOM 2

typedef enum {
    ROOM_WAITING,
    ROOM_SELECTING,
    ROOM_PLAYING,
    ROOM_FINISHED
} RoomStatus;

typedef struct {
    uint64_t session_id;
    bool connected;
} PlayerSession;

typedef struct {
    char code[ROOM_CODE_LENGTH + 1];
    RoomStatus status;
    PlayerSession players[PLAYERS_PER_ROOM];
    size_t player_count;
} Room;

typedef struct {
    Room rooms[MAX_ROOMS];
    bool occupied[MAX_ROOMS];
    size_t room_count;
} RoomManager;

void room_manager_init(RoomManager *manager);

Room *room_create(RoomManager *manager, uint64_t host_session_id);

Room *room_find(RoomManager *manager, const char *code);

bool room_join(RoomManager *manager, Room *room, uint64_t guest_session_id);

bool room_destroy(RoomManager *manager, const char *code);

bool room_leave(RoomManager *manager, Room *room, uint64_t session_id);

bool room_disconnect(RoomManager *manager, Room *room, uint64_t session_id);

bool room_reconnect(RoomManager *manager, Room *room, uint64_t session_id);

#endif
#include "room.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <bcrypt.h>
#else
    #include <errno.h>
    #include <sys/random.h>
#endif

static bool secure_random_byte(unsigned char *byte)
{
    if (byte == NULL)
    {
        return false;
    }

#ifdef _WIN32
    NTSTATUS status = BCryptGenRandom(
        NULL,
        byte,
        1,
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );

    return BCRYPT_SUCCESS(status);
#else
    ssize_t result;

    do
    {
        result = getrandom(byte, 1, 0);
    }
    while (result == -1 && errno == EINTR);

    return result == 1;
#endif
}


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

static bool generate_room_code(char code[ROOM_CODE_LENGTH + 1])
{
    const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const unsigned int character_count = sizeof(characters) - 1;
    const unsigned int limit = 256u - (256u % character_count);

    for (size_t i = 0; i < ROOM_CODE_LENGTH;)
    {
        unsigned char random_byte;

        if (!secure_random_byte(&random_byte))
        {
            return false;
        }

        // Reject values that would introduce modulo bias.
        if (random_byte >= limit)
        {
            continue;
        }

        code[i] = characters[random_byte % character_count];
        i++;
    }

    code[ROOM_CODE_LENGTH] = '\0';

    return true;
}

Room *room_create(RoomManager *manager, uint64_t host_session_id)
{
    if(manager == NULL || host_session_id == 0)
    {
        return NULL;
    }

    if(manager->room_count >= MAX_ROOMS)
    {
        return NULL;
    }

    // Make sure user is not already in another room
    for (size_t i = 0; i<MAX_ROOMS; i++)
    {
        if (manager->occupied[i])
        {
            Room *existing = &manager->rooms[i];
            for (size_t j=0; j<existing->player_count; j++)
            {
                if(existing->players[j].session_id == host_session_id)
                {
                    return NULL;
                }
            }
        }
    }

    // Find and initilize an empty room
    for (size_t i=0; i<MAX_ROOMS; i++)
    {
        if (!manager->occupied[i])
        {
            Room *room = &manager->rooms[i];
            
            bool unique_code = false;

            for (int attempt = 0; attempt < 100; attempt++)
            {
                if (!generate_room_code(room->code))
                {
                    return NULL;
                }

                if (room_find(manager, room->code) == NULL)
                {
                    unique_code = true;
                    break;
                }
            }

            if (!unique_code)
            {
                return NULL;
            }

            room->status = ROOM_WAITING;
            room->player_count = 1;
            room->players[0].session_id = host_session_id;
            room->players[0].connected = true;

            room->players[1].session_id = 0;
            room->players[1].connected = false;

            manager->occupied[i] = true;
            manager->room_count++;

            return room;
        }
    }

    return NULL;
} 

bool room_join(RoomManager *manager, Room *room, uint64_t guest_session_id)
{   
    
    if (manager == NULL || room == NULL || guest_session_id == 0)
    {
        return false;
    }

    // Check if room exists and is occupied 
    bool room_exists = false;

    for (size_t i=0; i<MAX_ROOMS; i++)
    {
        if (manager->occupied[i] && room == &manager->rooms[i])
        {
            room_exists = true;
            break;
        }
    }

    if (!room_exists)
    {
        return false;
    }

    // Check if guest is already in another room
    for (size_t i = 0; i<MAX_ROOMS; i++)
    {
        if (manager->occupied[i])
        {
            Room *existing = &manager->rooms[i];
            for (size_t j=0; j<existing->player_count; j++)
            {
                if(existing->players[j].session_id == guest_session_id)
                {
                    return false;
                }
            }
        }
    }

    if (room->status != ROOM_WAITING)
    {
        return false;
    }

    if (room->player_count >= PLAYERS_PER_ROOM)
    {
        return false;
    }

    size_t slot = room->player_count;

    room->players[slot].session_id = guest_session_id;
    room->players[slot].connected = true;

    room->player_count++;

    // Both players are present; begin character selection.
    room->status = ROOM_SELECTING;

    return true;
}

bool room_destroy(RoomManager *manager, const char *code)
{
    if (manager == NULL || code == NULL)
    {
        return false;
    }

    for (size_t i = 0; i < MAX_ROOMS; i++)
    {
        if (!manager->occupied[i])
        {
            continue;
        }

        // Check whether this is the room we're looking for.
        if (strcmp(manager->rooms[i].code, code) == 0)
        {
        
            manager->occupied[i] = false;

            // Clear all information stored in the room.
            memset(&manager->rooms[i], 0, sizeof(Room));

            manager->room_count--;

            return true;
        }
    }

    return false;
}

bool room_leave(RoomManager *manager, Room *room, uint64_t session_id)
{
    if (manager == NULL || room == NULL || session_id == 0)
    {
        return false;
    }

    // Verify the room is active
    bool room_exists = false;

    for (size_t i = 0; i < MAX_ROOMS; i++)
    {
        if (manager->occupied[i] && room == &manager->rooms[i])
        {
            room_exists = true;
            break;
        }
    }

    if (!room_exists)
    {
        return false;
    }

    // Host is always player 0.
    // Destroy room if host leaves
    if (room->players[0].session_id == session_id)
    {
        char code[ROOM_CODE_LENGTH + 1];
        memcpy(code, room->code, sizeof(code));

        return room_destroy(manager, code);
    }

    // Guest is player 1.
    if (room->player_count == 2 &&
        room->players[1].session_id == session_id)
    {
        // If the match is already playing, mark it finished.
        if (room->status == ROOM_PLAYING)
        {
            room->status = ROOM_FINISHED;
            room->players[1].connected = false;
            return true;
        }

        // Otherwise remove the guest.
        room->players[1].session_id = 0;
        room->players[1].connected = false;

        room->player_count = 1;
        room->status = ROOM_WAITING;

        return true;
    }

    return false;
}

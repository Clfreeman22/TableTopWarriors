#include "room.h"
#include <stdio.h>
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

    // Find and join an empty room
    for (size_t i=0; i<MAX_ROOMS; i++)
    {
        if (!manager->occupied[i])
        {
            Room *room = &manager->rooms[i];
            
            // snprintf used for dev.. need secure code creation for online play
            snprintf(room->code, sizeof(room->code), "%06zu", i);

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

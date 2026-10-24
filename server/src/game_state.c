#include "game_state.h"

#include <stddef.h>
#include <string.h>

void game_state_init(GameState *game)
{
    // Check for an invalid pointer.
    if (game == NULL)
    {
        return;
    }

    // Clear the entire game state.
    memset(game, 0, sizeof(*game));

    // Initialize all 256 board tiles.
    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            game->board[y][x] = TILE_EMPTY;
        }
    }

    // Initialize turn information.
    game->current_player = -1;
    game->action_points = 0;

    // Initialize match information.
    game->status = GAME_WAITING;
    game->winner = -1;
}

bool board_in_bounds(int x, int y)
{
    if (x < 0 || x >= BOARD_WIDTH)
    {
        return false;
    }
        if (y < 0 || y >= BOARD_HEIGHT)
    {
        return false;
    }

    return true;
}

bool board_set_wall(GameState *game, int x, int y)
{
    if (game == NULL)
    {
        return false;
    }

    if (!board_in_bounds(x, y))
    {
        return false;
    }

    game->board[y][x] = TILE_WALL;

    return true;
}

void board_generate_map(GameState *game)
{
    if (game == NULL)
    {
        return;
    }

    // Wall positions for one half of the map.
    const Position walls[] =
    {
        {3, 3},
        {4, 3},
        {3, 4},

        {6, 5},
        {7, 5},

        {5, 8},
        {6, 8},

        {2, 11},
        {3, 11}
    };

    const size_t wall_count = sizeof(walls) / sizeof(walls[0]);

    for (size_t i = 0; i < wall_count; i++)
    {
        int x = walls[i].x;
        int y = walls[i].y;

        // Calculate the mirrored position.
        int mirror_x = BOARD_WIDTH - 1 - x;
        int mirror_y = BOARD_HEIGHT - 1 - y;

        // Place the original and mirrored walls.
        board_set_wall(game, x, y);
        board_set_wall(game, mirror_x, mirror_y);
    }
    
}

bool board_get_spawn_position(int player_index, int character_index, Position *out_position)
{
    if (out_position == NULL)
    {
        return false;
    }

    if (player_index < 0 || player_index >= MAX_PLAYERS)
    {
        return false;
    }

    if (character_index < 0 || character_index >= TEAM_SIZE)
    {
        return false;
    }

    const Position player_one_spawns[TEAM_SIZE] =
    {
        {1, 3},
        {1, 7},
        {1, 11}
    };

    Position spawn = player_one_spawns[character_index];

    if (player_index == 1)
    {
        spawn.x = BOARD_WIDTH - 1 - spawn.x;
        spawn.y = BOARD_HEIGHT - 1 - spawn.y;
    }

    *out_position = spawn;

    return true;
}

bool game_place_character(GameState *game, int player_index, int character_index, CharacterType type)
{
    if (game == NULL)
    {
        return false;
    }

    // Validate player and character indices.
    if (player_index < 0 || player_index >= MAX_PLAYERS)
    {
        return false;
    }

    if (character_index < 0 || character_index >= TEAM_SIZE)
    {
        return false;
    }

    // Validate character type.
    if (type < CHARACTER_ROCK || type >= CHARACTER_COUNT)
    {
        return false;
    }

    // Prevent placing characters after the match starts.
    if (game->status != GAME_WAITING)
    {
        return false;
    }

    Character *character =
        &game->players[player_index].characters[character_index];

    // Prevent overwriting an occupied character slot.
    if (character->selected)
    {
        return false;
    }

    // Prevent duplicate character classes on the same team.
    for (int i = 0; i < TEAM_SIZE; i++)
    {
        if (game->players[player_index].characters[i].selected &&
            game->players[player_index].characters[i].type == type)
        {
            return false;
        }
    }

    // Find the spawn position.
    Position spawn;

    if (!board_get_spawn_position(player_index, character_index, &spawn))
    {
        return false;
    }

    // Make sure the spawn tile is not blocked.
    if (game->board[spawn.y][spawn.x] != TILE_EMPTY)
    {
        return false;
    }

    // Initialize the character.
    memset(character, 0, sizeof(*character));

    character->type = type;
    character->position = spawn;

    CharacterStats stats;

    if (!character_get_stats(type, &stats))
    {
        return false;
    }

    character->max_health = stats.max_health;
    character->health = stats.max_health;
    character->armor = stats.armor;

    character->movement_range = stats.movement_range;
    character->attack_range = stats.attack_range;
    character->attack_damage = stats.attack_damage;

    character->selected = true;
    character->alive = true;
    character->has_moved = false;
    character->has_attacked = false;

    character->moved_last_turn = false;
    character->attacked_last_turn = false;

    return true;
}

bool character_get_stats(CharacterType type, CharacterStats *out_stats)
{
    if (out_stats == NULL)
    {
        return false;
    }

    if (type < 0 || type >= CHARACTER_COUNT)
    {
        return false;
    }

    // {max_health, armor, movement_range, attack_range, attack_damage}
    const CharacterStats stats[CHARACTER_COUNT] =
    {   
        [CHARACTER_ROCK] =      {150, 5, 3, 2, 20},
        [CHARACTER_CLOUD] =     { 90, 1, 6, 5, 15},
        [CHARACTER_DODGEBALL] = {100, 2, 5, 7, 25},
        [CHARACTER_WYTHEGUY] =  {110, 3, 4, 4, 20},
        [CHARACTER_ZOMBIE] =    {130, 2, 3, 1, 30}
    };

    *out_stats = stats[type];

    return true;
}

bool game_start_match(GameState *game, int starting_player)
{
    if (game == NULL)
    {
        return false;
    }

    // A match can only start once.
    if (game->status != GAME_WAITING)
    {
        return false;
    }

    // Validate the starting player.
    if (starting_player < 0 || starting_player >= MAX_PLAYERS)
    {
        return false;
    }

    // Both players must have three selected, living characters.
    for (int player = 0; player < MAX_PLAYERS; player++)
    {
        for (int slot = 0; slot < TEAM_SIZE; slot++)
        {
            Character *character =
                &game->players[player].characters[slot];

            if (!character->selected || !character->alive ||
                character->health <= 0)
            {
                return false;
            }
        }
    }

    // Reset per-turn actions for every character.
    for (int player = 0; player < MAX_PLAYERS; player++)
    {
        for (int slot = 0; slot < TEAM_SIZE; slot++)
        {
            Character *character =
                &game->players[player].characters[slot];

            character->has_moved = false;
            character->has_attacked = false;
        }
    }

    game->current_player = starting_player;
    game->action_points = ACTION_POINTS_PER_TURN;
    game->winner = -1;
    game->status = GAME_ACTIVE;

    return true;
}

bool game_end_turn(GameState *game)
{
    if (game == NULL)
    {
        return false;
    }

    if (game->status != GAME_ACTIVE)
    {
        return false;
    }

    if (game->current_player < 0 ||
        game->current_player >= MAX_PLAYERS)
    {
        return false;
    }

    // Record the outgoing player's completed turn.
    for (int i = 0; i < TEAM_SIZE; i++)
    {
        Character *character =
            &game->players[game->current_player].characters[i];

        character->moved_last_turn = character->has_moved;
        character->attacked_last_turn = character->has_attacked;
    }

    // Switch to the other player.
    game->current_player = 1 - game->current_player;

    // Restore action points for the incoming player.
    game->action_points = ACTION_POINTS_PER_TURN;

    // Reset the incoming player's current-turn action flags.
    for (int i = 0; i < TEAM_SIZE; i++)
    {
        Character *character =
            &game->players[game->current_player].characters[i];

        character->has_moved = false;
        character->has_attacked = false;
    }

    return true;
}

static bool board_is_occupied(const GameState *game, int x, int y)
{
    for (int player = 0; player < MAX_PLAYERS; player++)
    {
        for (int slot = 0; slot < TEAM_SIZE; slot++)
        {
            const Character *character =
                &game->players[player].characters[slot];

            if (character->selected &&
                character->alive &&
                character->position.x == x &&
                character->position.y == y)
            {
                return true;
            }
        }
    }

    return false;
}

static void board_bfs(const GameState *game, Position start, int max_distance, int distance[BOARD_HEIGHT][BOARD_WIDTH])
{
    // Initialize all distances to -1 (unreachable).
    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            distance[y][x] = -1;
        }
    }

    // Validate the inputs.
    if (game == NULL ||
        max_distance < 0 ||
        !board_in_bounds(start.x, start.y))
    {
        return;
    }

    if (game->board[start.y][start.x] == TILE_WALL)
    {
        return;
    }

    Position queue[BOARD_WIDTH * BOARD_HEIGHT];

    int front = 0;
    int back = 0;

    distance[start.y][start.x] = 0;
    queue[back++] = start;

    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};

    while (front < back)
    {
        Position current = queue[front++];

        int current_distance = distance[current.y][current.x];

        if (current_distance >= max_distance)
        {
            continue;
        }

        for (int direction = 0; direction < 4; direction++)
        {
            int next_x = current.x + dx[direction];
            int next_y = current.y + dy[direction];

            if (!board_in_bounds(next_x, next_y))
            {
                continue;
            }

            if (distance[next_y][next_x] != -1)
            {
                continue;
            }

            if (game->board[next_y][next_x] == TILE_WALL ||
                board_is_occupied(game, next_x, next_y))
            {
                continue;
            }

            distance[next_y][next_x] = current_distance + 1;

            queue[back++] = (Position){next_x, next_y};
        }
    }
}

bool board_get_shortest_path(const GameState *game, Position start, Position destination, int max_distance, BoardPath *out_path)
{
    if (out_path == NULL)
    {
        return false;
    }

    out_path->length = 0;

    if (!board_in_bounds(destination.x, destination.y))
    {
        return false;
    }

    int distance[BOARD_HEIGHT][BOARD_WIDTH];

    board_bfs(game, start, max_distance, distance);

    // Destination cannot be reached.
    if (distance[destination.y][destination.x] == -1)
    {
        return false;
    }

    // Work backward from destination to start.
    Position current = destination;

    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};

    while (true)
    {
        out_path->positions[out_path->length++] = current;

        if (current.x == start.x && current.y == start.y)
        {
            break;
        }

        int current_distance = distance[current.y][current.x];
        bool found_previous = false;

        for (int direction = 0; direction < 4; direction++)
        {
            int previous_x = current.x + dx[direction];
            int previous_y = current.y + dy[direction];

            if (!board_in_bounds(previous_x, previous_y))
            {
                continue;
            }

            if (distance[previous_y][previous_x] == current_distance - 1)
            {
                current = (Position){previous_x, previous_y};
                found_previous = true;
                break;
            }
        }

        if (!found_previous)
        {
            out_path->length = 0;
            return false;
        }
    }

    // Reverse the path so it runs from start to destination.
    for (int i = 0; i < out_path->length / 2; i++)
    {
        int opposite = out_path->length - 1 - i;

        Position temp = out_path->positions[i];
        out_path->positions[i] = out_path->positions[opposite];
        out_path->positions[opposite] = temp;
    }

    return true;
}

bool board_find_path(const GameState *game, Position start, Position destination, int max_distance)
{
    if (!board_in_bounds(destination.x, destination.y))
    {
        return false;
    }

    int distance[BOARD_HEIGHT][BOARD_WIDTH];

    board_bfs(game, start, max_distance, distance);

    return distance[destination.y][destination.x] != -1;
}

bool game_move_character(GameState *game, int player_index, int character_index, Position destination)
{
    if (game == NULL || game->status != GAME_ACTIVE)
    {
        return false;
    }

    if (player_index != game->current_player)
    {
        return false;
    }

    if (character_index < 0 || character_index >= TEAM_SIZE)
    {
        return false;
    }

    Character *character = &game->players[player_index].characters[character_index];

    if (!character->selected ||
        !character->alive ||
        character->has_moved ||
        game->action_points < 1)
    {
        return false;
    }

    // Moving to the current position is not a valid action.
    if (character->position.x == destination.x &&
        character->position.y == destination.y)
    {
        return false;
    }

    if (!board_find_path(game, character->position, destination, character->movement_range))
    {
        return false;
    }

    character->position = destination;
    character->has_moved = true;
    game->action_points--;

    return true;
}

void board_get_reachable_tiles(const GameState *game, Position start, int max_distance, bool reachable[BOARD_HEIGHT][BOARD_WIDTH])
{
    if (reachable == NULL)
    {
        return;
    }

    int distance[BOARD_HEIGHT][BOARD_WIDTH];

    board_bfs(game, start, max_distance, distance);

    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            reachable[y][x] = (distance[y][x] != -1);
        }
    }
}


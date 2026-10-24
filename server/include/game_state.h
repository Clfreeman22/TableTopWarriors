
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>
#include <stdint.h>

// Board dimensions
#define BOARD_WIDTH 16
#define BOARD_HEIGHT 16
#define MAX_PATH_LENGTH (BOARD_WIDTH * BOARD_HEIGHT)

// Game rules
#define TEAM_SIZE 3
#define MAX_PLAYERS 2
#define ACTION_POINTS_PER_TURN 6

// Types of tiles on the board
typedef enum
{
    TILE_EMPTY,
    TILE_WALL
} TileType;

// Available character classes
typedef enum
{
    CHARACTER_ROCK,
    CHARACTER_CLOUD,
    CHARACTER_DODGEBALL,
    CHARACTER_WYTHEGUY,
    CHARACTER_ZOMBIE,
    CHARACTER_COUNT
} CharacterType;

// Position on the board
typedef struct
{
    int x;
    int y;
} Position;

typedef struct
{
    Position positions[MAX_PATH_LENGTH];
    int length;
} BoardPath;

typedef struct
{
    int max_health;
    int armor;
    int movement_range;
    int attack_range;
    int attack_damage;
} CharacterStats;

// Individual character information
typedef struct
{
    CharacterType type;
    Position position;

    int health;
    int max_health;
    int armor;

    int movement_range;
    int attack_range;
    int attack_damage;

    bool alive;
    bool has_moved;
    bool has_attacked;
    bool selected;

    bool moved_last_turn;
    bool attacked_last_turn;
} Character;

// One player's team
typedef struct
{
    uint64_t session_id;
    Character characters[TEAM_SIZE];
} GamePlayer;

// Current state of the match
typedef enum
{
    GAME_WAITING,
    GAME_ACTIVE,
    GAME_FINISHED
} GameStatus;

// Complete state of one match
typedef struct
{
    TileType board[BOARD_HEIGHT][BOARD_WIDTH];

    GamePlayer players[MAX_PLAYERS];

    int current_player;
    int action_points;

    GameStatus status;
    int winner;
} GameState;

// Function declarations
void game_state_init(GameState *game);
bool board_in_bounds(int x, int y);
bool board_set_wall(GameState *game, int x, int y);
void board_generate_map(GameState *game);
bool board_get_spawn_position(int player_index, int character_index, Position *out_position);
bool game_place_character(GameState *game, int player_index, int character_index, CharacterType type);
bool character_get_stats(CharacterType type, CharacterStats *out_stats);
bool game_start_match(GameState *game, int starting_player);
bool game_end_turn(GameState *game);
bool board_find_path(const GameState *game, Position start, Position destination, int max_distance);
void board_get_reachable_tiles(const GameState *game, Position start, int max_distance, bool reachable[BOARD_HEIGHT][BOARD_WIDTH]);
bool board_get_shortest_path(const GameState *game, Position start, Position destination, int max_distance, BoardPath *out_path);

#endif

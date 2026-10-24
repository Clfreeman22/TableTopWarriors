
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "game_state.h"

void test_game_state_init(void)
{
    GameState game;

    game_state_init(&game);

    // Verify all 256 tiles are empty.
    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            assert(game.board[y][x] == TILE_EMPTY);
        }
    }

    // Verify the match has not started.
    assert(game.status == GAME_WAITING);

    // Verify no player has the current turn.
    assert(game.current_player == -1);

    // Verify no action points have been assigned.
    assert(game.action_points == 0);

    // Verify there is no winner.
    assert(game.winner == -1);

    // Verify both player slots are empty.
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        assert(game.players[i].session_id == 0);
    }

    printf("PASS: game_state_init\n");
}

void test_board_in_bounds(void)
{
    // Valid coordinates
    assert(board_in_bounds(0, 0) == true);
    assert(board_in_bounds(15, 15) == true);
    assert(board_in_bounds(8, 5) == true);

    // Invalid coordinates
    assert(board_in_bounds(-1, 5) == false);
    assert(board_in_bounds(5, -1) == false);
    assert(board_in_bounds(16, 5) == false);
    assert(board_in_bounds(5, 16) == false);

    printf("PASS: board_in_bounds\n");
}

void test_board_set_wall(void)
{
    GameState game;
    game_state_init(&game);

    // Place a valid wall.
    assert(board_set_wall(&game, 5, 8) == true);
    assert(game.board[8][5] == TILE_WALL);

    // Invalid positions must fail.
    assert(board_set_wall(&game, -1, 5) == false);
    assert(board_set_wall(&game, 16, 5) == false);

    // Invalid game pointer must fail.
    assert(board_set_wall(NULL, 5, 8) == false);

    // Other tiles should remain empty.
    assert(game.board[8][6] == TILE_EMPTY);

    printf("PASS: board_set_wall\n");
}

void test_game_state_reset(void)
{
    GameState game;
    game_state_init(&game);

    // Modify the game state.
    game.board[5][5] = TILE_WALL;
    game.current_player = 1;
    game.action_points = 3;
    game.status = GAME_ACTIVE;
    game.winner = 0;
    game.players[0].session_id = 101;

    // Reset the game.
    game_state_init(&game);

    // Verify everything was reset.
    assert(game.board[5][5] == TILE_EMPTY);
    assert(game.current_player == -1);
    assert(game.action_points == 0);
    assert(game.status == GAME_WAITING);
    assert(game.winner == -1);
    assert(game.players[0].session_id == 0);

    printf("PASS: game_state_reset\n");
}

void test_board_generate_map(void)
{
    GameState game;
    game_state_init(&game);

    board_generate_map(&game);

    int wall_count = 0;

    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            if (game.board[y][x] == TILE_WALL)
            {
                wall_count++;

                int mirror_x = BOARD_WIDTH - 1 - x;
                int mirror_y = BOARD_HEIGHT - 1 - y;

                // Every wall must have a mirrored wall.
                assert(game.board[mirror_y][mirror_x] == TILE_WALL);
            }
        }
    }

    // Nine wall positions, each with one mirror.
    assert(wall_count == 18);

    // Verify some specific walls.
    assert(game.board[3][3] == TILE_WALL);
    assert(game.board[12][12] == TILE_WALL);

    // Starting areas should remain empty.
    assert(game.board[4][1] == TILE_EMPTY);
    assert(game.board[8][1] == TILE_EMPTY);
    assert(game.board[12][1] == TILE_EMPTY);

    assert(game.board[11][14] == TILE_EMPTY);
    assert(game.board[7][14] == TILE_EMPTY);
    assert(game.board[3][14] == TILE_EMPTY);

    printf("PASS: board_generate_map\n");
}

void test_board_spawn_positions(void)
{
    GameState game;
    game_state_init(&game);
    board_generate_map(&game);

    for (int i = 0; i < TEAM_SIZE; i++)
    {
        Position player_one;
        Position player_two;

        assert(board_get_spawn_position(0, i, &player_one));
        assert(board_get_spawn_position(1, i, &player_two));

        // Both positions must be inside the board.
        assert(board_in_bounds(player_one.x, player_one.y));
        assert(board_in_bounds(player_two.x, player_two.y));

        // Both spawn tiles must be empty.
        assert(game.board[player_one.y][player_one.x] == TILE_EMPTY);
        assert(game.board[player_two.y][player_two.x] == TILE_EMPTY);

        // Verify 180-degree rotational symmetry.
        assert(player_two.x == BOARD_WIDTH - 1 - player_one.x);
        assert(player_two.y == BOARD_HEIGHT - 1 - player_one.y);
    }

    // Reject invalid player and character indices.
    Position position;

    assert(!board_get_spawn_position(-1, 0, &position));
    assert(!board_get_spawn_position(2, 0, &position));
    assert(!board_get_spawn_position(0, -1, &position));
    assert(!board_get_spawn_position(0, TEAM_SIZE, &position));
    assert(!board_get_spawn_position(0, 0, NULL));

    printf("PASS: board_spawn_positions\n");
}

void test_game_place_character(void)
{
    GameState game;
    game_state_init(&game);
    board_generate_map(&game);

    // Place Player 1's Rock Guy.
    assert(game_place_character(&game, 0, 0, CHARACTER_ROCK));

    Character *rock = &game.players[0].characters[0];

    assert(rock->type == CHARACTER_ROCK);
    assert(rock->position.x == 1);
    assert(rock->position.y == 3);
    assert(rock->health == 150);
    assert(rock->alive == true);

    // Place Player 2's Railcaster.
    assert(game_place_character(&game, 1, 0, CHARACTER_CLOUD));

    Character *railcaster = &game.players[1].characters[0];

    assert(railcaster->type == CHARACTER_CLOUD);
    assert(railcaster->position.x == 14);
    assert(railcaster->position.y == 12);

    // Cannot overwrite an occupied slot.
    assert(!game_place_character(&game, 0, 0, CHARACTER_DODGEBALL));

    // Cannot select the same class twice on one team.
    assert(!game_place_character(&game, 0, 1, CHARACTER_ROCK));

    // Both players may select the same class.
    assert(game_place_character(&game, 1, 1, CHARACTER_ROCK));

    // Invalid indices must fail.
    assert(!game_place_character(&game, -1, 0, CHARACTER_WYTHEGUY));
    assert(!game_place_character(&game, 2, 0, CHARACTER_WYTHEGUY));
    assert(!game_place_character(&game, 0, -1, CHARACTER_WYTHEGUY));
    assert(!game_place_character(&game, 0, TEAM_SIZE, CHARACTER_WYTHEGUY));

    // Invalid game pointer must fail.
    assert(!game_place_character(NULL, 0, 0, CHARACTER_WYTHEGUY));

    printf("PASS: game_place_character\n");
}

void test_character_get_stats(void)
{
    CharacterStats stats;

    assert(character_get_stats(CHARACTER_ROCK, &stats));
    assert(stats.max_health == 150);
    assert(stats.armor == 5);
    assert(stats.movement_range == 3);
    assert(stats.attack_range == 2);
    assert(stats.attack_damage == 20);

    assert(character_get_stats(CHARACTER_CLOUD, &stats));
    assert(stats.max_health == 90);
    assert(stats.movement_range == 6);

    assert(!character_get_stats(CHARACTER_COUNT, &stats));
    assert(!character_get_stats((CharacterType)-1, &stats));
    assert(!character_get_stats(CHARACTER_ROCK, NULL));

    printf("PASS: character_get_stats\n");
}

void test_character_selection_persists_after_death(void)
{
    GameState game;
    game_state_init(&game);
    board_generate_map(&game);

    assert(game_place_character(&game, 0, 0, CHARACTER_ROCK));

    Character *rock = &game.players[0].characters[0];

    assert(rock->selected);
    assert(rock->alive);

    // Simulate Rock being defeated.
    rock->health = 0;
    rock->alive = false;

    // Rock still occupies its team slot.
    assert(rock->selected);

    // Cannot replace Rock with another character.
    assert(!game_place_character(&game, 0, 0, CHARACTER_CLOUD));

    // Cannot select Rock again in another slot.
    assert(!game_place_character(&game, 0, 1, CHARACTER_ROCK));

    printf("PASS: character_selection_persists_after_death\n");
}

void setup_full_match(GameState *game)
{
    game_state_init(game);
    board_generate_map(game);

    assert(game_place_character(game, 0, 0, CHARACTER_ROCK));
    assert(game_place_character(game, 0, 1, CHARACTER_CLOUD));
    assert(game_place_character(game, 0, 2, CHARACTER_DODGEBALL));

    assert(game_place_character(game, 1, 0, CHARACTER_WYTHEGUY));
    assert(game_place_character(game, 1, 1, CHARACTER_ZOMBIE));
    assert(game_place_character(game, 1, 2, CHARACTER_ROCK));
}

void test_game_start_match(void)
{
    GameState game;

    // Cannot start an incomplete match.
    game_state_init(&game);
    assert(!game_start_match(&game, 0));
    assert(game.status == GAME_WAITING);

    // Prepare both teams.
    setup_full_match(&game);

    // Reject invalid starting players.
    assert(!game_start_match(&game, -1));
    assert(!game_start_match(&game, MAX_PLAYERS));
    assert(game.status == GAME_WAITING);

    // Start with Player 2.
    assert(game_start_match(&game, 1));

    assert(game.status == GAME_ACTIVE);
    assert(game.current_player == 1);
    assert(game.action_points == ACTION_POINTS_PER_TURN);
    assert(game.winner == -1);

    // Cannot start the same match twice.
    assert(!game_start_match(&game, 0));

    // Cannot place new characters after starting.
    assert(!game_place_character(&game, 0, 0, CHARACTER_ZOMBIE));

    // NULL must be rejected.
    assert(!game_start_match(NULL, 0));

    printf("PASS: game_start_match\n");
}

void test_game_end_turn(void)
{
    GameState game;

    setup_full_match(&game);

    // Cannot end a turn before the match starts.
    assert(!game_end_turn(&game));

    // Start with Player 1.
    assert(game_start_match(&game, 0));

    // Simulate Player 1 taking actions.
    game.action_points = 2;
    game.players[0].characters[0].has_moved = true;
    game.players[0].characters[1].has_attacked = true;

    // Simulate Player 2 having old action flags.
    game.players[1].characters[0].has_moved = true;
    game.players[1].characters[1].has_attacked = true;

    assert(game_end_turn(&game));

    // Player 2's turn begins.
    assert(game.current_player == 1);
    assert(game.action_points == ACTION_POINTS_PER_TURN);

    // Player 2's flags should be reset.
    for (int i = 0; i < TEAM_SIZE; i++)
    {
        assert(!game.players[1].characters[i].has_moved);
        assert(!game.players[1].characters[i].has_attacked);
    }

    // Player 1's flags should remain unchanged.
    assert(game.players[0].characters[0].has_moved);
    assert(game.players[0].characters[1].has_attacked);

    // End Player 2's turn.
    assert(game_end_turn(&game));

    assert(game.current_player == 0);
    assert(game.action_points == ACTION_POINTS_PER_TURN);

    // Player 1's flags should now be reset.
    for (int i = 0; i < TEAM_SIZE; i++)
    {
        assert(!game.players[0].characters[i].has_moved);
        assert(!game.players[0].characters[i].has_attacked);
    }

    // Cannot end turns after the game finishes.
    game.status = GAME_FINISHED;
    assert(!game_end_turn(&game));

    // Reject NULL.
    assert(!game_end_turn(NULL));

    printf("PASS: game_end_turn\n");
}

void test_character_action_history(void)
{
    GameState game;
    setup_full_match(&game);

    assert(game_start_match(&game, 0));

    Character *rock = &game.players[0].characters[0];

    // Rock moves and attacks during Player 1's turn.
    rock->has_moved = true;
    rock->has_attacked = true;

    // Before the turn ends, no history has been recorded.
    assert(!rock->moved_last_turn);
    assert(!rock->attacked_last_turn);

    // Player 1 ends their turn.
    assert(game_end_turn(&game));

    // Rock's completed-turn actions are recorded.
    assert(rock->moved_last_turn);
    assert(rock->attacked_last_turn);

    // Player 2 ends their turn.
    assert(game_end_turn(&game));

    // Player 1 begins a new turn.
    assert(!rock->has_moved);
    assert(!rock->has_attacked);

    // Previous-turn history is still available.
    assert(rock->moved_last_turn);
    assert(rock->attacked_last_turn);

    // Rock takes no actions this turn.
    assert(game_end_turn(&game));

    // The latest completed turn replaces the old history.
    assert(!rock->moved_last_turn);
    assert(!rock->attacked_last_turn);

    printf("PASS: character_action_history\n");
}


void test_board_find_path(void)
{
    GameState game;
    game_state_init(&game);

    Position start = {1, 1};

    // A destination two tiles away.
    assert(board_find_path(&game, start, (Position){3, 1}, 2));

    // Not enough movement range.
    assert(!board_find_path(&game, start, (Position){3, 1}, 1));

    // Block the direct path.
    assert(board_set_wall(&game, 2, 1));

    // BFS can find a longer route around the wall.
    assert(board_find_path(&game, start, (Position){3, 1}, 4));

    // But cannot reach it in two steps anymore.
    assert(!board_find_path(&game, start, (Position){3, 1}, 2));

    // Cannot move onto a wall.
    assert(!board_find_path(&game, start, (Position){2, 1}, 5));

    // Invalid positions.
    assert(!board_find_path(&game, start, (Position){16, 1}, 5));
    assert(!board_find_path(NULL, start, (Position){3, 1}, 5));

    printf("PASS: board_find_path\n");
}


void test_board_get_reachable_tiles(void)
{
    GameState game;
    game_state_init(&game);

    bool reachable[BOARD_HEIGHT][BOARD_WIDTH];

    Position start = {5, 5};

    board_get_reachable_tiles(&game, start, 2, reachable);

    // Starting position.
    assert(reachable[5][5]);

    // One tile away.
    assert(reachable[5][6]);
    assert(reachable[4][5]);

    // Two tiles away.
    assert(reachable[5][7]);
    assert(reachable[3][5]);

    // Three tiles away: outside movement range.
    assert(!reachable[5][8]);

    // Add a wall directly to the right.
    assert(board_set_wall(&game, 6, 5));

    board_get_reachable_tiles(&game, start, 2, reachable);

    // Wall cannot be reached.
    assert(!reachable[5][6]);

    // Tile behind the wall requires a detour.
    assert(!reachable[5][7]);

    // Other directions still work.
    assert(reachable[4][5]);

    printf("PASS: board_get_reachable_tiles\n");
}


void test_board_get_shortest_path(void)
{
    GameState game;
    game_state_init(&game);

    BoardPath path;

    Position start = {1, 1};
    Position destination = {3, 1};

    // Straight-line path.
    assert(board_get_shortest_path(&game, start, destination, 2, &path));

    assert(path.length == 3);
    assert(path.positions[0].x == 1);
    assert(path.positions[0].y == 1);
    assert(path.positions[2].x == 3);
    assert(path.positions[2].y == 1);

    // Block the direct route.
    assert(board_set_wall(&game, 2, 1));

    assert(board_get_shortest_path(&game, start, destination, 4, &path));

    // Detour requires four moves, or five positions.
    assert(path.length == 5);

    // Check every step is orthogonally adjacent.
    for (int i = 1; i < path.length; i++)
    {
        int dx = path.positions[i].x - path.positions[i - 1].x;
        int dy = path.positions[i].y - path.positions[i - 1].y;

        assert(abs(dx) + abs(dy) == 1);
    }

    // Not reachable with insufficient movement range.
    assert(!board_get_shortest_path(&game, start, destination, 2, &path));
    assert(path.length == 0);

    // Invalid output pointer.
    assert(!board_get_shortest_path(&game, start, destination, 4, NULL));

    printf("PASS: board_get_shortest_path\n");
}


int main(void)
{
    test_game_state_init();
    test_game_state_reset();
    test_board_in_bounds();
    test_board_set_wall();
    test_board_generate_map();
    test_board_spawn_positions();
    test_game_place_character();
    test_character_get_stats();
    test_character_selection_persists_after_death();
    test_game_start_match();
    test_game_end_turn();
    test_character_action_history();
    test_board_find_path();
    test_board_get_reachable_tiles();
    test_board_get_shortest_path();

    printf("All game state tests passed!\n");

    return 0;
}

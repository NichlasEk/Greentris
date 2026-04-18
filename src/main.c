#include <snes.h>
#include <stdlib.h>

#include "res/soundbank.h"

#define FIELD_W 10
#define FIELD_H 20
#define MAP_W 32
#define MAP_H 32
#define FIELD_X 5
#define FIELD_Y 6
#define PREVIEW_X 19
#define PREVIEW_Y 8
#define SAVE_MAGIC 0x5454

#define TILE_BG_A 0
#define TILE_BG_B 1
#define TILE_WALL 2
#define TILE_I 3
#define TILE_O 4
#define TILE_T 5
#define TILE_S 6
#define TILE_Z 7
#define TILE_J 8
#define TILE_L 9
#define TILE_FRAME 10
#define TILE_SHINE 11

#define STATE_TITLE 0
#define STATE_PLAY 1
#define STATE_PAUSE 2
#define STATE_GAMEOVER 3

extern char tilfont, palfont;
extern char board_tiles, board_tiles_end;
extern char board_palette, board_palette_end;
extern char jump_brr, jump_brr_end;
extern char eat_brr, eat_brr_end;
extern char tada_brr, tada_brr_end;
extern char crash_brr, crash_brr_end;
extern char SOUNDBANK__;

typedef struct
{
    u16 magic;
    u16 best_score;
} SaveData;

typedef struct
{
    s8 x;
    s8 y;
} PieceBlock;

static const PieceBlock piece_blocks[7][4][4] = {
    {
        {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
        {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
        {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
        {{0, 2}, {1, 2}, {2, 2}, {3, 2}}
    },
    {
        {{1, 1}, {2, 1}, {1, 2}, {2, 2}},
        {{1, 1}, {2, 1}, {1, 2}, {2, 2}},
        {{1, 1}, {2, 1}, {1, 2}, {2, 2}},
        {{1, 1}, {2, 1}, {1, 2}, {2, 2}}
    },
    {
        {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
        {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{1, 1}, {0, 2}, {1, 2}, {2, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {1, 2}}
    },
    {
        {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
        {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
        {{1, 0}, {1, 1}, {2, 1}, {2, 2}}
    },
    {
        {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
        {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
        {{2, 0}, {1, 1}, {2, 1}, {1, 2}}
    },
    {
        {{1, 0}, {1, 1}, {1, 2}, {0, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {2, 0}},
        {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
        {{0, 1}, {0, 2}, {1, 2}, {2, 2}}
    },
    {
        {{0, 0}, {0, 1}, {0, 2}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}},
        {{2, 1}, {0, 2}, {1, 2}, {2, 2}}
    }
};

static const u16 score_table[4] = {40, 100, 300, 1200};

static SaveData save_data;
static u8 field[FIELD_H][FIELD_W];
static u16 board_map[MAP_W * MAP_H];
static u8 piece_type;
static u8 next_piece;
static u8 rotation;
static s16 piece_x;
static s16 piece_y;
static u16 score;
static u16 best_score;
static u16 lines_cleared;
static u8 level;
static u8 game_state;
static u8 board_dirty;
static u8 hud_dirty;
static s8 horizontal_hold;
static u8 horizontal_repeat;
static u16 last_pad;
static u16 fall_counter;
static u8 bag[7];
static u8 bag_index;

enum
{
    SFX_MOVE,
    SFX_ROTATE,
    SFX_LOCK,
    SFX_CLEAR,
    SFX_TETRIS,
    SFX_LEVELUP,
    SFX_START,
    SFX_GAMEOVER,
    SFX_PAUSE,
    SFX_COUNT
};

static brrsamples sound_table[SFX_COUNT];

static u16 tile_attr(u16 tile)
{
    return BG_TIL_PAL(0) | BG_TIL_NUM(tile);
}

static u16 bg_attr(u8 x, u8 y)
{
    return tile_attr((((x + y) & 1) == 0) ? TILE_BG_A : TILE_BG_B);
}

static u16 block_tile(u8 type)
{
    return tile_attr(TILE_I + (type - 1));
}

static void clear_map(void)
{
    u8 x;
    u8 y;

    for (y = 0; y < MAP_H; y++)
    {
        for (x = 0; x < MAP_W; x++)
        {
            board_map[y * MAP_W + x] = bg_attr(x, y);
        }
    }
}

static void setup_sprites(void)
{
    u8 i;

    oamInitGfxSet((u8 *)&board_tiles, (&board_tiles_end - &board_tiles), (u8 *)&board_palette, (&board_palette_end - &board_palette), 0, 0x0000, OBJ_SIZE8_L16);

    for (i = 0; i < 4; i++)
    {
        oamSet(i * 4, 0, 240, 3, 0, 0, 0, 0);
        oamSetEx(i * 4, OBJ_SMALL, OBJ_HIDE);
    }
}

static void setup_audio(void)
{
    spcBoot();
    spcSetBank(&SOUNDBANK__);
    spcAllocateSoundRegion(72);
    spcLoad(MOD_POLLEN8);
    spcSetSoundEntry(11, 8, 6, &jump_brr_end - &jump_brr, &jump_brr, &sound_table[SFX_MOVE]);
    spcSetSoundEntry(10, 8, 7, &eat_brr_end - &eat_brr, &eat_brr, &sound_table[SFX_ROTATE]);
    spcSetSoundEntry(9, 8, 5, &eat_brr_end - &eat_brr, &eat_brr, &sound_table[SFX_LOCK]);
    spcSetSoundEntry(12, 8, 5, &tada_brr_end - &tada_brr, &tada_brr, &sound_table[SFX_CLEAR]);
    spcSetSoundEntry(15, 8, 6, &tada_brr_end - &tada_brr, &tada_brr, &sound_table[SFX_TETRIS]);
    spcSetSoundEntry(15, 9, 7, &tada_brr_end - &tada_brr, &tada_brr, &sound_table[SFX_LEVELUP]);
    spcSetSoundEntry(13, 8, 4, &tada_brr_end - &tada_brr, &tada_brr, &sound_table[SFX_START]);
    spcSetSoundEntry(15, 8, 4, &crash_brr_end - &crash_brr, &crash_brr, &sound_table[SFX_GAMEOVER]);
    spcSetSoundEntry(9, 8, 4, &jump_brr_end - &jump_brr, &jump_brr, &sound_table[SFX_PAUSE]);
    spcSetSoundTableEntry(&sound_table[0]);
    spcPlay(0);
    spcSetModuleVolume(96);
}

static void play_sfx(u8 id)
{
    if (id < SFX_COUNT)
    {
        spcPlaySound(id);
    }
}

static void draw_box(u8 x0, u8 y0, u8 w, u8 h)
{
    u8 x;
    u8 y;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            u16 dst = (y0 + y) * MAP_W + x0 + x;

            if (x == 0 || y == 0 || x == w - 1 || y == h - 1)
            {
                board_map[dst] = tile_attr(TILE_WALL);
            }
            else
            {
                board_map[dst] = bg_attr(x0 + x, y0 + y);
            }
        }
    }
}

static u8 piece_cell(u8 type, u8 rot, u8 x, u8 y)
{
    u8 i;

    for (i = 0; i < 4; i++)
    {
        if (piece_blocks[type][rot][i].x == (s8)x && piece_blocks[type][rot][i].y == (s8)y)
        {
            return 1;
        }
    }

    return 0;
}

static void shuffle_bag(void)
{
    u8 i;

    for (i = 0; i < 7; i++)
    {
        bag[i] = i;
    }

    for (i = 0; i < 7; i++)
    {
        u8 j = rand() % 7;
        u8 tmp = bag[i];
        bag[i] = bag[j];
        bag[j] = tmp;
    }

    bag_index = 0;
}

static u8 take_piece(void)
{
    if (bag_index >= 7)
    {
        shuffle_bag();
    }

    return bag[bag_index++];
}

static void save_highscore(void)
{
    save_data.magic = SAVE_MAGIC;
    save_data.best_score = best_score;
    consoleCopySram((u8 *)&save_data, sizeof(SaveData));
}

static void load_highscore(void)
{
    consoleLoadSram((u8 *)&save_data, sizeof(SaveData));

    if (save_data.magic != SAVE_MAGIC)
    {
        best_score = 0;
        save_highscore();
        return;
    }

    best_score = save_data.best_score;
}

static u8 drop_frames(void)
{
    u8 frames = 10;

    if (level > 14)
    {
        return 1;
    }

    frames -= level;
    if (frames < 1)
    {
        frames = 1;
    }
    return frames;
}

static u8 collision(s16 base_x, s16 base_y, u8 type, u8 rot)
{
    u8 i;

    for (i = 0; i < 4; i++)
    {
        s16 fx = base_x + piece_blocks[type][rot][i].x;
        s16 fy = base_y + piece_blocks[type][rot][i].y;

        if (fx < 0 || fx >= FIELD_W || fy >= FIELD_H)
        {
            return 1;
        }

        if (fy >= 0 && field[fy][fx] != 0)
        {
            return 1;
        }
    }

    return 0;
}

static void spawn_piece(void)
{
    piece_type = next_piece;
    next_piece = take_piece();
    rotation = 0;
    piece_x = 3;
    piece_y = 0;
    fall_counter = drop_frames();
    horizontal_hold = 0;
    horizontal_repeat = 0;
    board_dirty = 1;

    if (collision(piece_x, piece_y, piece_type, rotation))
    {
        game_state = STATE_GAMEOVER;
        board_dirty = 1;
        hud_dirty = 1;
        play_sfx(SFX_GAMEOVER);
    }
}

static void start_game(void)
{
    u8 x;
    u8 y;

    for (y = 0; y < FIELD_H; y++)
    {
        for (x = 0; x < FIELD_W; x++)
        {
            field[y][x] = 0;
        }
    }

    score = 0;
    lines_cleared = 0;
    level = 0;
    game_state = STATE_PLAY;
    board_dirty = 1;
    hud_dirty = 1;
    shuffle_bag();
    next_piece = take_piece();
    spawn_piece();
    play_sfx(SFX_START);
}

static void lock_piece(void)
{
    u8 i;

    for (i = 0; i < 4; i++)
    {
        s16 fx = piece_x + piece_blocks[piece_type][rotation][i].x;
        s16 fy = piece_y + piece_blocks[piece_type][rotation][i].y;

        if (fy >= 0 && fx >= 0 && fx < FIELD_W && fy < FIELD_H)
        {
            field[fy][fx] = piece_type + 1;
        }
    }
}

static void clear_lines(void)
{
    s16 y;
    u8 x;
    u8 cleared = 0;
    u8 old_level = level;

    for (y = FIELD_H - 1; y >= 0; y--)
    {
        u8 full = 1;

        for (x = 0; x < FIELD_W; x++)
        {
            if (field[y][x] == 0)
            {
                full = 0;
                break;
            }
        }

        if (!full)
        {
            continue;
        }

        cleared++;

        for (; y > 0; y--)
        {
            for (x = 0; x < FIELD_W; x++)
            {
                field[y][x] = field[y - 1][x];
            }
        }

        for (x = 0; x < FIELD_W; x++)
        {
            field[0][x] = 0;
        }

        y++;
    }

    if (cleared > 0)
    {
        score += score_table[cleared - 1] * (level + 1);
        lines_cleared += cleared;
        level = lines_cleared / 10;
        if (score > best_score)
        {
            best_score = score;
            save_highscore();
        }
        board_dirty = 1;
        hud_dirty = 1;

        if (cleared >= 4)
        {
            play_sfx(SFX_TETRIS);
        }
        else if (level != old_level)
        {
            play_sfx(SFX_LEVELUP);
        }
        else
        {
            play_sfx(SFX_CLEAR);
        }
    }
}

static void render_next_piece(void)
{
    u8 x;
    u8 y;
    u8 i;

    for (y = 0; y < 4; y++)
    {
        for (x = 0; x < 4; x++)
        {
            u16 dst = (PREVIEW_Y + y) * MAP_W + PREVIEW_X + x;

            board_map[dst] = bg_attr(PREVIEW_X + x, PREVIEW_Y + y);
        }
    }

    for (i = 0; i < 4; i++)
    {
        u8 px = PREVIEW_X + piece_blocks[next_piece][0][i].x;
        u8 py = PREVIEW_Y + piece_blocks[next_piece][0][i].y;
        board_map[py * MAP_W + px] = block_tile(next_piece + 1);
    }
}

static void render_title_panel(void)
{
    draw_box(3, 5, 26, 17);
    draw_box(6, 8, 20, 11);
}

static void render_gameover_panel(void)
{
    draw_box(6, 8, 20, 11);
    draw_box(8, 10, 16, 7);
}

static void render_field(void)
{
    u8 x;
    u8 y;

    clear_map();

    if (game_state == STATE_TITLE)
    {
        render_title_panel();
        return;
    }

    draw_box(1, 0, 30, 4);
    draw_box(FIELD_X - 1, FIELD_Y - 1, FIELD_W + 2, FIELD_H + 2);
    draw_box(PREVIEW_X - 1, PREVIEW_Y - 1, 6, 6);

    for (y = 0; y < FIELD_H; y++)
    {
        for (x = 0; x < FIELD_W; x++)
        {
            u16 dst = (FIELD_Y + y) * MAP_W + FIELD_X + x;

            board_map[dst] = bg_attr(FIELD_X + x, FIELD_Y + y);
            if (field[y][x] != 0)
            {
                board_map[dst] = block_tile(field[y][x]);
            }
        }
    }

    render_next_piece();

    if (game_state == STATE_GAMEOVER)
    {
        render_gameover_panel();
    }
}

static void render_active_piece_sprites(void)
{
    u8 i;

    if (game_state != STATE_PLAY)
    {
        for (i = 0; i < 4; i++)
        {
            oamSetEx(i * 4, OBJ_SMALL, OBJ_HIDE);
        }
        return;
    }

    for (i = 0; i < 4; i++)
    {
        s16 fx = piece_x + piece_blocks[piece_type][rotation][i].x;
        s16 fy = piece_y + piece_blocks[piece_type][rotation][i].y;

        if (fx >= 0 && fx < FIELD_W && fy >= 0 && fy < FIELD_H)
        {
            u16 sx = (FIELD_X + fx) << 3;
            u16 sy = (FIELD_Y + fy) << 3;
            u16 tile = TILE_I + piece_type;

            oamSet(i * 4, sx, sy, 3, 0, 0, tile, 0);
            oamSetEx(i * 4, OBJ_SMALL, OBJ_SHOW);
        }
        else
        {
            oamSetEx(i * 4, OBJ_SMALL, OBJ_HIDE);
        }
    }
}

static void render_hud(void)
{
    consoleDrawText(2, 1, "                            ");
    consoleDrawText(2, 2, "                            ");
    consoleDrawText(2, 3, "                            ");
    consoleDrawText(2, 4, "                            ");
    consoleDrawText(2, 5, "                            ");
    consoleDrawText(2, 6, "                            ");
    consoleDrawText(2, 7, "                            ");
    consoleDrawText(2, 8, "                            ");
    consoleDrawText(2, 9, "                            ");
    consoleDrawText(2, 10, "                            ");
    consoleDrawText(2, 11, "                            ");
    consoleDrawText(2, 12, "                            ");
    consoleDrawText(2, 13, "                            ");
    consoleDrawText(2, 14, "                            ");
    consoleDrawText(2, 15, "                            ");
    consoleDrawText(2, 16, "                            ");
    consoleDrawText(2, 17, "                            ");
    consoleDrawText(2, 18, "                            ");
    consoleDrawText(2, 19, "                            ");
    consoleDrawText(2, 20, "                            ");
    consoleDrawText(2, 27, "BY NICHLAS AND AI            ");
    consoleDrawText(2, 28, "                            ");

    if (game_state == STATE_TITLE)
    {
        consoleDrawText(9, 9, "TETRIS DX");
        consoleDrawText(8, 12, "GREEN SCREEN");
        consoleDrawText(8, 14, "EDITION");
        consoleDrawText(8, 17, "PRESS START");
        consoleDrawText(11, 20, "V 1.0.01");
        hud_dirty = 0;
        return;
    }

    consoleDrawText(3, 1, "TETRIS DX");
    consoleDrawText(17, 1, "HIGH %05u", best_score);
    consoleDrawText(3, 2, "SCORE %05u", score);
    consoleDrawText(17, 2, "LEVEL %02u", level + 1);
    consoleDrawText(3, 3, "NEXT PIECE");

    if (game_state == STATE_PAUSE)
    {
        consoleDrawText(17, 3, "PAUSED      ");
    }
    else if (game_state == STATE_GAMEOVER)
    {
        consoleDrawText(17, 3, "GAME OVER   ");
        consoleDrawText(10, 10, "GAME OVER");
        consoleDrawText(8, 13, "FINAL %05u", score);
        consoleDrawText(8, 15, "BEST  %05u", best_score);
        consoleDrawText(8, 17, "PRESS START");
    }
    else
    {
        consoleDrawText(17, 3, "PLAYING     ");
    }

    hud_dirty = 0;
}

static void setup_video(void)
{
    consoleSetTextMapPtr(0x6000);
    consoleSetTextGfxPtr(0x3000);
    consoleInitText(1, 16 * 2, &tilfont, &palfont);

    bgInitTileSet(0, &board_tiles, &board_palette, 0, (&board_tiles_end - &board_tiles), (&board_palette_end - &board_palette), BG_16COLORS, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_32x32);
    bgSetGfxPtr(1, 0x3000);
    bgSetMapPtr(1, 0x6000, SC_32x32);

    setMode(BG_MODE1, 0);
    bgSetDisable(2);
}

static u8 try_move(s16 dx, s16 dy)
{
    if (!collision(piece_x + dx, piece_y + dy, piece_type, rotation))
    {
        piece_x += dx;
        piece_y += dy;
        return 1;
    }

    return 0;
}

static void handle_horizontal_input(u16 current, u16 down)
{
    s8 dir = 0;

    if ((current & KEY_LEFT) != 0 && (current & KEY_RIGHT) == 0)
    {
        dir = -1;
    }
    else if ((current & KEY_RIGHT) != 0 && (current & KEY_LEFT) == 0)
    {
        dir = 1;
    }

    if (dir == 0)
    {
        horizontal_hold = 0;
        horizontal_repeat = 0;
        return;
    }

    if ((dir == -1 && (down & KEY_LEFT) != 0) || (dir == 1 && (down & KEY_RIGHT) != 0) || horizontal_hold != dir)
    {
        if (try_move(dir, 0))
        {
            play_sfx(SFX_MOVE);
        }
        horizontal_hold = dir;
        horizontal_repeat = 5;
        return;
    }

    if (horizontal_repeat > 0)
    {
        horizontal_repeat--;
    }

    if (horizontal_repeat == 0)
    {
        if (try_move(dir, 0))
        {
            play_sfx(SFX_MOVE);
        }
        horizontal_repeat = 1;
    }
}

static void try_rotate(s8 dir)
{
    u8 target = (rotation + dir) & 3;

    if (!collision(piece_x, piece_y, piece_type, target))
    {
        rotation = target;
        play_sfx(SFX_ROTATE);
        return;
    }

    if (!collision(piece_x - 1, piece_y, piece_type, target))
    {
        piece_x--;
        rotation = target;
        play_sfx(SFX_ROTATE);
        return;
    }

    if (!collision(piece_x + 1, piece_y, piece_type, target))
    {
        piece_x++;
        rotation = target;
        play_sfx(SFX_ROTATE);
    }
}

static void step_fall(void)
{
    if (!try_move(0, 1))
    {
        play_sfx(SFX_LOCK);
        lock_piece();
        clear_lines();
        spawn_piece();
    }

    fall_counter = drop_frames();
}

static void fast_drop_step(void)
{
    u8 steps = 3;

    while (steps--)
    {
        if (try_move(0, 1))
        {
            score++;
            if (score > best_score)
            {
                best_score = score;
                save_highscore();
            }
            hud_dirty = 1;
            continue;
        }

        play_sfx(SFX_LOCK);
        lock_piece();
        clear_lines();
        spawn_piece();
        break;
    }

    fall_counter = drop_frames();
}

static void hard_drop(void)
{
    while (try_move(0, 1))
    {
        score += 2;
        if (score > best_score)
        {
            best_score = score;
            save_highscore();
        }
        hud_dirty = 1;
    }

    play_sfx(SFX_LOCK);
    lock_piece();
    clear_lines();
    spawn_piece();
    fall_counter = drop_frames();
}

int main(void)
{
    setup_audio();
    setup_video();
    setup_sprites();
    load_highscore();
    score = 0;
    lines_cleared = 0;
    level = 0;
    game_state = STATE_TITLE;
    board_dirty = 1;
    hud_dirty = 1;
    horizontal_hold = 0;
    horizontal_repeat = 0;
    shuffle_bag();
    next_piece = take_piece();
    piece_type = next_piece;
    rotation = 0;
    piece_x = 3;
    piece_y = 0;
    render_field();
    render_hud();
    render_active_piece_sprites();
    setScreenOn();
    WaitForVBlank();
    dmaCopyVram((u8 *)board_map, 0x6800, sizeof(board_map));
    last_pad = 0;

    while (1)
    {
        u16 current;
        u16 down;

        current = padsCurrent(0);
        down = current & (u16)~last_pad;
        last_pad = current;

        if (game_state == STATE_TITLE)
        {
            if (down & KEY_A)
            {
                play_sfx(SFX_START);
            }
            if (down & KEY_START)
            {
                start_game();
            }
        }
        else if (game_state == STATE_PAUSE)
        {
            if (down & KEY_START)
            {
                game_state = STATE_PLAY;
                board_dirty = 1;
                hud_dirty = 1;
                play_sfx(SFX_PAUSE);
            }
        }
        else if (game_state == STATE_GAMEOVER)
        {
            if (down & KEY_START)
            {
                start_game();
            }
        }
        else
        {
            if (down & KEY_START)
            {
                game_state = STATE_PAUSE;
                board_dirty = 1;
                hud_dirty = 1;
                play_sfx(SFX_PAUSE);
            }
            else
            {
                handle_horizontal_input(current, down);
                if (down & KEY_A)
                {
                    try_rotate(1);
                }
                if (down & KEY_B)
                {
                    try_rotate(3);
                }
                if (down & KEY_UP)
                {
                    hard_drop();
                }

                if ((current & KEY_DOWN) != 0 && (down & KEY_UP) == 0)
                {
                    fast_drop_step();
                }
                else
                {
                    if (fall_counter > 0)
                    {
                        fall_counter--;
                    }

                    if (fall_counter == 0)
                    {
                        step_fall();
                    }
                }
            }
        }

        if (board_dirty)
        {
            render_field();
        }
        if (hud_dirty)
        {
            render_hud();
        }
        render_active_piece_sprites();
        spcProcess();

        WaitForVBlank();
        if (board_dirty)
        {
            dmaCopyVram((u8 *)board_map, 0x6800, sizeof(board_map));
            board_dirty = 0;
        }
    }

    return 0;
}

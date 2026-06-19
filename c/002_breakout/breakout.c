#include <gb/gb.h>
#include <gb/cgb.h>

#include "ball.h"
#include "brick.h"
#include "racket.h"

#include "audio.h"

// t = 00 (Transparent Blue), W = 01 (White), Y = 10 (Yellow), B = 11 (Black)
uint8_t ball_radius = 3;
// 0 = blank tile
// 1 = left of brick
// 2 = right of brick
uint8_t brick_offset = 1;

// Palettes
const UWORD background_palette[] = {
    RGB8(0, 0, 128),     // Dark blue
};

#define BRICK_ROWS 4
#define BRICK_COLS 10
uint8_t bricks[BRICK_ROWS][BRICK_COLS];

unsigned char brick_palette_id[2] = {1, 1};

unsigned char background_id[2] = {0, 0};
unsigned char background_palette_id[2] = {0, 0};

// Starting parameter definitions
#define START_BALL_X   80
#define START_BALL_Y   80
#define START_RACKET_X 72
#define START_RACKET_Y 144
#define START_BALL_DX  1
#define START_BALL_DY  1
#define START_DELAY    25

// Runtime state variables globally tracked.
// This is dirty but this makes it easier to deduplicate logic in reset_play_state.
uint8_t ball_x, ball_y;
int8_t ball_dx, ball_dy;
uint8_t racket_x, racket_y;
uint8_t wait;

void reset_play_state(void) {
    ball_x = START_BALL_X;
    ball_y = START_BALL_Y;
    ball_dx = START_BALL_DX;
    ball_dy = START_BALL_DY;
    racket_x = START_RACKET_X;
    racket_y = START_RACKET_Y;
    wait = START_DELAY;
}

void init_bricks(void) {
    uint8_t r, c;
    for (r = 0; r < BRICK_ROWS; r++) {
        for (c = 0; c < BRICK_COLS; c++) {
            bricks[r][c] = 1; // 1 means Brick is alive
            // Draw brick tile (ID 4) onto the background map
            // Note: We shift down by 2 rows (r + 2) to leave room at the absolute top
            VBK_REG = VBK_BANK_0;
            // Note: this is a 2x1 block of tiles.
            set_bkg_based_tiles(2*c, r + 2, 2, 1, brick_map, brick_offset);
            VBK_REG = VBK_BANK_1;
            set_bkg_tiles(2*c, r + 2, 2, 1, brick_palette_id);
            VBK_REG = VBK_BANK_0;
        }
    }
}

void init_sound(void) {
    NR52_REG = 0x80; // 1000 0000 -> Turn master sound controller ON
    NR50_REG = 0x77; // Set maximum volume for left and right master speakers
    NR51_REG = 0xFF; // Route all 4 channels to both left and right speakers
}

void ball_movement(void) {
  ball_x += ball_dx;
  ball_y += ball_dy;
}

uint8_t check_victory_condition(void) {
    uint8_t r, c;
    for (r = 0; r < BRICK_ROWS; r++) {
        for (c = 0; c < BRICK_COLS; c++) {
            if (bricks[r][c] == 1) {
                return 0; // Found a brick that's still standing
            }
        }
    }
    return 1; // All bricks are cleared!
}

void main(void) {
    if (_cpu == CGB_TYPE) { cpu_fast(); }

    init_sound();

    set_bkg_palette(0, 1, background_palette);
    set_bkg_palette(1, 2, brick_palettes);
    set_sprite_palette(0, 1, ball_palettes);
    set_sprite_palette(1, 1, racket_palettes);

    // Load Background Asset Data (Slots 4 and 5)
    set_bkg_data(brick_map[0]+brick_offset, brick_TILE_COUNT, brick_tiles);

    // Load the 3 needed asset blocks into memory
    set_sprite_data(0, 1, ball_tiles);
    set_sprite_data(1, 2, racket_tiles);

    // Link Hardware IDs to tile maps
    set_sprite_tile(0, 0); // Ball
    set_sprite_tile(1, 1); // Racket Left
    set_sprite_tile(2, 2); // Racket Right

    // Bind elements to Palette Slot 0
    set_sprite_prop(0, 0);
    set_sprite_prop(1, 1);
    set_sprite_prop(2, 1);

    // Build the grid map on startup
    init_bricks();

    reset_play_state();

    SHOW_BKG;
    SHOW_SPRITES;

    while(1) {
        update_background_music();

        // Input scanning
        uint8_t input = joypad();
        if (input & J_LEFT) {
            if (racket_x > 8) { racket_x -= 2; }
        }
        if (input & J_RIGHT) {
            if (racket_x < 152) { racket_x += 2; } // 168 max screen - 16 wide racket
        }

        if (wait > 0) wait--;
        if (wait == 0) {
          ball_movement();
        }

        // Sidewall and ceiling impacts
        if (ball_x <= 8 || ball_x >= 160) { ball_dx *= -1; }
        if (ball_y <= 16) { ball_dy *= -1; }

        // Checks if ball falls within the racket
        if (ball_x + ball_radius >= racket_x && ball_x <= (racket_x + 16 - ball_radius)) {
          if (ball_dy > 0 && ball_y == (racket_y - 2*ball_radius) ) {
            // Bouncing on the top.
            ball_dy *= -1; 
            play_racket_sound();
          } else if (ball_dy > 0 && ball_y >= (racket_y - 2*ball_radius) && ball_y <= (racket_y) ) {
            // Bouncing on the side.
            ball_dy *= -1; 
            ball_dx *= -1; 
            play_racket_sound();
          }
        }

        // Bottom screen boundary
        if (ball_y >= 160) {
            reset_play_state();
            //play_lose_sound();
            play_win_sound();
        }

        // Bricks - ball collisions.
        // Hardware rendering shifts: Background starts at X=0, Y=0. Sprites add X+8, Y+16.
        uint8_t ball_center_x = ball_x - 8 + ball_radius;
        uint8_t ball_center_y = ball_y - 16 + ball_radius;

        // Position to brick mapping.
        uint8_t brick_x = ball_center_x / 16;
        uint8_t brick_y = ball_center_y / 8;

        // Check if the ball is within the bounding vertical row boundaries of the grid layout
        if (brick_y >= 2 && brick_y < (2 + BRICK_ROWS)) {
            uint8_t brick_row = brick_y - 2;

            // Check if a live target is hit at these precise layout indexes
            if (bricks[brick_row][brick_x] == 1) {
                bricks[brick_row][brick_x] = 0; // Destroy brick index flag

                VBK_REG = VBK_BANK_0;
                // Replace the physical visual asset slot on screen with the blank blue space tile (ID 5)
                // Note: this is a 2x1 block of tiles.
                set_bkg_tiles(2*brick_x, brick_y, 2, 1, background_id);
                VBK_REG = VBK_BANK_1;
                set_bkg_tiles(2*brick_x, brick_y, 2, 1, background_palette_id);
                VBK_REG = VBK_BANK_0; // Always drop back to Bank 0 for s

                play_brick_sound();

                if (check_victory_condition() == 1) {
                    play_win_sound();
                    delay(1000);
                    init_bricks();      // Rebuild the wall
                    reset_play_state(); // Reset positions
                }

                // Deflection decision:
                //  - reverse vertical direction on horizontal brick boundary
                //  - reverse horizontal direction on vertical brick boundary
                // Deciding which one we hit is based on whether we passed the
                // boundary in the last ball movement. This means we can reverse
                // both if hitting close to the corner.
                uint8_t brick_left   = brick_x * 16;
                uint8_t brick_right  = brick_left + 16;
                uint8_t brick_top    = brick_y * 8;
                uint8_t brick_bottom = brick_top + 8;

                uint8_t hit_vertical = (ball_dx > 0) ?
                  (ball_center_x - ball_dx) < brick_left && ball_center_x >= brick_left :
                  (ball_center_x - ball_dx) >= brick_right && ball_center_x < brick_right;
                uint8_t hit_horizontal = (ball_dy > 0) ?
                  (ball_center_y - ball_dy) < brick_top && ball_center_y >= brick_top :
                  (ball_center_y - ball_dy) >= brick_bottom && ball_center_y < brick_bottom;

                if (hit_vertical) {
                    ball_dx *= -1; // Side wall collision
                }
                if (hit_horizontal) {
                    ball_dy *= -1; // Horizontal surface impact
                }
            }
        }

        // Render buffer updates
        move_sprite(0, ball_x, ball_y);
        
        // Render 2 sprites side-by-side for a 16px wide paddle
        move_sprite(1, racket_x, racket_y);
        move_sprite(2, racket_x + 8, racket_y);

        wait_vbl_done();
    }
}

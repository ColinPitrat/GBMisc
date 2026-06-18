#include <gb/gb.h>
#include <gb/cgb.h>

// t = 00 (Transparent Blue), W = 01 (White), Y = 10 (Yellow), B = 11 (Black)

/*
// Sprite 0: Ball (8 pixels diameter)
unsigned char ball_tile_data[] = {
    0x3C, 0x3C,  // t t B B B B t t
    0x66, 0x5A,  // t B W Y Y W B t
    0xE7, 0x99,  // B W W Y Y W W B
    0xC3, 0xBD,  // B W Y Y Y Y W B
    0xC3, 0xBD,  // B W Y Y Y Y W B
    0xE7, 0x99,  // B W W Y Y W W B
    0x66, 0x5A,  // t B W Y Y W B t
    0x3C, 0x3C   // t t B B B B t t
};
*/

// Sprite 0: Ball (6 pixels diameter)
unsigned char ball_tile_data[] = {
    0x00, 0x00,  // t t t t t t t t
    0x18, 0x18,  // t t t B B t t t
    0x24, 0x3C,  // t t B Y Y B t t
    0x42, 0x7E,  // t B Y Y Y Y B t
    0x42, 0x7E,  // t B Y Y Y Y B t
    0x24, 0x3C,  // t t B Y Y B t t
    0x18, 0x18,  // t t t B B t t t
    0x00, 0x00,  // t t t t t t t t
};

uint8_t ball_radius = 3;

// Sprite 1: Racket left side
unsigned char racket_left_data[] = {
    0x1F, 0x1F,  // t t t B B B B B
    0x3F, 0x3F,  // t t B B B B B B
    0x3F, 0x30,  // t t B B G G G G
    0x3F, 0x30,  // t t B B G G G G
    0x3F, 0x3F,  // t t B B B B B B
    0x3F, 0x3F,  // t t B B B B B B
    0x00, 0x00,  // t t t t t t t t
    0x00, 0x00   // t t t t t t t t
};

/*
// Sprite 2: Racket middle
unsigned char racket_middle_data[] = {
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0, 0,
    0, 0
};
*/

// Sprite 3: Racket right side
unsigned char racket_right_data[] = {
    0xF8, 0xF8,  // B B B B B t t t
    0xFC, 0xFC,  // B B B B B B t t
    0xFC, 0x0C,  // G G G G B B t t
    0xFC, 0x0C,  // G G G G B B t t
    0xFC, 0xFC,  // B B B B B B t t
    0xFC, 0xFC,  // B B B B B B t t
    0x00, 0x00,  // t t t t t t t t
    0x00, 0x00   // t t t t t t t t
};

unsigned char brick_tile_data[] = {
    0x01, 0x01,  // Shiny line
    0x01, 0x7F,  // Shiny left line, dark right line
    0x01, 0x7F,
    0x01, 0x7F,
    0x01, 0x7F,
    0x01, 0x7F,
    0x01, 0x7F,
    0xFF, 0xFF   // Dark bottom line
};

unsigned char blank_tile_data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Palettes
const UWORD background_palette[] = {
    RGB8(0, 0, 128),     // Dark blue
};

const UWORD brick_palette[] = {
    RGB8(255, 60, 60),   // Bright red
    RGB8(0, 0, 0),       // Unused
    RGB8(220, 40, 40),   // Brick red
    RGB8(110, 40, 40)    // Dark red
};

const UWORD ball_palette[]     = {
    RGB8(0, 0, 0),       // Transparent
    RGB8(255, 255, 255), // White
    RGB8(255, 255, 0),   // Yellow
    RGB8(0, 0, 0)        // Black
};

const UWORD racket_palette[]     = {
    RGB8(0, 0, 0),       // Transparent
    RGB8(180, 180, 186), // Gray
    RGB8(255, 0, 0),     // Red
    RGB8(0, 0, 0)        // Black
};

#define BRICK_ROWS 4
#define BRICK_COLS 20
uint8_t bricks[BRICK_ROWS][BRICK_COLS];

unsigned char brick_id[1] = {4};
unsigned char brick_palette_id[1] = {1};

unsigned char background_id[1] = {5};
unsigned char background_palette_id[1] = {0};

void init_bricks(void) {
    uint8_t r, c;
    for (r = 0; r < BRICK_ROWS; r++) {
        for (c = 0; c < BRICK_COLS; c++) {
            bricks[r][c] = 1; // 1 means Brick is alive
            // Draw brick tile (ID 4) onto the background map
            // Note: We shift down by 2 rows (r + 2) to leave room at the absolute top
            VBK_REG = VBK_BANK_0;
            set_bkg_tiles(c, r + 2, 1, 1, brick_id);
            VBK_REG = VBK_BANK_1;
            set_bkg_tiles(c, r + 2, 1, 1, brick_palette_id);
            VBK_REG = VBK_BANK_0;
        }
    }
}

void init_sound(void) {
    NR52_REG = 0x80; // 1000 0000 -> Turn master sound controller ON
    NR50_REG = 0x77; // Set maximum volume for left and right master speakers
    NR51_REG = 0xFF; // Route all 4 channels to both left and right speakers
}

void play_racket_sound(void) {
    // Channel 1: square wave tone sweep
    NR10_REG = 0x16; // Sweep register: sweep time, increase/decrease, shift
    NR11_REG = 0x40; // Wave duty cycle (25%) and sound length
    NR12_REG = 0x73; // Volume envelope: initial volume, fade direction, sweep step
    NR13_REG = 0x00; // Frequency low bits
    NR14_REG = 0xC6; // Frequency high bits + initialize trigger playback flag
}

void play_brick_sound(void) {
    // Channel 4: White noise (explosion/crash effect)
    NR41_REG = 0x1F; // Sound length
    NR42_REG = 0xF1; // Envelope: high volume initial spike, fast volume fade drop
    NR43_REG = 0x30; // Clock shift frequency divider ratio for white noise texturing
    NR44_REG = 0x80; // Initialize trigger playback flag
}

void main(void) {
    if (_cpu == CGB_TYPE) { cpu_fast(); }

    init_sound();

    set_bkg_palette(0, 1, background_palette);
    set_bkg_palette(1, 1, brick_palette);
    set_sprite_palette(0, 1, ball_palette);
    set_sprite_palette(1, 1, racket_palette);

    // Load Background Asset Data (Slots 4 and 5)
    set_bkg_data(4, 1, brick_tile_data);
    set_bkg_data(5, 1, blank_tile_data);

    // Load the 3 needed asset blocks into memory
    set_sprite_data(0, 1, ball_tile_data);
    set_sprite_data(1, 1, racket_left_data);
    set_sprite_data(2, 1, racket_right_data);

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

    // Starting parameters
    uint8_t start_ball_x = 80;   uint8_t start_ball_y = 80;
    uint8_t start_racket_x = 72; uint8_t start_racket_y = 144;
    int8_t start_ball_dx = 1;    int8_t start_ball_dy = 1;
    uint8_t start_delay = 25;

    // Initialization (ideally we'd share that with the "lost life" part that just does exactly the same thing).
    uint8_t ball_x = start_ball_x;     uint8_t ball_y = start_ball_y;
    int8_t ball_dx = start_ball_dx;    int8_t ball_dy = start_ball_dy;
    uint8_t racket_x = start_racket_x; uint8_t racket_y = start_racket_y; 
    uint8_t wait = start_delay;

    SHOW_BKG;
    SHOW_SPRITES;

    while(1) {
        // Input scanning
        uint8_t input = joypad();
        if (input & J_LEFT) {
            if (racket_x > 8) { racket_x -= 2; }
        }
        if (input & J_RIGHT) {
            if (racket_x < 152) { racket_x += 2; } // 168 max screen - 16 wide racket
        }

        if (wait > 0) {
          wait--;
        }
        if (wait == 0) {
          // Ball movement
          ball_x += ball_dx;
          ball_y += ball_dy;
        }

        // Sidewall and ceiling impacts
        if (ball_x <= 8 || ball_x >= 160) { ball_dx *= -1; }
        if (ball_y <= 16) { ball_dy *= -1; }

        // Checks if falling ball falls within the racket
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
            // TODO: lose a life
            ball_x = start_ball_x;     ball_y = start_ball_y;
            ball_dx = start_ball_dx;   ball_dy = start_ball_dy;
            racket_x = start_racket_x; racket_y = start_racket_y; 
            wait = start_delay;
        }

        // Bricks - ball collisions.
        // Hardware coordinate conversion: Divide pixel locations by 8 to identify grid block index
        uint8_t tile_x = (ball_x - 8) / 8;
        uint8_t tile_y = (ball_y - 16) / 8;

        // Check if the ball is within the bounding vertical row boundaries of the grid layout
        if (tile_y >= 2 && tile_y < (2 + BRICK_ROWS) && tile_x < BRICK_COLS) {
            uint8_t brick_row = tile_y - 2;

            // Check if a live target is hit at these precise layout indexes
            if (bricks[brick_row][tile_x] == 1) {
                bricks[brick_row][tile_x] = 0; // Destroy brick index flag

                VBK_REG = VBK_BANK_0;
                // Replace the physical visual asset slot on screen with the blank blue space tile (ID 5)
                set_bkg_tiles(tile_x, tile_y, 1, 1, background_id);
                VBK_REG = VBK_BANK_1;
                set_bkg_tiles(tile_x, tile_y, 1, 1, background_palette_id);
                VBK_REG = VBK_BANK_0; // Always drop back to Bank 0 for s

                play_brick_sound();

                // TODO: different mechanic if hitting the brick on the side - support hitting the brick from the top.
                ball_dy *= -1; // Deflect the ball back along the vertical path axis
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

#include <gb/gb.h>
#include <gb/cgb.h>

/*
 * t = transparent = 00
 * W = white       = 01
 * R = red         = 10
 * B = black       = 11
 *
 * ttBBBBtt
 * tBWRRWBt
 * BWWRRWWB
 * BWRRRRWB
 * BWRRRRWB
 * BWWRRWWB
 * tBWRRWBt
 * ttBBBBtt
 *
 * 00 00 11 11 11 11 00 00
 * 00 11 01 10 10 01 11 00
 * 11 01 01 10 10 01 01 11
 * 11 01 10 10 10 10 01 11
 * ... and then symmetric ...
 *
 * Second bits for first line:
 * 00111100
 * First bits for first line:
 * 00111100
 *
 * First line:  00111100 00111100 
 * Second line: 01100110 01011010 
 * Third line:  11100111 10011001 
 * Fourth line: 11000011 10111101 
 *
 */

unsigned char ball_tile_data[] = {
  0b00111100, 0b00111100,
  0b01100110, 0b01011010, 
  0b11100111, 0b10011001, 
  0b11000011, 0b10111101, 
  0b11000011, 0b10111101, 
  0b11100111, 0b10011001, 
  0b01100110, 0b01011010, 
  0b00111100, 0b00111100,
};

/*
unsigned char ball_tile_data[] = {
    0x3C, 0x3C,  // Line 1: Top Black Border [00111100]
    0x42, 0x3E,  // Line 2: Black edge, Red left, White right, Black edge
    0x81, 0x7F,  // Line 3: Black edge, Red left, White right, Black edge
    0x81, 0x7F,  // Line 4: Black edge, Red left, White right, Black edge
    0x81, 0x7F,  // Line 5: Black edge, Red left, White right, Black edge
    0x81, 0x7F,  // Line 6: Black edge, Red left, White right, Black edge
    0x42, 0x3E,  // Line 7: Black edge, Red left, White right, Black edge
    0x3C, 0x3C   // Line 8: Bottom Black Border [00111100]
};
*/

// Define Color Palettes using native RGB values (0 to 31 scale)
// Format: RGB888 conversions to GBC 15-bit color hardware layout
const UWORD background_palette[] = {
    RGB8(0, 0, 128),   // Index 0: Dark Blue background color
    RGB8(0, 0, 0),     // Unused
    RGB8(0, 0, 0),     // Unused
    RGB8(0, 0, 0)      // Unused
};

const UWORD sprite_palette[] = {
    RGB8(0, 0, 128),   // Index 0: Transparent color (matches blue background)
    RGB8(255, 255, 255), // Index 1: Pure White
    RGB8(255, 0, 0),   // Index 2: Pure Red
    RGB8(0, 0, 0)      // Index 3: Deep Black
};

void main(void) {
    // Initialize Game Boy Color Hardware Mode
    if (_cpu == CGB_TYPE) {
        cpu_fast(); // Unlock GBC double-speed engine
    }

    // Load Palettes into Video RAM
    set_bkg_palette(0, 1, background_palette); // Apply blue background to Palette Slot 0
    set_sprite_palette(0, 1, sprite_palette); // Apply ball colors to Sprite Palette Slot 0

    // Load Tile Data into Sprite Memory Slots
    // Parameters: Start slot, Number of tiles, Data source array pointer
    set_sprite_data(0, 1, ball_tile_data);

    // Bind Hardware Sprite ID 0 to Tile Data Slot 0
    set_sprite_tile(0, 0);

    // Assign Sprite 0 to use GBC Palette Slot 0
    // Without this, the GBC forces the sprite into legacy black-and-white DMG mode.
    set_sprite_prop(0, 0);

    // Physics variables for positioning and velocity
    uint8_t x_pos = 80;
    uint8_t y_pos = 80;
    int8_t x_velocity = 1;
    int8_t y_velocity = 1;

    // Turn on Display Layers (Background and Sprites)
    SHOW_BKG;
    SHOW_SPRITES;

    // Game Loop Execution
    while(1) {
        // Move the physical coordinate values
        x_pos += x_velocity;
        y_pos += y_velocity;

        // X-Axis Screen Boundary Collision checks (accounting for 8px sprite offset)
        if (x_pos <= 8 || x_pos >= 160) {
            x_velocity *= -1; // Invert movement vector
        }
        // Y-Axis Screen Boundary Collision checks (accounting for 16px vertical offset)
        if (y_pos <= 16 || y_pos >= 152) {
            y_velocity *= -1; // Invert movement vector
        }

        // Draw hardware sprite 0 at the updated screen coordinates
        move_sprite(0, x_pos, y_pos);

        // Synchronize with vertical blanking interval to avoid screen tearing artifacts
        wait_vbl_done();
    }
}

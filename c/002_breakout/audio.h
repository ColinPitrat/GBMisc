#define AUDIO_MASTER_ON    0x80
#define AUDIO_MAX_VOLUME   0x77
#define AUDIO_STEREO_ALL   0xFF

#define CH1_SWEEP_FAST_DOWN 0x6E
#define CH1_SWEEP_SLIDE_DN  0x67
#define CH1_SWEEP_BOUNCE    0x16
#define CH1_DUTY_25         0x40
#define CH1_DUTY_50         0x80
#define CH1_DUTY_CRISP      0x90
#define CH1_ENV_PADDLE      0x73
#define CH1_ENV_LOSE        0xF2
#define CH1_ENV_SUSTAIN     0xF0
#define CH1_TRIGGER_FLAG    0x80

#define CH2_DUTY_WHISTLE    0x60
#define CH2_ENV_SOFT_BGM    0x41
#define CH2_TRIGGER_FLAG    0x80

#define CH4_LEN_DEFAULT     0x1F
#define CH4_ENV_EXPLOSION   0xF1
#define CH4_FREQ_CRASH      0x30
#define CH4_TRIGGER_FLAG    0x80

#define NOTE_C3  0x02C4
#define NOTE_D3  0x03D5
#define NOTE_E3  0x04B3
#define NOTE_F3  0x0522
#define NOTE_G3  0x05E3
#define NOTE_A3  0x067C
#define NOTE_B3  0x0702
#define NOTE_C4  0x06C6
#define NOTE_D4  0x06F0
#define NOTE_E4  0x072A
#define NOTE_F4  0x0746
#define NOTE_G4  0x075C
#define NOTE_A4  0x0793
#define NOTE_B4  0x07B2
#define NOTE_C5  0x07C1
#define NOTE_D5  0x07D6
#define NOTE_E5  0x07EA
#define NOTE_G5  0x07F8
#define NOTE_REST 0x0000

#define BEAT_EIGHTH 8
#define BEAT_QUARTER 16
#define BEAT_HALF 32
#define BEAT_FULL 48

typedef struct {
    uint16_t pitch;
    uint16_t duration;
} Note;

uint8_t current_note_index = 0;
uint8_t note_frame_counter = 0;

// A poor attempt at Molly Malone...
const Note melody[] = {
    {NOTE_C4, BEAT_HALF}, {NOTE_C4, BEAT_EIGHTH}, {NOTE_C4, BEAT_QUARTER}, {NOTE_C4, BEAT_QUARTER}, {NOTE_E4, BEAT_HALF}, {NOTE_C4, BEAT_QUARTER},
    {NOTE_D4, BEAT_HALF}, {NOTE_D4, BEAT_EIGHTH}, {NOTE_D4, BEAT_QUARTER}, {NOTE_D4, BEAT_QUARTER}, {NOTE_G4, BEAT_HALF}, {NOTE_D4, BEAT_QUARTER},
    {NOTE_E4, BEAT_QUARTER},  {NOTE_D4, BEAT_QUARTER},   {NOTE_C4, BEAT_QUARTER}, {NOTE_A4, BEAT_HALF}, {NOTE_G4, BEAT_QUARTER}, {NOTE_E4, BEAT_HALF},
    {NOTE_E4, BEAT_QUARTER},  {NOTE_D4, BEAT_QUARTER},   {NOTE_C4, BEAT_QUARTER}, {NOTE_D4, BEAT_HALF}, {NOTE_E4, BEAT_QUARTER}, {NOTE_REST, BEAT_FULL},

    {NOTE_C4, BEAT_HALF}, {NOTE_C4, BEAT_EIGHTH}, {NOTE_C4, BEAT_QUARTER}, {NOTE_C4, BEAT_QUARTER}, {NOTE_E4, BEAT_HALF}, {NOTE_C4, BEAT_QUARTER},
    {NOTE_D4, BEAT_HALF}, {NOTE_D4, BEAT_EIGHTH}, {NOTE_D4, BEAT_QUARTER}, {NOTE_D4, BEAT_QUARTER}, {NOTE_G4, BEAT_HALF}, {NOTE_D4, BEAT_QUARTER},
    {NOTE_E4, BEAT_QUARTER},  {NOTE_A4, BEAT_QUARTER},   {NOTE_G4, BEAT_QUARTER}, {NOTE_E4, BEAT_HALF}, {NOTE_A4, BEAT_QUARTER}, {NOTE_G4, BEAT_HALF},
    {NOTE_E4, BEAT_QUARTER},  {NOTE_C4, BEAT_QUARTER},   {NOTE_D4, BEAT_QUARTER}, {NOTE_C4, BEAT_FULL},  {NOTE_C4, BEAT_QUARTER}, {NOTE_REST, BEAT_FULL}
};

void update_background_music(void) {
    if (note_frame_counter >= melody[current_note_index].duration) {
        note_frame_counter = 0;
        current_note_index = (current_note_index + 1) % 46;

        uint16_t next_pitch = melody[current_note_index].pitch;

        if (next_pitch == NOTE_REST) {
            NR22_REG = 0x00;
            NR24_REG = CH2_TRIGGER_FLAG;
        } else {
            NR21_REG = CH2_DUTY_WHISTLE;
            NR22_REG = CH2_ENV_SOFT_BGM;
            NR23_REG = (uint8_t)next_pitch;
            NR24_REG = (uint8_t)(next_pitch >> 8) | CH2_TRIGGER_FLAG;
        }
    }
    note_frame_counter++;
}

void play_racket_sound(void) {
    // Channel 1: square wave tone sweep
    NR10_REG = CH1_SWEEP_BOUNCE;
    NR11_REG = CH1_DUTY_25;
    NR12_REG = CH1_ENV_PADDLE;
    NR13_REG = 0x00; // Frequency low bits
    NR14_REG = 0xC6; // Frequency high bits + initialize trigger playback flag
}

void play_brick_sound(void) {
    // Channel 4: White noise (explosion/crash effect)
    NR41_REG = CH4_LEN_DEFAULT;
    NR42_REG = CH4_ENV_EXPLOSION;
    NR43_REG = CH4_FREQ_CRASH;
    NR44_REG = CH4_TRIGGER_FLAG;
}

void play_lose_sound(void) {
    // Channel 1 downward sweep explosion crash
    NR10_REG = CH1_SWEEP_FAST_DOWN;
    NR11_REG = CH1_DUTY_50;
    NR12_REG = CH1_ENV_LOSE; // Maximum volume fading quickly
    NR13_REG = 0x00;
    NR14_REG = 0xC6; // Trigger
}

void play_win_sound(void) {
    uint8_t i;

    uint16_t well_done[] = {
        0x05E3, 0x06C6, 0x072A, 0x0793, // Up Arpeggio
        0x072A, 0x0793, 0x07B2, 0x07C1, // Climbing Higher
    };

    for(i = 0; i < 8; i++) {
        // NR10: Disable the automatic pitch sweep to let notes resonate cleanly
        NR10_REG = 0x00;

        // NR11: Max length wave setting
        NR11_REG = 0x80;

        // FIX: NR12_REG = 0xF0 means Initial Volume Max (0xF), Fading Steps = 0.
        // This tells the hardware: SUSTAIN THE NOTE! It will never go quiet on its own.
        NR12_REG = 0xF0;

        NR13_REG = (uint8_t)well_done[i];
        NR14_REG = (uint8_t)(well_done[i] >> 8) | 0x80;

        // Now that sustain is on, notes can hold for a full 250ms dramatically!
        // We'll give the final note a double-length hold to stick the landing.
        if (i == 17) {
            delay(600);
        } else {
            delay(250);
        }

        // Quickly cut the channel envelope to zero right before the next note
        // to avoid awkward legato muddying.
        NR12_REG = 0x00;
        NR14_REG = 0x80;
    }
}


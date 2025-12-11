#ifndef __PREDICTOR_H__
#define __PREDICTOR_H__

#include <stdint.h>

// Outcome constants
#define TAKEN     1
#define NOT_TAKEN 0

// Generic predictor interface -------------------------------
// All predictors must implement these two functions.

struct Predictor {
    int  (*predict)(struct Predictor* self, uint32_t pc);
    void (*update)(struct Predictor* self, uint32_t pc, int taken);
    void (*destroy)(struct Predictor* self);

    // Predictor-specific internal state lives here:
    void* state;
};

// --- Statistics for the simulator to fill in ---------------
struct BPStats {
    long total_branches;
    long mispredictions;
};

// Create different predictors -------------------------------
// Team member 2 will implement these constructors.
// For now we ONLY declare them.

struct Predictor* predictor_nt();                  // Always Not Taken
struct Predictor* predictor_btfnt();               // Backwards Taken, Forwards Not Taken
struct Predictor* predictor_bimodal(int size);     // size in entries (256, 1024, 4096, 16384)
struct Predictor* predictor_gshare(int size);      // size in entries

#endif

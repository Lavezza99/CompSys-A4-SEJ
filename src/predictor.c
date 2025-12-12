#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "predictor.h"

/* Utility */
static int is_power_of_two(int x) {
    return x > 0 && ((x & (x - 1)) == 0);
}
static int log2_int(int x) {
    int r = 0;
    while (x > 1) { x >>= 1; r++; }
    return r;
}

/* ------------------ NT ------------------ */
static int nt_predict(struct Predictor* self, uint32_t instr_pc, uint32_t target_pc) {
    (void)self; (void)instr_pc; (void)target_pc;
    return NOT_TAKEN;
}
static void nt_update(struct Predictor* self, uint32_t instr_pc, uint32_t target_pc, int taken) {
    (void)self; (void)instr_pc; (void)target_pc; (void)taken;
}
static void nt_destroy(struct Predictor* self) {
    free(self);
}
struct Predictor* predictor_nt() {
    struct Predictor* p = malloc(sizeof(struct Predictor));
    if (!p) return NULL;
    p->predict = nt_predict;
    p->update  = nt_update;
    p->destroy = nt_destroy;
    p->state   = NULL;
    return p;
}

/* ------------------ BTFNT (static) ------------------
   Predict TAKEN if branch target < instr_pc (backward), else NOT_TAKEN.
   This is static and needs no state, but we implement predict/update
   using instr_pc and target_pc provided by simulate.
------------------------------------------------------*/
static int btfnt_predict(struct Predictor* self, uint32_t instr_pc, uint32_t target_pc) {
    (void)self;
    if (target_pc < instr_pc) return TAKEN;
    return NOT_TAKEN;
}
static void btfnt_update(struct Predictor* self, uint32_t instr_pc, uint32_t target_pc, int taken) {
    (void)self; (void)instr_pc; (void)target_pc; (void)taken;
}
static void btfnt_destroy(struct Predictor* self) {
    free(self);
}
struct Predictor* predictor_btfnt() {
    struct Predictor* p = malloc(sizeof(struct Predictor));
    if (!p) return NULL;
    p->predict = btfnt_predict;
    p->update  = btfnt_update;
    p->destroy = btfnt_destroy;
    p->state   = NULL;
    return p;
}

/* ------------------ Bimodal ------------------ */
/* 2-bit saturating counters: 0..3 (>=2 means predict TAKEN) */
struct bimodal_state {
    int size;
    int idx_mask;
    uint8_t *table;
};
static int bimodal_predict(struct Predictor* self, uint32_t instr_pc, uint32_t target_pc) {
    struct bimodal_state* s = (struct bimodal_state*) self->state;
    (void)target_pc;
    uint32_t index = (instr_pc >> 2) & s->idx_mask;
    return (s->table[index] >= 2) ? TAKEN : NOT_TAKEN;
}
static void bimodal_update(struct Predictor* self, uint32_t instr_pc, uint32_t target_pc, int taken) {
    struct bimodal_state* s = (struct bimodal_state*) self->state;
    (void)target_pc;
    uint32_t index = (instr_pc >> 2) & s->idx_mask;
    uint8_t ctr = s->table[index];
    if (taken) {
        if (ctr < 3) ctr++;
    } else {
        if (ctr > 0) ctr--;
    }
    s->table[index] = ctr;
}
static void bimodal_destroy(struct Predictor* self) {
    if (!self) return;
    struct bimodal_state* s = (struct bimodal_state*) self->state;
    if (s) {
        if (s->table) free(s->table);
        free(s);
    }
    free(self);
}
struct Predictor* predictor_bimodal(int size) {
    if (!is_power_of_two(size)) return NULL;
    struct Predictor* p = malloc(sizeof(struct Predictor));
    if (!p) return NULL;
    struct bimodal_state* s = malloc(sizeof(struct bimodal_state));
    if (!s) { free(p); return NULL; }
    s->size = size;
    s->idx_mask = size - 1;
    s->table = calloc(size, sizeof(uint8_t));
    if (!s->table) { free(s); free(p); return NULL; }
    for (int i = 0; i < size; ++i) s->table[i] = 2; /* weakly taken */
    p->predict = bimodal_predict;
    p->update  = bimodal_update;
    p->destroy = bimodal_destroy;
    p->state   = s;
    return p;
}

/* ------------------ gShare ------------------ */
/* GHR bits = log2(size). Use index = ((instr_pc>>2) ^ GHR) & mask */
struct gshare_state {
    int size;
    int idx_mask;
    int ghr_bits;
    uint32_t ghr;
    uint8_t *table;
};
static int gshare_predict(struct Predictor* self, uint32_t instr_pc, uint32_t target_pc) {
    struct gshare_state* s = (struct gshare_state*) self->state;
    (void)target_pc;
    uint32_t pc_index = (instr_pc >> 2) & s->idx_mask;
    uint32_t idx = (pc_index ^ (s->ghr & s->idx_mask)) & s->idx_mask;
    return (s->table[idx] >= 2) ? TAKEN : NOT_TAKEN;
}
static void gshare_update(struct Predictor* self, uint32_t instr_pc, uint32_t target_pc, int taken) {
    struct gshare_state* s = (struct gshare_state*) self->state;
    (void)target_pc;
    uint32_t pc_index = (instr_pc >> 2) & s->idx_mask;
    uint32_t idx = (pc_index ^ (s->ghr & s->idx_mask)) & s->idx_mask;
    uint8_t ctr = s->table[idx];
    if (taken) {
        if (ctr < 3) ctr++;
    } else {
        if (ctr > 0) ctr--;
    }
    s->table[idx] = ctr;
    s->ghr = ((s->ghr << 1) | (taken ? 1u : 0u)) & ((1u << s->ghr_bits) - 1u);
}
static void gshare_destroy(struct Predictor* self) {
    if (!self) return;
    struct gshare_state* s = (struct gshare_state*) self->state;
    if (s) {
        if (s->table) free(s->table);
        free(s);
    }
    free(self);
}
struct Predictor* predictor_gshare(int size) {
    if (!is_power_of_two(size)) return NULL;
    struct Predictor* p = malloc(sizeof(struct Predictor));
    if (!p) return NULL;
    struct gshare_state* s = malloc(sizeof(struct gshare_state));
    if (!s) { free(p); return NULL; }
    s->size = size;
    s->idx_mask = size - 1;
    s->ghr_bits = log2_int(size);
    if (s->ghr_bits <= 0) s->ghr_bits = 1;
    s->ghr = 0;
    s->table = malloc(size * sizeof(uint8_t));
    if (!s->table) { free(s); free(p); return NULL; }
    for (int i = 0; i < size; ++i) s->table[i] = 2; /* weakly taken */
    p->predict = gshare_predict;
    p->update  = gshare_update;
    p->destroy = gshare_destroy;
    p->state   = s;
    return p;
}

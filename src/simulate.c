#include "simulate.h"
#include "disassemble.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Small helpers ---------------------------------------------------------

// Sign-extend 'val' which is 'bits' wide (bits <= 32)
static int32_t sext(int32_t val, int bits) {
    int32_t m = 1 << (bits - 1);
    return (val ^ m) - m;
}

// I-type immediate (12-bit)
static int32_t imm_i(uint32_t inst) {
    int32_t imm = (int32_t)(inst >> 20);
    return sext(imm, 12);
}

// S-type immediate (12-bit)
static int32_t imm_s(uint32_t inst) {
    uint32_t imm = ((inst >> 7) & 0x1f) | (((inst >> 25) & 0x7f) << 5);
    return sext((int32_t)imm, 12);
}

// B-type immediate (branch, 13-bit incl. sign, then << 1)
static int32_t imm_b(uint32_t inst) {
    uint32_t imm = 0;
    imm |= ((inst >> 8) & 0x0f) << 1;      // bits 4:1
    imm |= ((inst >> 25) & 0x3f) << 5;     // bits 10:5
    imm |= ((inst >> 7) & 0x01) << 11;     // bit 11
    imm |= ((inst >> 31) & 0x01) << 12;    // bit 12 (sign)
    return sext((int32_t)imm, 13);
}

// U-type immediate (upper 20 bits, already aligned)
static int32_t imm_u(uint32_t inst) {
    return (int32_t)(inst & 0xfffff000);
}

// J-type immediate (jump, 21-bit incl. sign, then << 1)
static int32_t imm_j(uint32_t inst) {
    uint32_t imm = 0;
    imm |= ((inst >> 21) & 0x3ff) << 1;    // bits 10:1
    imm |= ((inst >> 20) & 0x001) << 11;   // bit 11
    imm |= ((inst >> 12) & 0x0ff) << 12;   // bits 19:12
    imm |= ((inst >> 31) & 0x001) << 20;   // bit 20 (sign)
    return sext((int32_t)imm, 21);
}

// Convenience: enforce that x0 is always zero
static void enforce_x0(int32_t regs[32]) {
    regs[0] = 0;
}

// --- System call handling --------------------------------------------------
//
// ecall: opcode 0x73, funct3 = 0, imm = 0
// A7 (x17) = call number
// A0 (x10) = argument / return value
//
// 1 : getchar() -> result in A0
// 2 : putchar(A0)
// 3, 93 : terminate simulation
//
static int handle_ecall(int32_t regs[32]) {
    int32_t call = regs[17]; // a7
    switch (call) {
        case 1: {
            int ch = getchar();
            if (ch == EOF) ch = -1;
            regs[10] = ch; // a0
            break;
        }
        case 2: {
            int ch = regs[10] & 0xff;
            putchar(ch);
            fflush(stdout);
            break;
        }
        case 3:
        case 93:
            // terminate simulation
            return 0; // 0 => stop
        default:
            // unknown syscall, just stop for now
            fprintf(stderr, "Unknown ecall: %d\n", call);
            return 0;
    }
    return 1; // continue
}

// --- Main simulation -------------------------------------------------------
//
// This is a first, "foundation" implementation: it can run RV32I + RV32M
// programs and handle the required system calls. Logging and branch
// prediction will be added on top later.
//
struct Stat simulate(struct memory *mem, int start_addr,
                     FILE *log_file, struct symbols* symbols,
                     struct Predictor* predictor, struct BPStats* stats) {

    (void)log_file;   // not used yet – we’ll hook this up later
    (void)symbols;    // ditto

    int32_t regs[32];
    for (int i = 0; i < 32; i++) regs[i] = 0;
    enforce_x0(regs);

    uint32_t pc = (uint32_t) start_addr;
    long int insn_count = 0;
    int running = 1;

    while (running) {
        uint32_t addr = pc;
        uint32_t inst = (uint32_t) memory_rd_w(mem, (int)pc);

        pc += 4;
        insn_count++;

        uint32_t opcode = inst & 0x7f;
        uint32_t rd     = (inst >> 7) & 0x1f;
        uint32_t funct3 = (inst >> 12) & 0x7;
        uint32_t rs1    = (inst >> 15) & 0x1f;
        uint32_t rs2    = (inst >> 20) & 0x1f;
        uint32_t funct7 = (inst >> 25) & 0x7f;

        int32_t r1 = regs[rs1];
        int32_t r2 = regs[rs2];

        switch (opcode) {

            // ----------------- LUI / AUIPC -----------------
            case 0x37: { // LUI
                int32_t imm = imm_u(inst);
                if (rd != 0) regs[rd] = imm;
                break;
            }
            case 0x17: { // AUIPC
                int32_t imm = imm_u(inst);
                if (rd != 0) regs[rd] = (int32_t)(addr + imm);
                break;
            }

            // ----------------- Jumps ------------------------
            case 0x6f: { // JAL
                int32_t imm = imm_j(inst);
                if (rd != 0) regs[rd] = (int32_t)(addr + 4);
                pc = (uint32_t)((int32_t)addr + imm);
                break;
            }
            case 0x67: { // JALR
                int32_t imm = imm_i(inst);
                int32_t target = (r1 + imm) & ~1; // clear lowest bit
                if (rd != 0) regs[rd] = (int32_t)(addr + 4);
                pc = (uint32_t) target;
                break;
            }

            // ----------------- Branches ---------------------
            case 0x63: { // Branches (B-type)
                int32_t imm = imm_b(inst);
                int actual_taken = 0;

                // --- PREDICTOR: predict before executing ---
                int predicted_taken = 0;
                if (predictor) {
                    predicted_taken = predictor->predict(predictor, pc);
                    stats->total_branches++;
                }

                switch (funct3) {
                    case 0x0: actual_taken = (r1 == r2); break;        // BEQ
                    case 0x1: actual_taken = (r1 != r2); break;        // BNE
                    case 0x4: actual_taken = (r1 <  r2); break;        // BLT
                    case 0x5: actual_taken = (r1 >= r2); break;        // BGE
                    case 0x6: actual_taken = ((uint32_t)r1 <  (uint32_t)r2); break; // BLTU
                    case 0x7: actual_taken = ((uint32_t)r1 >= (uint32_t)r2); break; // BGEU
                    default:
                        fprintf(stderr, "Unknown branch funct3: 0x%x at 0x%08x\n", funct3, addr);
                        running = 0;
                        break;
                }

                // --- PREDICTOR: update and count mispredicts ---
                if (predictor) {
                    if (predicted_taken != actual_taken)
                        stats->mispredictions++;

                    predictor->update(predictor, pc, actual_taken);
                }  

                // --- Execute branch normally ---
                if (actual_taken)
                    pc = addr + imm;

                break;
            }


            // ----------------- Loads ------------------------
            case 0x03: { // LOAD (I-type)
                int32_t imm = imm_i(inst);
                int32_t eff = r1 + imm;
                int32_t val = 0;
                switch (funct3) {
                    case 0x0: { // LB
                        int32_t b = memory_rd_b(mem, eff);
                        val = sext(b & 0xff, 8);
                        break;
                    }
                    case 0x1: { // LH
                        int32_t h = memory_rd_h(mem, eff);
                        val = sext(h & 0xffff, 16);
                        break;
                    }
                    case 0x2: { // LW
                        val = memory_rd_w(mem, eff);
                        break;
                    }
                    case 0x4: { // LBU
                        int32_t b = memory_rd_b(mem, eff);
                        val = b & 0xff;
                        break;
                    }
                    case 0x5: { // LHU
                        int32_t h = memory_rd_h(mem, eff);
                        val = h & 0xffff;
                        break;
                    }
                    default:
                        fprintf(stderr, "Unknown LOAD funct3: 0x%x at 0x%08x\n", funct3, addr);
                        running = 0;
                        break;
                }
                if (!running) break;
                if (rd != 0) regs[rd] = val;
                break;
            }

            // ----------------- Stores -----------------------
            case 0x23: { // STORE (S-type)
                int32_t imm = imm_s(inst);
                int32_t eff = r1 + imm;
                switch (funct3) {
                    case 0x0: // SB
                        memory_wr_b(mem, eff, r2);
                        break;
                    case 0x1: // SH
                        memory_wr_h(mem, eff, r2);
                        break;
                    case 0x2: // SW
                        memory_wr_w(mem, eff, r2);
                        break;
                    default:
                        fprintf(stderr, "Unknown STORE funct3: 0x%x at 0x%08x\n", funct3, addr);
                        running = 0;
                        break;
                }
                break;
            }

            // ----------------- ALU immediate ----------------
            case 0x13: { // OP-IMM (I-type)
                int32_t imm = imm_i(inst);
                int32_t res = 0;
                switch (funct3) {
                    case 0x0: // ADDI
                        res = r1 + imm;
                        break;
                    case 0x2: // SLTI
                        res = (r1 < imm) ? 1 : 0;
                        break;
                    case 0x3: // SLTIU
                        res = ((uint32_t)r1 < (uint32_t)imm) ? 1 : 0;
                        break;
                    case 0x4: // XORI
                        res = r1 ^ imm;
                        break;
                    case 0x6: // ORI
                        res = r1 | imm;
                        break;
                    case 0x7: // ANDI
                        res = r1 & imm;
                        break;
                    case 0x1: { // SLLI
                        uint32_t shamt = (inst >> 20) & 0x1f;
                        res = (int32_t)((uint32_t)r1 << shamt);
                        break;
                    }
                    case 0x5: {
                        uint32_t shamt = (inst >> 20) & 0x1f;
                        if (funct7 == 0x00) {
                            // SRLI
                            res = (int32_t)((uint32_t)r1 >> shamt);
                        } else if (funct7 == 0x20) {
                            // SRAI
                            res = r1 >> shamt;
                        } else {
                            fprintf(stderr, "Unknown OP-IMM shift funct7: 0x%x at 0x%08x\n", funct7, addr);
                            running = 0;
                        }
                        break;
                    }
                    default:
                        fprintf(stderr, "Unknown OP-IMM funct3: 0x%x at 0x%08x\n", funct3, addr);
                        running = 0;
                        break;
                }
                if (!running) break;
                if (rd != 0) regs[rd] = res;
                break;
            }

                    // ----------------- ALU register -----------------
        case 0x33: { // OP (R-type), includes RV32M
            int32_t res = 0;

            if (funct7 == 0x00 || funct7 == 0x20) {
                // Basic integer ops (ADD/SUB, shifts, logic, compares)
                switch (funct3) {
                    case 0x0: // ADD / SUB
                        if (funct7 == 0x00) {
                            // ADD
                            res = r1 + r2;
                        } else {
                            // SUB (funct7 == 0x20)
                            res = r1 - r2;
                        }
                        break;

                    case 0x1: // SLL
                        res = (int32_t)((uint32_t) r1 << (r2 & 0x1f));
                        break;

                    case 0x2: // SLT
                        res = (r1 < r2) ? 1 : 0;
                        break;

                    case 0x3: // SLTU
                        res = ((uint32_t) r1 < (uint32_t) r2) ? 1 : 0;
                        break;

                    case 0x4: // XOR
                        res = r1 ^ r2;
                        break;

                    case 0x5: // SRL / SRA
                        if (funct7 == 0x00) {
                            // SRL
                            res = (int32_t)((uint32_t) r1 >> (r2 & 0x1f));
                        } else {
                            // SRA (funct7 == 0x20)
                            res = r1 >> (r2 & 0x1f);
                        }
                        break;

                    case 0x6: // OR
                        res = r1 | r2;
                        break;

                    case 0x7: // AND
                        res = r1 & r2;
                        break;

                    default:
                        fprintf(stderr, "Unknown OP funct3: 0x%x at 0x%08x\n", funct3, addr);
                        running = 0;
                        break;
                }
            } else if (funct7 == 0x01) {
                // RV32M (Mul/Div/Rem)
                int32_t s1 = r1;
                int32_t s2 = r2;
                uint32_t u1 = (uint32_t) r1;
                uint32_t u2 = (uint32_t) r2;

                switch (funct3) {
                    case 0x0: // MUL
                        res = s1 * s2;
                        break;
                    case 0x4: // DIV
                        if (s2 == 0) res = -1;
                        else if (s1 == INT32_MIN && s2 == -1) res = INT32_MIN;
                        else res = s1 / s2;
                        break;
                    case 0x5: // DIVU
                        if (u2 == 0) res = -1;
                        else res = (int32_t)(u1 / u2);
                        break;
                    case 0x6: // REM
                        if (s2 == 0) res = s1;
                        else if (s1 == INT32_MIN && s2 == -1) res = 0;
                        else res = s1 % s2;
                        break;
                    case 0x7: // REMU
                        if (u2 == 0) res = (int32_t) u1;
                        else res = (int32_t)(u1 % u2);
                        break;
                    default:
                        fprintf(stderr, "Unknown M-extension funct3: 0x%x at 0x%08x\n", funct3, addr);
                        running = 0;
                        break;
                }
            } else {
                fprintf(stderr, "Unknown OP funct7: 0x%x at 0x%08x\n", funct7, addr);
                running = 0;
            }

            if (!running) break;
            if (rd != 0) regs[rd] = res;
            break;
        }


            // ----------------- SYSTEM / ECALL ---------------
            case 0x73: { // SYSTEM
                // We only care about ecall here (funct3=0, imm=0)
                uint32_t funct12 = inst >> 20;
                if (funct3 == 0 && funct12 == 0) {
                    // ecall
                    running = handle_ecall(regs);
                } else {
                    fprintf(stderr, "Unknown SYSTEM instruction at 0x%08x\n", addr);
                    running = 0;
                }
                break;
            }

            // ----------------- Unknown opcode ---------------
            default:
                fprintf(stderr, "Unknown opcode 0x%x at 0x%08x\n", opcode, addr);
                running = 0;
                break;
        }

        enforce_x0(regs);
    }

    struct Stat st;
    st.insns = insn_count;
    return st;
}

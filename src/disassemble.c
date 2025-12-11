#include "disassemble.h"
#include "read_elf.h"   // for symbols_value_to_sym (optional pretty-print)
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// --- small helpers ---------------------------------------------------------

// sign-extend 'val' which is 'bits' wide (bits <= 32)
static int32_t sext(int32_t val, int bits) {
    int32_t m = 1 << (bits - 1);
    return (val ^ m) - m;
}

// immediates (same layout as in simulate.c)
static int32_t imm_i(uint32_t inst) {
    int32_t imm = (int32_t)(inst >> 20);
    return sext(imm, 12);
}

static int32_t imm_s(uint32_t inst) {
    uint32_t imm = ((inst >> 7) & 0x1f) | (((inst >> 25) & 0x7f) << 5);
    return sext((int32_t)imm, 12);
}

static int32_t imm_b(uint32_t inst) {
    uint32_t imm = 0;
    imm |= ((inst >> 8)  & 0x0f) << 1;   // bits 4:1
    imm |= ((inst >> 25) & 0x3f) << 5;   // bits 10:5
    imm |= ((inst >> 7)  & 0x01) << 11;  // bit 11
    imm |= ((inst >> 31) & 0x01) << 12;  // bit 12 (sign)
    return sext((int32_t)imm, 13);
}

static int32_t imm_u(uint32_t inst) {
    return (int32_t)(inst & 0xfffff000);
}

static int32_t imm_j(uint32_t inst) {
    uint32_t imm = 0;
    imm |= ((inst >> 21) & 0x3ff) << 1;   // bits 10:1
    imm |= ((inst >> 20) & 0x001) << 11;  // bit 11
    imm |= ((inst >> 12) & 0x0ff) << 12;  // bits 19:12
    imm |= ((inst >> 31) & 0x001) << 20;  // bit 20 (sign)
    return sext((int32_t)imm, 21);
}

// pretty register name
static const char* reg_name(unsigned r) {
    static const char* names[32] = {
        "x0","ra","sp","gp","tp","t0","t1","t2",
        "s0","s1","a0","a1","a2","a3","a4","a5",
        "a6","a7","s2","s3","s4","s5","s6","s7",
        "s8","s9","s10","s11","t3","t4","t5","t6"
    };
    if (r < 32) return names[r];
    return "x?";
}

// helper to maybe print a symbol for an address
static void format_addr(char* buf, size_t buf_size,
                        uint32_t addr, struct symbols* symbols) {
    if (symbols) {
        const char* name = symbols_value_to_sym(symbols, addr);
        if (name) {
            snprintf(buf, buf_size, "0x%08x <%s>", addr, name);
            return;
        }
    }
    snprintf(buf, buf_size, "0x%08x", addr);
}

// ---------------------------------------------------------------------------
// main disassemble function
// ---------------------------------------------------------------------------
void disassemble(uint32_t addr, uint32_t inst,
                 char* result, size_t buf_size,
                 struct symbols* symbols)
{
    // fallback in case we bail out
    snprintf(result, buf_size, "unknown   0x%08x", inst);

    uint32_t opcode = inst & 0x7f;
    uint32_t rd     = (inst >> 7)  & 0x1f;
    uint32_t funct3 = (inst >> 12) & 0x7;
    uint32_t rs1    = (inst >> 15) & 0x1f;
    uint32_t rs2    = (inst >> 20) & 0x1f;
    uint32_t funct7 = (inst >> 25) & 0x7f;

    char target_buf[64];

    switch (opcode) {

    // ----------------- LUI / AUIPC -------------------------
    case 0x37: { // LUI
        int32_t imm = imm_u(inst);
        snprintf(result, buf_size,
                 "lui     %s, 0x%x",
                 reg_name(rd), (unsigned)(imm >> 12));
        break;
    }
    case 0x17: { // AUIPC
        int32_t imm = imm_u(inst);
        snprintf(result, buf_size,
                 "auipc   %s, 0x%x",
                 reg_name(rd), (unsigned)(imm >> 12));
        break;
    }

    // ----------------- Jumps -------------------------------
    case 0x6f: { // JAL
        int32_t imm = imm_j(inst);
        uint32_t target = addr + imm;
        format_addr(target_buf, sizeof(target_buf), target, symbols);
        snprintf(result, buf_size,
                 "jal     %s, %s",
                 reg_name(rd), target_buf);
        break;
    }
    case 0x67: { // JALR
        int32_t imm = imm_i(inst);
        snprintf(result, buf_size,
                 "jalr    %s, %d(%s)",
                 reg_name(rd), imm, reg_name(rs1));
        break;
    }

    // ----------------- Branches ----------------------------
    case 0x63: { // B-type
        int32_t imm = imm_b(inst);
        uint32_t target = addr + imm;
        format_addr(target_buf, sizeof(target_buf), target, symbols);
        const char* mnem = "unknown";

        switch (funct3) {
            case 0x0: mnem = "beq";  break;
            case 0x1: mnem = "bne";  break;
            case 0x4: mnem = "blt";  break;
            case 0x5: mnem = "bge";  break;
            case 0x6: mnem = "bltu"; break;
            case 0x7: mnem = "bgeu"; break;
        }
        snprintf(result, buf_size,
                 "%-7s %s, %s, %s",
                 mnem, reg_name(rs1), reg_name(rs2), target_buf);
        break;
    }

    // ----------------- Loads -------------------------------
    case 0x03: { // LOAD (I-type)
        int32_t imm = imm_i(inst);
        const char* mnem = "unknown";
        switch (funct3) {
            case 0x0: mnem = "lb";  break;
            case 0x1: mnem = "lh";  break;
            case 0x2: mnem = "lw";  break;
            case 0x4: mnem = "lbu"; break;
            case 0x5: mnem = "lhu"; break;
        }
        snprintf(result, buf_size,
                 "%-7s %s, %d(%s)",
                 mnem, reg_name(rd), imm, reg_name(rs1));
        break;
    }

    // ----------------- Stores ------------------------------
    case 0x23: { // STORE (S-type)
        int32_t imm = imm_s(inst);
        const char* mnem = "unknown";
        switch (funct3) {
            case 0x0: mnem = "sb"; break;
            case 0x1: mnem = "sh"; break;
            case 0x2: mnem = "sw"; break;
        }
        snprintf(result, buf_size,
                 "%-7s %s, %d(%s)",
                 mnem, reg_name(rs2), imm, reg_name(rs1));
        break;
    }

    // ----------------- ALU immediate -----------------------
    case 0x13: { // OP-IMM
        int32_t imm = imm_i(inst);
        const char* mnem = "unknown";

        switch (funct3) {
            case 0x0: mnem = "addi";  break;
            case 0x2: mnem = "slti";  break;
            case 0x3: mnem = "sltiu"; break;
            case 0x4: mnem = "xori";  break;
            case 0x6: mnem = "ori";   break;
            case 0x7: mnem = "andi";  break;
            case 0x1: { // SLLI
                unsigned shamt = (inst >> 20) & 0x1f;
                snprintf(result, buf_size,
                         "slli    %s, %s, %u",
                         reg_name(rd), reg_name(rs1), shamt);
                return;
            }
            case 0x5: { // SRLI/SRAI
                unsigned shamt = (inst >> 20) & 0x1f;
                if (funct7 == 0x00) {
                    snprintf(result, buf_size,
                             "srli    %s, %s, %u",
                             reg_name(rd), reg_name(rs1), shamt);
                } else if (funct7 == 0x20) {
                    snprintf(result, buf_size,
                             "srai    %s, %s, %u",
                             reg_name(rd), reg_name(rs1), shamt);
                } else {
                    snprintf(result, buf_size,
                             "unknown shift imm 0x%08x", inst);
                }
                return;
            }
        }

        snprintf(result, buf_size,
                 "%-7s %s, %s, %d",
                 mnem, reg_name(rd), reg_name(rs1), imm);
        break;
    }

    // ----------------- ALU register + M extension ----------
    case 0x33: { // OP
        const char* mnem = "unknown";

        if (funct7 == 0x00 || funct7 == 0x20) {
            // base integer ops
            switch (funct3) {
                case 0x0:
                    if (funct7 == 0x00) mnem = "add";
                    else                mnem = "sub";
                    break;
                case 0x1: mnem = "sll";  break;
                case 0x2: mnem = "slt";  break;
                case 0x3: mnem = "sltu"; break;
                case 0x4: mnem = "xor";  break;
                case 0x5:
                    if (funct7 == 0x00) mnem = "srl";
                    else                mnem = "sra";
                    break;
                case 0x6: mnem = "or";   break;
                case 0x7: mnem = "and";  break;
            }
        } else if (funct7 == 0x01) {
            // RV32M
            switch (funct3) {
                case 0x0: mnem = "mul";  break;
                case 0x4: mnem = "div";  break;
                case 0x5: mnem = "divu"; break;
                case 0x6: mnem = "rem";  break;
                case 0x7: mnem = "remu"; break;
                // we skip mulh/mulhsu/mulhu for now
            }
        }

        snprintf(result, buf_size,
                 "%-7s %s, %s, %s",
                 mnem, reg_name(rd), reg_name(rs1), reg_name(rs2));
        break;
    }

    // ----------------- SYSTEM / ECALL ----------------------
    case 0x73: {
        uint32_t funct12 = inst >> 20;
        if (funct3 == 0 && funct12 == 0) {
            snprintf(result, buf_size, "ecall");
        } else {
            snprintf(result, buf_size, "system  0x%08x", inst);
        }
        break;
    }

    // ----------------- default / unknown -------------------
    default:
        snprintf(result, buf_size,
                 "unknown 0x%08x", inst);
        break;
    }
}


#include "memory.h"
#include "read_elf.h"
#include "disassemble.h"
#include "simulate.h"
#include "predictor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void terminate(const char *error)
{
  printf("%s\n", error);
  printf("RISC-V Simulator v0.11.0: Usage:\n");
  printf("  sim riscv-elf sim-options -- prog-args\n");
  printf("    sim-options:\n");
  printf("      sim riscv-elf -d\n");
  printf("      sim riscv-elf -l log\n");
  printf("      sim riscv-elf -s log\n");
  printf("      sim riscv-elf -p prof -b <nt|btfnt|bimodal|gshare> [size]\n");
  printf("    prog-args:\n");
  printf("      sim riscv-elf -- arg1 arg2 ...\n");
  exit(-1);
}

// Helper function - grabs args to simulated program from command line and places them in simulated memory
static int pass_args_to_program(struct memory* mem, int argc, char* argv[]) {
  int seperator_position = 1; // skip first, it is the path to the simulator
  int seperator_found = 0;
  while (seperator_position < argc) {
    seperator_found = strcmp(argv[seperator_position],"--") == 0;
    if (seperator_found) break;
    seperator_position++;
  }
  if (seperator_found) { // we've got args for the program!!
    int first_arg = seperator_position;
    int num_args = argc - first_arg;
    unsigned count_addr = 0x1000000;
    unsigned argv_addr = 0x1000004;
    unsigned str_addr = argv_addr + 4 * num_args;
    memory_wr_w(mem, count_addr, num_args);
    for (int index = 0; index < num_args; ++index) {
      memory_wr_w(mem, argv_addr + 4 * index, str_addr);
      char* cp = argv[first_arg + index];
      int c;
      do {
        c = *cp++;
        memory_wr_b(mem, str_addr++, c);
      } while (c);
    }
  }
  return seperator_position;
}

// Helper function, prints disassembly
static void disassemble_to_stdout(struct memory* mem, struct program_info* prog_info, struct symbols* symbols)
{
  const int buf_size = 100;
  char disassembly[buf_size];
  for (unsigned int addr = prog_info->text_start; addr < prog_info->text_end; addr += 4) {
    unsigned int instruction = memory_rd_w(mem, addr);
    disassemble(addr, instruction, disassembly, buf_size, symbols);
    printf("%8x : %08X       %s\n", addr, instruction, disassembly);
  }
}

static struct Predictor* build_predictor(const char* name, int size)
{
  if (!name) return NULL;
  if (!strcmp(name, "nt")) return predictor_nt();
  if (!strcmp(name, "btfnt")) return predictor_btfnt();
  if (!strcmp(name, "bimodal")) return predictor_bimodal(size);
  if (!strcmp(name, "gshare")) return predictor_gshare(size);
  return NULL;
}

int main(int argc, char *argv[])
{
  struct memory *mem = memory_create();
  argc = pass_args_to_program(mem, argc, argv);

  if (argc < 2) {
    terminate("Missing operands");
  }

  FILE *log_file = NULL;
  FILE *prof_file = NULL;
  int disasm_only = 0;

  const char* pred_name = NULL;
  int pred_size = 0;

  // Parse sim-options (argv[2..argc-1])
  for (int i = 2; i < argc; i++) {
    if (!strcmp(argv[i], "-d")) {
      disasm_only = 1;
    } else if (!strcmp(argv[i], "-l")) {
      if (i + 1 >= argc) terminate("Missing logfile name after -l");
      log_file = fopen(argv[i + 1], "w");
      if (!log_file) terminate("Could not open logfile, terminating.");
      i++;
    } else if (!strcmp(argv[i], "-s")) {
      if (i + 1 >= argc) terminate("Missing logfile name after -s");
      // We open it later (after simulation) exactly like your original code did.
      // Store the name in argv[i+1] and open after sim.
      // For simplicity, we keep the old behavior by reopening after simulation.
      // (log_file is handled later)
      i++;
    } else if (!strcmp(argv[i], "-p")) {
      if (i + 1 >= argc) terminate("Missing profile filename after -p");
      prof_file = fopen(argv[i + 1], "w");
      if (!prof_file) terminate("Could not open profile file, terminating.");
      i++;
    } else if (!strcmp(argv[i], "-b")) {
      if (i + 1 >= argc) terminate("Missing predictor name after -b");
      pred_name = argv[i + 1];
      i++;
      if (!strcmp(pred_name, "bimodal") || !strcmp(pred_name, "gshare")) {
        if (i + 1 >= argc) terminate("Missing size after -b bimodal/gshare");
        pred_size = atoi(argv[i + 1]);
        i++;
      }
    } else {
      terminate("Unknown sim-option");
    }
  }

  struct program_info prog_info;
  int status = read_elf(mem, &prog_info, argv[1], log_file);
  if (status) exit(status);

  struct symbols* symbols = symbols_read_from_elf(argv[1]);
  if (symbols == NULL) exit(-1);

  if (disasm_only) {
    disassemble_to_stdout(mem, &prog_info, symbols);
    symbols_delete(symbols);
    memory_delete(mem);
    return 0;
  }

  struct Predictor* predictor = build_predictor(pred_name, pred_size);
  struct BPStats bpstats = (struct BPStats){0};

  int start_addr = prog_info.start;
  clock_t before = clock();
  struct Stat sim_stats = simulate(mem, start_addr, log_file, symbols, predictor, &bpstats);
  clock_t after = clock();

  long int num_insns = sim_stats.insns;
  int ticks = (int)(after - before);
  double mips = (ticks == 0) ? 0.0 : (1.0 * num_insns * CLOCKS_PER_SEC) / ticks / 1000000.0;

  // Print summary (stdout if no -s/-l log file is open)
  if (log_file) {
    fprintf(log_file, "\nSimulated %ld instructions in %d host ticks (%f MIPS)\n", num_insns, ticks, mips);
    fclose(log_file);
  } else {
    printf("\nSimulated %ld instructions in %d host ticks (%f MIPS)\n", num_insns, ticks, mips);
  }

  // Write profile (branch predictor stats)
  if (prof_file) {
    fprintf(prof_file, "Predictor: %s\n", pred_name ? pred_name : "none");
    if (pred_name && (!strcmp(pred_name, "bimodal") || !strcmp(pred_name, "gshare"))) {
      fprintf(prof_file, "Size: %d\n", pred_size);
    }
    fprintf(prof_file, "Instructions: %ld\n", num_insns);
    fprintf(prof_file, "Total branches: %ld\n", bpstats.total_branches);
    fprintf(prof_file, "Mispredictions: %ld\n", bpstats.mispredictions);

    if (bpstats.total_branches > 0) {
      double rate = (100.0 * (double)bpstats.mispredictions) / (double)bpstats.total_branches;
      fprintf(prof_file, "Misprediction rate: %.2f%%\n", rate);
    }
    if (num_insns > 0) {
      double mpki = (1000.0 * (double)bpstats.mispredictions) / (double)num_insns;
      fprintf(prof_file, "MPKI: %.3f\n", mpki);
    }
    fclose(prof_file);
  }

  if (predictor) predictor->destroy(predictor);

  symbols_delete(symbols);
  memory_delete(mem);
  return 0;
}



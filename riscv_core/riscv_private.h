#pragma once
#include <stdbool.h>

#include "riscv_conf.h"
#include "riscv.h"

#include "../tinycg/tinycg.h"

#define RV_NUM_REGS 32

// csrs
enum {
  // floating point
  CSR_FFLAGS     = 0x001,
  CSR_FRM        = 0x002,
  CSR_FCSR       = 0x003,
  // machine trap status
  CSR_MSTATUS    = 0x300,
  CSR_MISA       = 0x301,
  CSR_MEDELEG    = 0x302,
  CSR_MIDELEG    = 0x303,
  CSR_MIE        = 0x304,
  CSR_MTVEC      = 0x305,
  CSR_MCOUNTEREN = 0x306,
  // machine trap handling
  CSR_MSCRATCH   = 0x340,
  CSR_MEPC       = 0x341,
  CSR_MCAUSE     = 0x342,
  CSR_MTVAL      = 0x343,
  CSR_MIP        = 0x344,
  // low words
  CSR_CYCLE      = 0xC00,
  CSR_TIME       = 0xC01,
  CSR_INSTRET    = 0xC02,
  // high words
  CSR_CYCLEH     = 0xC80,
  CSR_TIMEH      = 0xC81,
  CSR_INSTRETH   = 0xC82,

  CSR_MVENDORID  = 0xF11,
  CSR_MARCHID    = 0xF12,
  CSR_MIMPID     = 0xF13,
  CSR_MHARTID    = 0xF14,
};

// instruction decode masks
enum {
  //               ....xxxx....xxxx....xxxx....xxxx
  INST_6_2     = 0b00000000000000000000000001111100,
  //               ....xxxx....xxxx....xxxx....xxxx
  FR_OPCODE    = 0b00000000000000000000000001111111, // r-type
  FR_RD        = 0b00000000000000000000111110000000,
  FR_FUNCT3    = 0b00000000000000000111000000000000,
  FR_RS1       = 0b00000000000011111000000000000000,
  FR_RS2       = 0b00000001111100000000000000000000,
  FR_FUNCT7    = 0b11111110000000000000000000000000,
  //               ....xxxx....xxxx....xxxx....xxxx
  FI_IMM_11_0  = 0b11111111111100000000000000000000, // i-type
  //               ....xxxx....xxxx....xxxx....xxxx
  FS_IMM_4_0   = 0b00000000000000000000111110000000, // s-type
  FS_IMM_11_5  = 0b11111110000000000000000000000000,
  //               ....xxxx....xxxx....xxxx....xxxx
  FB_IMM_11    = 0b00000000000000000000000010000000, // b-type
  FB_IMM_4_1   = 0b00000000000000000000111100000000,
  FB_IMM_10_5  = 0b01111110000000000000000000000000,
  FB_IMM_12    = 0b10000000000000000000000000000000,
  //               ....xxxx....xxxx....xxxx....xxxx
  FU_IMM_31_12 = 0b11111111111111111111000000000000, // u-type
  //               ....xxxx....xxxx....xxxx....xxxx
  FJ_IMM_19_12 = 0b00000000000011111111000000000000, // j-type
  FJ_IMM_11    = 0b00000000000100000000000000000000,
  FJ_IMM_10_1  = 0b01111111111000000000000000000000,
  FJ_IMM_20    = 0b10000000000000000000000000000000,
  //               ....xxxx....xxxx....xxxx....xxxx
  FR4_FMT      = 0b00000110000000000000000000000000, // r4-type
  FR4_RS3      = 0b11111000000000000000000000000000,
  //               ....xxxx....xxxx....xxxx....xxxx
};

enum {
  //             ....xxxx....xxxx....xxxx....xxxx
  FMASK_SIGN = 0b10000000000000000000000000000000,
  FMASK_EXPN = 0b01111111100000000000000000000000,
  FMASK_FRAC = 0b00000000011111111111111111111111,
  //             ........xxxxxxxx........xxxxxxxx
};

// a translated basic block
struct block_t {
  // number of instructions encompased
  uint32_t instructions;
  // address range of the basic block
  uint32_t pc_start;
  uint32_t pc_end;
  // static next block prediction
  struct block_t *predict;
  // code gen structure
  struct cg_state_t cg;
#if RISCV_JIT_PROFILE
  // number of times this block is executed
  uint32_t hit_count;
#endif
  // start of this blocks code
  uint8_t code[];
};

struct block_map_t {
  // max number of entries in the block map
  uint32_t num_entries;
  // block map
  struct block_t **map;
};

struct code_buffer_t {
  // memory range for code buffer
  uint8_t *start;
  uint8_t *end;
  // code buffer write point
  uint8_t *head;
};

struct riscv_jit_t {
  // code buffer
  struct code_buffer_t code;
  // block hash map
  struct block_map_t block_map;
  // handler for non jitted op_op instructions
  void(*handle_op_op)(struct riscv_t *, uint32_t);
  void(*handle_op_fp)(struct riscv_t *, uint32_t);
  void(*handle_op_system)(struct riscv_t *, uint32_t);
};

struct riscv_t {

  bool halt;

  // io interface
  struct riscv_io_t io;
  // integer registers
  riscv_word_t X[RV_NUM_REGS];
  riscv_word_t PC;
  // user provided data
  riscv_user_t userdata;

#if RISCV_VM_SUPPORT_RV32F
  // float registers
  riscv_float_t F[RV_NUM_REGS];
  uint32_t csr_fcsr;
#endif  // RISCV_VM_SUPPORT_RV32F

  // csr registers
  uint64_t csr_cycle;
  uint32_t csr_mstatus;
  uint32_t csr_mtvec;
  uint32_t csr_misa;
  uint32_t csr_mtval;
  uint32_t csr_mcause;
  uint32_t csr_mscratch;
  uint32_t csr_mepc;
  uint32_t csr_mip;
  uint32_t csr_mbadaddr;

  // jit specific data
  struct riscv_jit_t jit;
};

// decode rd field
static inline uint32_t dec_rd(uint32_t inst) {
  return (inst & FR_RD) >> 7;
}

// decode rs1 field
static inline uint32_t dec_rs1(uint32_t inst) {
  return (inst & FR_RS1) >> 15;
}

// decode rs2 field
static inline uint32_t dec_rs2(uint32_t inst) {
  return (inst & FR_RS2) >> 20;
}

// decoded funct3 field
static inline uint32_t dec_funct3(uint32_t inst) {
  return (inst & FR_FUNCT3) >> 12;
}

// decode funct7 field
static inline uint32_t dec_funct7(uint32_t inst) {
  return (inst & FR_FUNCT7) >> 25;
}

// decode utype instruction immediate
static inline uint32_t dec_utype_imm(uint32_t inst) {
  return inst & FU_IMM_31_12;
}

// decode jtype instruction immediate
static inline int32_t dec_jtype_imm(uint32_t inst) {
  uint32_t dst = 0;
  dst |= (inst & FJ_IMM_20);
  dst |= (inst & FJ_IMM_19_12) << 11;
  dst |= (inst & FJ_IMM_11)    << 2;
  dst |= (inst & FJ_IMM_10_1)  >> 9;
  // note: shifted to 2nd least significant bit
  return ((int32_t)dst) >> 11;
}

// decode itype instruction immediate
static inline int32_t dec_itype_imm(uint32_t inst) {
  return ((int32_t)(inst & FI_IMM_11_0)) >> 20;
}

// decode r4type format field
static inline uint32_t dec_r4type_fmt(uint32_t inst) {
  return (inst & FR4_FMT) >> 25;
}

// decode r4type rs3 field
static inline uint32_t dec_r4type_rs3(uint32_t inst) {
  return (inst & FR4_RS3) >> 27;
}

// decode csr instruction immediate (same as itype, zero extend)
static inline uint32_t dec_csr(uint32_t inst) {
  return ((uint32_t)(inst & FI_IMM_11_0)) >> 20;
}

// decode btype instruction immediate
static inline int32_t dec_btype_imm(uint32_t inst) {
  uint32_t dst = 0;
  dst |= (inst & FB_IMM_12);
  dst |= (inst & FB_IMM_11) << 23;
  dst |= (inst & FB_IMM_10_5) >> 1;
  dst |= (inst & FB_IMM_4_1) << 12;
  // note: shifted to 2nd least significant bit
  return ((int32_t)dst) >> 19;
}

// decode stype instruction immediate
static inline int32_t dec_stype_imm(uint32_t inst) {
  uint32_t dst = 0;
  dst |= (inst & FS_IMM_11_5);
  dst |= (inst & FS_IMM_4_0) << 13;
  return ((int32_t)dst) >> 20;
}

// sign extend a 16 bit value
static inline uint32_t sign_extend_h(uint32_t x) {
  return (int32_t)((int16_t)x);
}

// sign extend an 8 bit value
static inline uint32_t sign_extend_b(uint32_t x) {
  return (int32_t)((int8_t)x);
}

// compute the fclass result
static inline uint32_t calc_fclass(uint32_t f) {
  const uint32_t sign = f & FMASK_SIGN;
  const uint32_t expn = f & FMASK_EXPN;
  const uint32_t frac = f & FMASK_FRAC;

  // note: this could be turned into a binary decision tree for speed

  uint32_t out = 0;
  // 0x001    rs1 is -INF
  out |= (f == 0xff800000) ? 0x001 : 0;
  // 0x002    rs1 is negative normal
  out |= (expn && expn < 0x78000000 && sign) ? 0x002 : 0;
  // 0x004    rs1 is negative subnormal
  out |= (!expn && frac && sign) ? 0x004 : 0;
  // 0x008    rs1 is -0
  out |= (f == 0x80000000) ? 0x008 : 0;
  // 0x010    rs1 is +0
  out |= (f == 0x00000000) ? 0x010 : 0;
  // 0x020    rs1 is positive subnormal
  out |= (!expn && frac && !sign) ? 0x020 : 0;
  // 0x040    rs1 is positive normal
  out |= (expn && expn < 0x78000000 && !sign) ? 0x040 : 0;
  // 0x080    rs1 is +INF
  out |= (f == 0x7f800000) ? 0x080 : 0;
  // 0x100    rs1 is a signaling NaN
  out |= (expn == FMASK_EXPN && (frac <= 0x7ff) && frac) ? 0x100 : 0;
  // 0x200    rs1 is a quiet NaN
  out |= (expn == FMASK_EXPN && (frac >= 0x800)) ? 0x200 : 0;

  return out;
}

void rv_except_inst_misaligned(struct riscv_t *rv, uint32_t old_pc);
void rv_except_load_misaligned(struct riscv_t *rv, uint32_t addr);
void rv_except_store_misaligned(struct riscv_t *rv, uint32_t addr);
void rv_except_illegal_inst(struct riscv_t *rv);

uint32_t csr_csrrw(struct riscv_t *rv, uint32_t csr, uint32_t val);
uint32_t csr_csrrs(struct riscv_t *rv, uint32_t csr, uint32_t val);
uint32_t csr_csrrc(struct riscv_t *rv, uint32_t csr, uint32_t val);

bool rv_jit_init(struct riscv_t *rv);
void rv_jit_free(struct riscv_t *rv);

#ifndef __MIPS_H__
#define __MIPS_H__

#include "sim.h"

class Mipc;
class MipcSysCall;
class SysCall;

typedef unsigned Bool;
#define TRUE 1
#define FALSE 0

#if BYTE_ORDER == LITTLE_ENDIAN

#define FP_TWIDDLE 0

#else

#define FP_TWIDDLE 1

#endif

#define MIPC_DEBUG 1
#define STALL_ON_BRANCH 1

// #define ENABLE_BYPASS 1
// #define BYPASS_LVL 1

#include "mem.h"
#include "../../common/syscall.h"
#include "queue.h"


#ifdef ENABLE_BYPASS
typedef enum {
    EX = 101,
    MEM = 102,
    NONE = 103
}BYPASS_SRC;
#endif

typedef struct
{
    unsigned int _pc;
    unsigned int _ins;
    unsigned _rs, _rt;
    signed int _decodedSRC1, _decodedSRC2; // Reg fetch output (source values)
    unsigned _decodedDST;                  // Decoder output (dest reg no)
    unsigned _subregOperand;               // Needed for lwl and lwr
    unsigned _MAR;                         // Memory address register
    unsigned _opResultHi, _opResultLo;     // Result of operation
    unsigned int _btgt;                     // branch target
    void (*_opControl)(Mipc *, unsigned);
    void (*_memOp)(Mipc *);
    Bool _memControl;                      // Memory instruction?
    Bool _writeREG, _writeFREG;            // WB control
    signed int _branchOffset;
    Bool _hiWPort, _loWPort;   // WB control
    unsigned _decodedShiftAmt; // Shift amount

    Bool _isSyscall;   // 1 if system call
    Bool _isIllegalOp; // 1 if illegal opcode
    Bool _isBranchIns;
    Bool _isNoOp;
    Bool _isFloating;

#ifdef ENABLE_BYPASS
    BYPASS_SRC _bypassSrc1;
    BYPASS_SRC _bypassSrc2;
#endif

} regs;

typedef struct
{
    regs IF_ID;
    regs ID_EX;
    regs EX_MEM;
    regs MEM_WB;

} PipelineRegs;

class Mipc : public SimObject
{
public:
    Mipc(Mem *m);
    ~Mipc();

    FAKE_SIM_TEMPLATE;

    MipcSysCall *_sys; // Emulated system call layer

    void dumpregs(void); // Dumps current register state

    void Reboot(char *image = NULL);
    // Restart processor.
    // "image" = file name for new memory
    // image if any.

    void MipcDumpstats();                // Prints simulation statistics
    void Dec(unsigned int ins);          // Decoder function
    void fake_syscall(unsigned int ins); // System call interface

    /* processor state */
    unsigned int _ins; // instruction register
    Bool _insValid;    // Needed for unpipelined design
    Bool _decodeValid; // Needed for unpipelined design
    Bool _execValid;   // Needed for unpipelined design
    Bool _memValid;    // Needed for unpipelined design
    Bool _insDone;     // Needed for unpipelined design

    signed int _decodedSRC1, _decodedSRC2; // Reg fetch output (source values)
    unsigned _decodedDST;                  // Decoder output (dest reg no)
    unsigned _subregOperand;               // Needed for lwl and lwr
    unsigned _MAR;                         // Memory address register
    unsigned _opResultHi, _opResultLo;     // Result of operation
    Bool _memControl;                      // Memory instruction?
    Bool _writeREG, _writeFREG;            // WB control
    signed int _branchOffset;
    Bool _hiWPort, _loWPort;   // WB control
    unsigned _decodedShiftAmt; // Shift amount

    unsigned int _gpr[32]; // general-purpose integer registers

    union
    {
        unsigned int l[2];
        float f[2];
        double d;
    } _fpr[16]; // floating-point registers (paired)

    unsigned int _hi, _lo; // mult, div destination
    unsigned int _pc;      // Program counter
    unsigned int _lastbd;  // branch delay state
    unsigned int _boot;    // boot code loaded?

    int _btaken;        // taken branch (1 if taken, 0 if fall-through)
    int _bd;            // 1 if the next ins is delay slot
    unsigned int _btgt; // branch target

    Bool _isSyscall;   // 1 if system call
    Bool _isIllegalOp; // 1 if illegal opcode


    // Pipeline State
    unsigned _rs, _rt;
    PipelineRegs _pipe_regs_live;
    PipelineRegs _pipe_regs_copy;
    bool _stall;
#ifdef STALL_ON_BRANCH
    bool _isBranchInterlock;
#endif
    unsigned int _stall_cycles;
    bool _executing_syscall;


    void killID_EX();
    void killIF_ID();


    // decode local variable
    bool _readLo;
    bool _readHi;
    bool _isFloating;
    bool _hasImm;
    // Simulation statistics counters

    LL _nfetched;
    LL _num_cond_br;
    LL _num_jal;
    LL _num_jr;
    LL _num_load;
    LL _num_store;
    LL _fpinst;
    LL _num_load_stall;

    Mem *_mem; // attached memory (not a cache)

    Log _l;
    int _sim_exit; // 1 on normal termination

    void (*_opControl)(Mipc *, unsigned);
    void (*_memOp)(Mipc *);

    FILE *_debugLog;

    // EXE stage definitions

    static void func_add_addu(Mipc *, unsigned);
    static void func_and(Mipc *, unsigned);
    static void func_nor(Mipc *, unsigned);
    static void func_or(Mipc *, unsigned);
    static void func_sll(Mipc *, unsigned);
    static void func_sllv(Mipc *, unsigned);
    static void func_slt(Mipc *, unsigned);
    static void func_sltu(Mipc *, unsigned);
    static void func_sra(Mipc *, unsigned);
    static void func_srav(Mipc *, unsigned);
    static void func_srl(Mipc *, unsigned);
    static void func_srlv(Mipc *, unsigned);
    static void func_sub_subu(Mipc *, unsigned);
    static void func_xor(Mipc *, unsigned);
    static void func_div(Mipc *, unsigned);
    static void func_divu(Mipc *, unsigned);
    static void func_mfhi(Mipc *, unsigned);
    static void func_mflo(Mipc *, unsigned);
    static void func_mthi(Mipc *, unsigned);
    static void func_mtlo(Mipc *, unsigned);
    static void func_mult(Mipc *, unsigned);
    static void func_multu(Mipc *, unsigned);
    static void func_jalr(Mipc *, unsigned);
    static void func_jr(Mipc *, unsigned);
    static void func_await_break(Mipc *, unsigned);
    static void func_syscall(Mipc *, unsigned);
    static void func_addi_addiu(Mipc *, unsigned);
    static void func_andi(Mipc *, unsigned);
    static void func_lui(Mipc *, unsigned);
    static void func_ori(Mipc *, unsigned);
    static void func_slti(Mipc *, unsigned);
    static void func_sltiu(Mipc *, unsigned);
    static void func_xori(Mipc *, unsigned);
    static void func_beq(Mipc *, unsigned);
    static void func_bgez(Mipc *, unsigned);
    static void func_bgezal(Mipc *, unsigned);
    static void func_bltzal(Mipc *, unsigned);
    static void func_bltz(Mipc *, unsigned);
    static void func_bgtz(Mipc *, unsigned);
    static void func_blez(Mipc *, unsigned);
    static void func_bne(Mipc *, unsigned);
    static void func_j(Mipc *, unsigned);
    static void func_jal(Mipc *, unsigned);
    static void func_lb(Mipc *, unsigned);
    static void func_lbu(Mipc *, unsigned);
    static void func_lh(Mipc *, unsigned);
    static void func_lhu(Mipc *, unsigned);
    static void func_lwl(Mipc *, unsigned);
    static void func_lw(Mipc *, unsigned);
    static void func_lwr(Mipc *, unsigned);
    static void func_lwc1(Mipc *, unsigned);
    static void func_swc1(Mipc *, unsigned);
    static void func_sb(Mipc *, unsigned);
    static void func_sh(Mipc *, unsigned);
    static void func_swl(Mipc *, unsigned);
    static void func_sw(Mipc *, unsigned);
    static void func_swr(Mipc *, unsigned);
    static void func_mtc1(Mipc *, unsigned);
    static void func_mfc1(Mipc *, unsigned);

    // MEM stage definitions

    static void mem_lb(Mipc *);
    static void mem_lbu(Mipc *);
    static void mem_lh(Mipc *);
    static void mem_lhu(Mipc *);
    static void mem_lwl(Mipc *);
    static void mem_lw(Mipc *);
    static void mem_lwr(Mipc *);
    static void mem_lwc1(Mipc *);
    static void mem_swc1(Mipc *);
    static void mem_sb(Mipc *);
    static void mem_sh(Mipc *);
    static void mem_swl(Mipc *);
    static void mem_sw(Mipc *);
    static void mem_swr(Mipc *);
};

// Emulated system call interface

class MipcSysCall : public SysCall
{
public:
    MipcSysCall(Mipc *ms)
    {

        char buf[1024];
        m = ms->_mem;
        _ms = ms;
        _num_load = 0;
        _num_store = 0;
    };

    ~MipcSysCall(){};

    LL GetDWord(LL addr);
    void SetDWord(LL addr, LL data);

    Word GetWord(LL addr);
    void SetWord(LL addr, Word data);

    void SetReg(int reg, LL val);
    LL GetReg(int reg);
    LL GetTime(void);

private:
    Mipc *_ms;
};
#endif /* __MIPS_H__ */

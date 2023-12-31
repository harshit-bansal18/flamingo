#include "mips.h"
#include <assert.h>
#include "mips-irix5.h"

Mipc::Mipc(Mem *m) : _l('M')
{
    _mem = m;
    _sys = new MipcSysCall(this); // Allocate syscall layer

#ifdef MIPC_DEBUG
    _debugLog = fopen("mipc.debug", "w");
    assert(_debugLog != NULL);
#endif

    Reboot(ParamGetString("Mipc.BootROM"));
}

Mipc::~Mipc(void)
{
}

void Mipc::MainLoop(void)
{
    LL addr;
    unsigned int ins; // Local instruction register

    Assert(_boot, "Mipc::MainLoop() called without boot?");

    _nfetched = 0;

    while (!_sim_exit)
    {
        AWAIT_P_PHI0; // @posedge
        // fetch at negegde
        AWAIT_P_PHI1; // @negedge
        if (_executing_syscall)
        {
            killIF_ID();
            continue;
        }
        else
        {
            if (_stall)
                continue;
#ifdef STALL_ON_BRANCH
            if (_isBranchInterlock)
            {
                killIF_ID();
                continue;
            }
#endif
            addr = _pc;
            ins = _mem->BEGetWord(addr, _mem->Read(addr & ~(LL)0x7));

            // forward the instruction to ID stage
            _pipe_regs_live.IF_ID._ins = ins;
            _pipe_regs_live.IF_ID._pc = _pc;

            _pc += 4;
            _nfetched++;
#ifdef MIPC_DEBUG
            fprintf(_debugLog, "<%llu> Fetched ins %#x from PC %#x\n", SIM_TIME, ins, _pc - 4);
#endif
        }
    }

    MipcDumpstats();
    Log::CloseLog();

#ifdef MIPC_DEBUG
    assert(_debugLog != NULL);
    fclose(_debugLog);
#endif

    exit(0);
}

void Mipc::MipcDumpstats()
{
    Log l('*');
    l.startLogging = 0;

    l.print("");
    l.print("************************************************************");
    l.print("");
    l.print("Number of instructions: %llu", _nfetched - 1);
    l.print("Number of simulated cycles: %llu", SIM_TIME);
    l.print("CPI: %.2f", ((double)SIM_TIME) / (_nfetched - 1));
    l.print("Int Conditional Branches: %llu", _num_cond_br);
    l.print("Jump and Link: %llu", _num_jal);
    l.print("Jump Register: %llu", _num_jr);
    l.print("Number of fp instructions: %llu", _fpinst);
    l.print("Number of loads: %llu", _num_load);
    l.print("Number of syscall emulated loads: %llu", _sys->_num_load);
    l.print("Number of stores: %llu", _num_store);
    l.print("Number of syscall emulated stores: %llu", _sys->_num_store);
    l.print("Number of load stalls: %llu", _num_load_stall);
    l.print("");
}

void Mipc::fake_syscall(unsigned int ins)
{
    _sys->pc = _pipe_regs_copy.MEM_WB._pc;
    _sys->quit = 0;
    _sys->EmulateSysCall();
    if (_sys->quit)
        _sim_exit = 1;
}

/*------------------------------------------------------------------------
 *
 *  Mipc::Reboot --
 *
 *   Reset processor state
 *
 *------------------------------------------------------------------------
 */
void Mipc::Reboot(char *image)
{
    FILE *fp;
    Log l('*');

    _boot = 0;

    if (image)
    {
        _boot = 1;
        printf("Executing %s\n", image);
        fp = fopen(image, "r");
        if (!fp)
        {
            fatal_error("Could not open `%s' for booting host!", image);
        }
        _mem->ReadImage(fp);
        fclose(fp);

        // Reset state
        _ins = 0;
        _insValid = FALSE;
        _decodeValid = FALSE;
        _execValid = FALSE;
        _memValid = FALSE;
        _insDone = TRUE;

        _num_load = 0;
        _num_store = 0;
        _fpinst = 0;
        _num_cond_br = 0;
        _num_jal = 0;
        _num_jr = 0;
        _num_load_stall = 0;

        _lastbd = 0;
        _bd = 0;
        _btaken = 0;
        _btgt = 0xdeadbeef;
        _sim_exit = 0;
        _executing_syscall = FALSE;
        _hasImm = FALSE;
#ifdef STALL_ON_BRANCH
        _isBranchInterlock = FALSE;
#endif
        _pipe_regs_live.ID_EX._isIllegalOp = FALSE;
        _pipe_regs_live.EX_MEM._isIllegalOp = FALSE;
        _pipe_regs_live.MEM_WB._isIllegalOp = FALSE;
#ifdef ENABLE_BYPASS
        _pipe_regs_live.ID_EX._bypassSrc1 = NONE;
        _pipe_regs_live.EX_MEM._bypassSrc1 = NONE;
        _pipe_regs_live.MEM_WB._bypassSrc1 = NONE;
#endif
        _pc = ParamGetInt("Mipc.BootPC"); // Boom! GO
    }
}

LL MipcSysCall::GetDWord(LL addr)
{
    _num_load++;
    return m->Read(addr);
}

void MipcSysCall::SetDWord(LL addr, LL data)
{

    m->Write(addr, data);
    _num_store++;
}

Word MipcSysCall::GetWord(LL addr)
{

    _num_load++;
    return m->BEGetWord(addr, m->Read(addr & ~(LL)0x7));
}

void MipcSysCall::SetWord(LL addr, Word data)
{

    m->Write(addr & ~(LL)0x7, m->BESetWord(addr, m->Read(addr & ~(LL)0x7), data));
    _num_store++;
}

void MipcSysCall::SetReg(int reg, LL val)
{
    _ms->_gpr[reg] = val;
}

LL MipcSysCall::GetReg(int reg)
{
    return _ms->_gpr[reg];
}

LL MipcSysCall::GetTime(void)
{
    return SIM_TIME;
}

void Mipc::killID_EX(void)
{
    _pipe_regs_live.ID_EX._ins = 0;
    _pipe_regs_live.ID_EX._isBranchIns = 0;
    _pipe_regs_live.ID_EX._isSyscall = 0;
    _pipe_regs_live.ID_EX._isIllegalOp = 0;
    _pipe_regs_live.ID_EX._pc = 0;
    _pipe_regs_live.ID_EX._isNoOp = 1;
    _pipe_regs_live.ID_EX._decodedDST = 0;
    _pipe_regs_live.ID_EX._opControl = 0;
    _pipe_regs_live.ID_EX._loWPort = FALSE;
    _pipe_regs_live.ID_EX._hiWPort = FALSE;
    _pipe_regs_live.ID_EX._isFloating = FALSE;
    _pipe_regs_live.ID_EX._writeFREG = FALSE;
    _pipe_regs_live.ID_EX._writeREG = FALSE;
    _pipe_regs_live.ID_EX._memControl = FALSE;
#ifdef ENABLE_BYPASS
    _pipe_regs_live.ID_EX._bypassSrc1 = NONE;
    _pipe_regs_live.ID_EX._bypassSrc2 = NONE;
#endif
}

void Mipc::killIF_ID(void)
{
    _pipe_regs_live.IF_ID._ins = 0;
    _pipe_regs_live.IF_ID._pc = 0;
    _pipe_regs_copy.IF_ID._isNoOp = 1;
}
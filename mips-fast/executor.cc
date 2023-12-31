#include "executor.h"

Exe::Exe(Mipc *mc)
{
    _mc = mc;
}

Exe::~Exe(void) {}

void Exe::MainLoop(void)
{
    unsigned int ins;
    Bool isSyscall, isIllegalOp;

    while (1)
    {
        AWAIT_P_PHI0; // @posedge
#ifdef STALL_ON_BRANCH
        _mc->_isBranchInterlock = FALSE; // write in posedge, read in negedge (IF)
#endif
        _mc->_pipe_regs_copy.ID_EX = _mc->_pipe_regs_live.ID_EX;
        ins = _mc->_pipe_regs_copy.ID_EX._ins;
        isSyscall = _mc->_pipe_regs_copy.ID_EX._isSyscall;
        isIllegalOp = _mc->_pipe_regs_copy.ID_EX._isIllegalOp;

        if (!isIllegalOp && !isSyscall && _mc->_pipe_regs_copy.ID_EX._opControl)
        {
#ifdef ENABLE_BYPASS
            switch (_mc->_pipe_regs_copy.ID_EX._bypassSrc1)
            {
            case EX:
/* code */
#ifdef MIPC_DEBUG
                fprintf(_mc->_debugLog, "bypassing %#x from EX to src1 for ins %#x\n", _mc->_pipe_regs_live.EX_MEM._opResultLo, ins);
#endif
                _mc->_pipe_regs_copy.ID_EX._decodedSRC1 = _mc->_pipe_regs_live.EX_MEM._opResultLo;
                break;
            case MEM:
#ifdef MIPC_DEBUG
                fprintf(_mc->_debugLog, "bypassing %#x from MEM to src1 for ins %#x\n", _mc->_pipe_regs_live.MEM_WB._opResultLo, ins);
#endif
                _mc->_pipe_regs_copy.ID_EX._decodedSRC1 = _mc->_pipe_regs_live.MEM_WB._opResultLo;
                break;
            case NONE:
                break;
            default:
                break;
            }
            switch (_mc->_pipe_regs_copy.ID_EX._bypassSrc2)
            {
            case EX:
/* code */
#ifdef MIPC_DEBUG
                fprintf(_mc->_debugLog, "bypassing %#x from EX to src2 for ins %#x\n", _mc->_pipe_regs_live.EX_MEM._opResultLo, ins);
#endif
                _mc->_pipe_regs_copy.ID_EX._decodedSRC2 = _mc->_pipe_regs_live.EX_MEM._opResultLo;
                break;
            case MEM:
#ifdef MIPC_DEBUG
                fprintf(_mc->_debugLog, "bypassing %#x from MEM to src2 for ins %#x\n", _mc->_pipe_regs_live.MEM_WB._opResultLo, ins);
#endif
                _mc->_pipe_regs_copy.ID_EX._decodedSRC2 = _mc->_pipe_regs_live.MEM_WB._opResultLo;
                break;
            case NONE:
                break;
            default:
                break;
            }

#endif
            _mc->_pipe_regs_copy.ID_EX._opControl(_mc, ins);
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Executed ins %#x\n", SIM_TIME, ins);
#endif
        }
        else if (isSyscall)
        {
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Deferring execution of syscall ins %#x\n", SIM_TIME, ins);
#endif
        }
        else
        {
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Illegal ins %#x in execution stage at PC %#x\n", SIM_TIME, ins, _mc->_pipe_regs_copy.ID_EX._pc);
#endif
        }

        if (!isIllegalOp && !isSyscall)
        {
#ifdef STALL_ON_BRANCH
            _mc->_isBranchInterlock = _mc->_pipe_regs_copy.ID_EX._isBranchIns;
#endif

            if (_mc->_pipe_regs_copy.ID_EX._isBranchIns && _mc->_btaken)
            {
                _mc->_pc = _mc->_pipe_regs_copy.ID_EX._btgt;
            }
        }

        AWAIT_P_PHI1; // @negedge
        _mc->_pipe_regs_live.EX_MEM = _mc->_pipe_regs_copy.ID_EX;
    }
}

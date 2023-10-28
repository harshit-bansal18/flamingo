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

    //     while (1)
    //     {
    //         AWAIT_P_PHI0; // @posedge
    //         if (_mc->_decodeValid)
    //         {
    //             ins = _mc->_ins;
    //             isSyscall = _mc->_isSyscall;
    //             isIllegalOp = _mc->_isIllegalOp;
    //             AWAIT_P_PHI1; // @negedge
    //             if (!isSyscall && !isIllegalOp)
    //             {
    //                 _mc->_opControl(_mc, ins);
    // #ifdef MIPC_DEBUG
    //                 fprintf(_mc->_debugLog, "<%llu> Executed ins %#x\n", SIM_TIME, ins);
    // #endif
    //             }
    //             else if (isSyscall)
    //             {
    // #ifdef MIPC_DEBUG
    //                 fprintf(_mc->_debugLog, "<%llu> Deferring execution of syscall ins %#x\n", SIM_TIME, ins);
    // #endif
    //             }
    //             else
    //             {
    // #ifdef MIPC_DEBUG
    //                 fprintf(_mc->_debugLog, "<%llu> Illegal ins %#x in execution stage at PC %#x\n", SIM_TIME, ins, _mc->_pc);
    // #endif
    //             }
    //             _mc->_decodeValid = FALSE;
    //             _mc->_execValid = TRUE;

    //             if (!isIllegalOp && !isSyscall)
    //             {
    //                 if (_mc->_lastbd && _mc->_btaken)
    //                 {
    //                     _mc->_pc = _mc->_btgt;
    //                 }
    //                 else
    //                 {
    //                     _mc->_pc = _mc->_pc + 4;
    //                 }
    //                 _mc->_lastbd = _mc->_bd;
    //             }
    //         }
    //         else
    //         {
    //             PAUSE(1);
    //         }
    //     }

    while (1)
    {
        AWAIT_P_PHI0; // @posedge
#ifdef BRANCH_INTERLOCK
        _mc->_isBranchInterlock = FALSE; // write in posedge, read in negedge (IF)
#endif
        _mc->_pipe_regs_copy.ID_EX = _mc->_pipe_regs_live.ID_EX;
        ins = _mc->_pipe_regs_copy.ID_EX._ins;
        isSyscall = _mc->_pipe_regs_copy.ID_EX._isSyscall;
        isIllegalOp = _mc->_pipe_regs_copy.ID_EX._isIllegalOp;

        if (!isIllegalOp && !isSyscall && _mc->_pipe_regs_copy.ID_EX._opControl)
        {
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

        // branch -> IF ID(bd=1; btgt=x) EX(lastbd = bd = 1)
        // delay ->                                    IF              ID()    EX(lastbd = 1; pc + 4)
        // target -> 
        // need to correct these
        if (!isIllegalOp && !isSyscall)
        {
            fprintf(_mc->_debugLog, "<%llu> btaken:%d isBranch: %d ins %#x\n", SIM_TIME, _mc->_pipe_regs_copy.ID_EX._isBranchIns, _mc->_btaken, ins);

#ifdef BRANCH_INTERLOCK
            _mc->_isBranchInterlock = _mc->_pipe_regs_copy.ID_EX._isBranchIns;
#endif

            if (_mc->_pipe_regs_copy.ID_EX._isBranchIns && _mc->_btaken)
            {
                _mc->_pc = _mc->_pipe_regs_copy.ID_EX._btgt;
            }
            // else if(_mc->_pipe_regs_copy.ID_EX._isBranchIns)
            // {
            //     _mc->_pc = _mc->_pipe_regs_copy.ID_EX._pc + 8;
            // }
            // _mc->_lastbd = _mc->_bd;
        }

        AWAIT_P_PHI1; // @negedge
        _mc->_pipe_regs_live.EX_MEM = _mc->_pipe_regs_copy.ID_EX;
    }
}

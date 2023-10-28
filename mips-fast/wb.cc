#include "wb.h"

Writeback::Writeback(Mipc *mc)
{
    _mc = mc;
}

Writeback::~Writeback(void) {}

void Writeback::MainLoop(void)
{
    unsigned int ins;
    Bool writeReg;
    Bool writeFReg;
    Bool loWPort;
    Bool hiWPort;
    Bool isSyscall;
    Bool isIllegalOp;
    unsigned decodedDST;
    unsigned opResultLo, opResultHi;

    //     while (1)
    //     {
    //         AWAIT_P_PHI0; // @posedge
    //         // Sample the important signals
    //         if (_mc->_memValid)
    //         {
    //             writeReg = _mc->_writeREG;
    //             writeFReg = _mc->_writeFREG;
    //             loWPort = _mc->_loWPort;
    //             hiWPort = _mc->_hiWPort;
    //             decodedDST = _mc->_decodedDST;
    //             opResultLo = _mc->_opResultLo;
    //             opResultHi = _mc->_opResultHi;
    //             isSyscall = _mc->_isSyscall;
    //             isIllegalOp = _mc->_isIllegalOp;
    //             ins = _mc->_ins;
    //             AWAIT_P_PHI1; // @negedge
    //             if (isSyscall)
    //             {
    // #ifdef MIPC_DEBUG
    //                 fprintf(_mc->_debugLog, "<%llu> SYSCALL! Trapping to emulation layer at PC %#x\n", SIM_TIME, _mc->_pc);
    // #endif
    //                 _mc->_opControl(_mc, ins);
    //                 _mc->_pc += 4;
    //             }
    //             else if (isIllegalOp)
    //             {
    //                 printf("Illegal ins %#x at PC %#x. Terminating simulation!\n", ins, _mc->_pc);
    // #ifdef MIPC_DEBUG
    //                 fclose(_mc->_debugLog);
    // #endif
    //                 printf("Register state on termination:\n\n");
    //                 _mc->dumpregs();
    //                 exit(0);
    //             }
    //             else
    //             {
    //                 if (writeReg)
    //                 {
    //                     _mc->_gpr[decodedDST] = opResultLo;
    // #ifdef MIPC_DEBUG
    //                     fprintf(_mc->_debugLog, "<%llu> Writing to reg %u, value: %#x\n", SIM_TIME, decodedDST, opResultLo);
    // #endif
    //                 }
    //                 else if (writeFReg)
    //                 {
    //                     _mc->_fpr[(decodedDST) >> 1].l[FP_TWIDDLE ^ ((decodedDST)&1)] = opResultLo;
    // #ifdef MIPC_DEBUG
    //                     fprintf(_mc->_debugLog, "<%llu> Writing to freg %u, value: %#x\n", SIM_TIME, decodedDST >> 1, opResultLo);
    // #endif
    //                 }
    //                 else if (loWPort || hiWPort)
    //                 {
    //                     if (loWPort)
    //                     {
    //                         _mc->_lo = opResultLo;
    // #ifdef MIPC_DEBUG
    //                         fprintf(_mc->_debugLog, "<%llu> Writing to Lo, value: %#x\n", SIM_TIME, opResultLo);
    // #endif
    //                     }
    //                     if (hiWPort)
    //                     {
    //                         _mc->_hi = opResultHi;
    // #ifdef MIPC_DEBUG
    //                         fprintf(_mc->_debugLog, "<%llu> Writing to Hi, value: %#x\n", SIM_TIME, opResultHi);
    // #endif
    //                     }
    //                 }
    //             }
    //             _mc->_gpr[0] = 0;
    //             _mc->_memValid = FALSE;
    //             _mc->_insDone = TRUE;
    //         }
    //         else
    //         {
    //             PAUSE(1);
    //         }
    //     }
    printf("starting wb main loop..\n");
    while (1)
    {
        AWAIT_P_PHI0; // @posedge
        _mc->_pipe_regs_copy.MEM_WB = _mc->_pipe_regs_live.MEM_WB;

        writeReg = _mc->_pipe_regs_copy.MEM_WB._writeREG;
        writeFReg = _mc->_pipe_regs_copy.MEM_WB._writeFREG;
        loWPort = _mc->_pipe_regs_copy.MEM_WB._loWPort;
        hiWPort = _mc->_pipe_regs_copy.MEM_WB._hiWPort;
        decodedDST = _mc->_pipe_regs_copy.MEM_WB._decodedDST;
        opResultLo = _mc->_pipe_regs_copy.MEM_WB._opResultLo;
        opResultHi = _mc->_pipe_regs_copy.MEM_WB._opResultHi;
        isSyscall = _mc->_pipe_regs_copy.MEM_WB._isSyscall;
        isIllegalOp = _mc->_pipe_regs_copy.MEM_WB._isIllegalOp;
        ins = _mc->_pipe_regs_copy.MEM_WB._ins;

        if (isSyscall)
        {
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> SYSCALL! Trapping to emulation layer at PC %#x\n", SIM_TIME, _mc->_pipe_regs_copy.MEM_WB._pc);
#endif
            _mc->_pipe_regs_copy.MEM_WB._opControl(_mc, ins);
            _mc->_pc = _mc->_pipe_regs_copy.MEM_WB._pc + 4;
            _mc->_syscall_in_pipe = FALSE; // update at posedge. 
        }
        else if (isIllegalOp)
        {
            printf("Illegal ins %#x at PC %#x. Terminating simulation!\n", ins, _mc->_pipe_regs_copy.MEM_WB._pc);
#ifdef MIPC_DEBUG
            fclose(_mc->_debugLog);
#endif
            printf("Register state on termination:\n\n");
            _mc->dumpregs();
            exit(0);
        }
        else
        {
            if (writeReg)
            {
                _mc->_gpr[decodedDST] = opResultLo;
#ifdef MIPC_DEBUG
                fprintf(_mc->_debugLog, "<%llu> Writing to reg %u, value: %#x\n", SIM_TIME, decodedDST, opResultLo);
#endif
            }
            else if (writeFReg)
            {
                _mc->_fpr[(decodedDST) >> 1].l[FP_TWIDDLE ^ ((decodedDST)&1)] = opResultLo;
#ifdef MIPC_DEBUG
                fprintf(_mc->_debugLog, "<%llu> Writing to freg %u, value: %#x\n", SIM_TIME, decodedDST >> 1, opResultLo);
#endif
            }
            else if (loWPort || hiWPort)
            {
                if (loWPort)
                {
                    _mc->_lo = opResultLo;
#ifdef MIPC_DEBUG
                    fprintf(_mc->_debugLog, "<%llu> Writing to Lo, value: %#x\n", SIM_TIME, opResultLo);
#endif
                }
                if (hiWPort)
                {
                    _mc->_hi = opResultHi;
#ifdef MIPC_DEBUG
                    fprintf(_mc->_debugLog, "<%llu> Writing to Hi, value: %#x\n", SIM_TIME, opResultHi);
#endif
                }
            }
        }
        _mc->_gpr[0] = 0;

        AWAIT_P_PHI1; // @negedge 
        // do nothing
    }
}

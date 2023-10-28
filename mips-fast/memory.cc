#include "memory.h"

Memory::Memory(Mipc *mc)
{
    _mc = mc;
}

Memory::~Memory(void) {}

void Memory::MainLoop(void)
{
    Bool memControl;

    while (1)
    {
        AWAIT_P_PHI0; // @posedge
        _mc->_pipe_regs_copy.EX_MEM = _mc->_pipe_regs_live.EX_MEM;

        memControl = _mc->_pipe_regs_copy.EX_MEM._memControl;


        AWAIT_P_PHI1; // @negedge

        if (memControl)
        {
            _mc->_pipe_regs_copy.EX_MEM._memOp(_mc);
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Accessing memory at address %#x for ins %#x\n", SIM_TIME, _mc->_pipe_regs_copy.EX_MEM._MAR, _mc->_pipe_regs_copy.EX_MEM._ins);
#endif
        }
        else
        {
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Memory has nothing to do for ins %#x\n", SIM_TIME, _mc->_pipe_regs_copy.EX_MEM._ins);
#endif
        }


        _mc->_pipe_regs_live.MEM_WB = _mc->_pipe_regs_copy.EX_MEM;
    }
}

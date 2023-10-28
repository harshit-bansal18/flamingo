#include "decode.h"

Decode::Decode(Mipc *mc)
{
    _mc = mc;
}

Decode::~Decode(void) {}

void Decode::MainLoop(void)
{

    unsigned int dataStalls = 0;
    while (1)
    {
        AWAIT_P_PHI0; // @posedge
        _mc->_regSRC1 = 100;
        _mc->_regSRC2 = 100;
        _mc->_bd = 0;
        _mc->_stall = 0;
        dataStalls = 0;
        // copy the IF_ID regs from pipeline reg
        _mc->_pipe_regs_copy.IF_ID = _mc->_pipe_regs_live.IF_ID;

        // need to stall at negedge if dependency found
        _mc->Dec(_mc->_pipe_regs_copy.IF_ID._ins);

#ifdef BYPASS_ENABLED
        printf("bypass enabled\n");
#else
        // check for interlock logic here
        // If source registers match for destination registers in last two instructions
        if (!_mc->_isIllegalOp)
        {
            if(!_mc->_isFloating) {
                if(_mc->_regSRC1 != 0 && _mc->_regSRC1 != 100) {
                    if(_mc->_pipe_regs_live.ID_EX._writeREG && _mc->_regSRC1 == _mc->_pipe_regs_live.ID_EX._decodedDST)
                        dataStalls = 1;
                    else if (_mc->_pipe_regs_live.EX_MEM._writeREG && _mc->_regSRC1 == _mc->_pipe_regs_live.EX_MEM._decodedDST)
                        dataStalls = 1;
                }
                if(_mc->_regSRC2 != 0 && _mc->_regSRC2 != 100) {
                    if(_mc->_pipe_regs_live.ID_EX._writeREG && _mc->_regSRC2 == _mc->_pipe_regs_live.ID_EX._decodedDST)
                        dataStalls = 1;
                    else if (_mc->_pipe_regs_live.EX_MEM._writeREG && _mc->_regSRC2 == _mc->_pipe_regs_live.EX_MEM._decodedDST)
                        dataStalls = 1;
                }
            } else {
                if(_mc->_regSRC1 != 100) {
                    if(_mc->_pipe_regs_live.ID_EX._writeFREG && _mc->_regSRC1 == _mc->_pipe_regs_live.ID_EX._decodedDST)
                        dataStalls = 1;
                    else if (_mc->_pipe_regs_live.EX_MEM._writeFREG && _mc->_regSRC1 == _mc->_pipe_regs_live.EX_MEM._decodedDST)
                        dataStalls = 1;
                }
                if(_mc->_regSRC2 != 100) {
                    if(_mc->_pipe_regs_live.ID_EX._writeFREG && _mc->_regSRC2 == _mc->_pipe_regs_live.ID_EX._decodedDST)
                        dataStalls = 1;
                    else if (_mc->_pipe_regs_live.EX_MEM._writeFREG && _mc->_regSRC2 == _mc->_pipe_regs_live.EX_MEM._decodedDST)
                        dataStalls = 1;
                }
            }

            if(_mc->_readHi && (_mc->_pipe_regs_live.EX_MEM._hiWPort || _mc->_pipe_regs_live.ID_EX._hiWPort))
                dataStalls = 1;
            if(_mc->_readLo && (_mc->_pipe_regs_live.EX_MEM._loWPort || _mc->_pipe_regs_live.ID_EX._loWPort))
                dataStalls = 1;
        }
        if (_mc->_isSyscall)
        {
            #ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> setting syscall-true %#x\n", SIM_TIME, _mc->_pipe_regs_copy.IF_ID._ins);
            #endif
            _mc->_syscall_in_pipe = TRUE;
        }
        else if (dataStalls > 0)
        {
            _mc->_stall = TRUE;
            // _mc->_pc = _mc->_pipe_regs_copy.IF_ID._pc; // race with execute 
        }

#endif
        AWAIT_P_PHI1; // @negedge
        // read the register file at negedge
        if (!_mc->_stall)
        {
            _mc->Dec(_mc->_pipe_regs_copy.IF_ID._ins);
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Decoded ins %#x\n", SIM_TIME, _mc->_pipe_regs_copy.IF_ID._ins);
#endif

            // update the ID_EX pipeline regs
            _mc->_pipe_regs_live.ID_EX._ins = _mc->_pipe_regs_copy.IF_ID._ins;
            _mc->_pipe_regs_live.ID_EX._pc = _mc->_pipe_regs_copy.IF_ID._pc;
            _mc->_pipe_regs_live.ID_EX._decodedDST = _mc->_decodedDST;
            _mc->_pipe_regs_live.ID_EX._decodedSRC1 = _mc->_decodedSRC1;
            _mc->_pipe_regs_live.ID_EX._decodedSRC2 = _mc->_decodedSRC2;
            _mc->_pipe_regs_live.ID_EX._isIllegalOp = _mc->_isIllegalOp;
            _mc->_pipe_regs_live.ID_EX._isSyscall = _mc->_isSyscall;
            _mc->_pipe_regs_live.ID_EX._decodedShiftAmt = _mc->_decodedShiftAmt;
            _mc->_pipe_regs_live.ID_EX._hiWPort = _mc->_hiWPort;
            _mc->_pipe_regs_live.ID_EX._loWPort = _mc->_loWPort;
            _mc->_pipe_regs_live.ID_EX._memOp = _mc->_memOp;
            _mc->_pipe_regs_live.ID_EX._opControl = _mc->_opControl;
            _mc->_pipe_regs_live.ID_EX._memControl = _mc->_memControl;
            _mc->_pipe_regs_live.ID_EX._writeFREG = _mc->_writeFREG;
            _mc->_pipe_regs_live.ID_EX._writeREG = _mc->_writeREG;
            _mc->_pipe_regs_live.ID_EX._rs = _mc->_regSRC1;
            _mc->_pipe_regs_live.ID_EX._rt = _mc->_regSRC2;
            _mc->_pipe_regs_live.ID_EX._isBranchIns = _mc->_bd;
            _mc->_pipe_regs_live.ID_EX._btgt = _mc->_btgt;
            _mc->_pipe_regs_live.ID_EX._isFloating = _mc->_isFloating;
        }
        else
        {
            _mc->killID_EX();
        }
    }
}

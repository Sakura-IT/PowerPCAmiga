// Copyright (c) 2019 Dennis van der Boon
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <exec/types.h>
#include "libstructs.h"
#include "constants.h"


void    Reset(void);
ULONG   ReadPVR(void);
ULONG   SetIdle(void);


static ULONG getLeadZ(ULONG value)="\tcntlzw\tr3,r3\n";
static ULONG getPVR(void)="\tmfpvr\tr3\n";

void mmuSetup(void)
{
    ULONG leadZeros;
    struct PPCZeroPage *myZP = 0;
    ULONG PTSizeShift        = (ULONG)(myZP->zp_MemSize);
    ULONG shiftVal           = 24;

    leadZeros   = getLeadZ(PTSizeShift);
    PTSizeShift = shiftVal - leadZeros;

    myZP->zp_PageTableSize = (1<<PTSizeShift);

    //bl .SetupPT

    return;
}

void installExceptions(ULONG Src)
{
    ULONG memExc = 0x100;
    for (int i=0; i<20; i++)
    {
        *((ULONG*)(memExc))   = *((ULONG*)(Src));
        *((ULONG*)(memExc+4)) = *((ULONG*)(Src+4));

        memExc += 0x100;
    } //add kernel copy

    return;
}

void killerFIFOs(struct InitData* initData)
{
    ULONG memBase  = initData->id_MemBase;
    ULONG memFIFO  = memBase + BASE_KMSG;
    ULONG memFIFO2 = memFIFO + SIZE_KBASE;
    ULONG memIF    = FIFO_OFFSET;
    ULONG memIP    = memIF + SIZE_KFIFO;
    ULONG memOF    = memIF + (SIZE_KFIFO * 2);
    ULONG memOP    = memIF + (SIZE_KFIFO * 3);

    ULONG memIF2   = memIF;
    ULONG memIP2   = memIP;
    ULONG memOF2   = memOF;
    ULONG memOP2   = memOP;

    struct killFIFO* baseFIFO = (struct killFIFO*)(memIF + (SIZE_KFIFO *4));

    for (int i=0; i<4096; i++)
    {
        *((ULONG*)(memIP)) = 0;
        *((ULONG*)(memOP)) = 0;
        *((ULONG*)(memIF)) = memFIFO;
        *((ULONG*)(memOF)) = memFIFO2;

        memIP    += 4;
        memOP    += 4;
        memIF    += 4;
        memOF    += 4;
        memFIFO  += 192;
        memFIFO2 += 192;
    }

    baseFIFO->kf_MIIFT = memBase + memIF2 + 4;
    baseFIFO->kf_MIIFH = memBase + memIF2;
    baseFIFO->kf_MIIPT = memBase + memIP2;
    baseFIFO->kf_MIIPH = memBase + memIP2;
    baseFIFO->kf_MIOFH = memBase + memOF2;
    baseFIFO->kf_MIOFT = memBase + memOF2 + 4;
    baseFIFO->kf_MIOPT = memBase + memOP2;
    baseFIFO->kf_MIOPH = memBase + memOP2;

    return;
}

__section (".setupppc","acrx") void setupPPC(struct InitData* initData)
{
    ULONG mem = 0;
    ULONG copySrc = 0;
    struct PPCZeroPage *myZP = 0;

    initData->id_Status = 0x496e6974;    //Init

    Reset();

    for (LONG i=0; i<64; i++)
    {
        *((ULONG*)(mem)) = 0;
        mem += 4;
    }

    ULONG myPVR = getPVR();

    if ((myPVR>>16) == ID_MPC834X)
    {
        myZP->zp_MemSize = initData->id_MemSize;

        killerFIFOs(initData);

	    copySrc = SetIdle();
    }

    if(!(copySrc))
    {
        while (1)
        {
            initData->id_Status = 0x45727234;
        }
    }

    installExceptions(copySrc);

    mmuSetup();

    myZP->zp_PPCMemBase = 0x426f6f6e;   //Boon

fakeEnd:
    goto fakeEnd;
}


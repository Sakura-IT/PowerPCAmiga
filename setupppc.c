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
#include "internalsppc.h"


BOOL setupTBL(ULONG startEffAddr, ULONG endEffAddr, ULONG physAddr, ULONG WIMG, ULONG ppKey)
{
    ULONG currSR, uPTE, lPTE, hash, hashTwo, mySDR, PTEG, tempPTEG, flag, checkPTEG;

    struct PPCZeroPage *myZP = 0;

    while (startEffAddr < endEffAddr)
    {
        flag   = 0;
        currSR = getSRIn(startEffAddr);
        uPTE   = ((((currSR << 7) & 0x7fffff80) | ((startEffAddr >> 22) & 0x3f)) | 0x80000000);
        lPTE   = (((physAddr & 0xfffff000) | ((WIMG << 3) & 0x78)) | 0x180 | ppKey);

	hash = (((startEffAddr >> 12) & 0xffff) ^ (currSR & 0x0007ffff));

        mySDR = getSDR1();

        while (1)
        {
            tempPTEG = ((((hash >> 10) & 0x1ff) & mySDR) | ((mySDR >> 16) & 0x1ff));
            PTEG = 0;
            PTEG = (((hash << 6) & 0xffc0) | ((tempPTEG << 16) & 0x01ff0000) | (mySDR & 0x3f));

            for (int n=0; n<8; n++)
            {
                checkPTEG = *((ULONG*)(PTEG));

                if (!(checkPTEG & 0x80000000))
                {
                    *((ULONG*)(PTEG))   = uPTE;
                    *((ULONG*)(PTEG) + 1) = lPTE;

                    startEffAddr += 4096;
                    physAddr     += 4096;
                    flag = 1;
                    break;
                }
                
                PTEG += 4;
            }

            if (flag)
            {
                break;
            }

            if (uPTE & 0x40)
            {
                return FALSE;
            }

            hash ^= 0xffffffff;
            uPTE |= 0x40;
        }

    }

    return TRUE;
}

void setupPT(void)
{
    struct PPCZeroPage *myZP = 0;

    ULONG memSize = myZP->zp_MemSize;
    ULONG ptSize  = myZP->zp_PageTableSize;

    if (ptSize < 0x100000)
    {
        ptSize = 0x100000;
    }

    ULONG ptLoc    = memSize - ptSize;
    LONG  HTABMASK = 0xffff;
    ULONG mask     = (HTABMASK <<16);
    ULONG HTABORG  = (memSize & mask);
    ULONG testVal  = (HTABORG & mask);

    while (!(testVal) || !(HTABMASK))
    {
        HTABMASK = HTABMASK >> 1;
        mask = HTABMASK << 16;
        testVal = (HTABORG & mask);
    }

    HTABORG |= HTABMASK;
    setSDR1(HTABORG);

    ULONG numClear = ptSize >> 2;

    for (int i = 0; i < numClear; i++)
    {
        *((ULONG*)(ptLoc)) = 0;
        ptLoc += 4;
    }

    ULONG keyVal = 0x20000000;
    ULONG segVal = 0;

    for (int i = 0; i < 16; i++)
    {
        setSRIn(keyVal, segVal);
	segVal += 0x10000000;
        keyVal += 1;
    }
}

void mmuSetup(void)
{
    ULONG leadZeros;
    struct PPCZeroPage *myZP = 0;
    ULONG PTSizeShift        = (ULONG)(myZP->zp_MemSize);
    ULONG shiftVal           = 24;

    leadZeros   = getLeadZ(PTSizeShift);
    PTSizeShift = shiftVal - leadZeros;

    myZP->zp_PageTableSize = (1<<PTSizeShift);

    setupPT();

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

    struct killFIFO* baseFIFO = (struct killFIFO*)(memIF + (SIZE_KFIFO *4));

    baseFIFO->kf_MIIFT = memBase + memIF + 4;
    baseFIFO->kf_MIIFH = memBase + memIF;
    baseFIFO->kf_MIIPT = memBase + memIP;
    baseFIFO->kf_MIIPH = memBase + memIP;
    baseFIFO->kf_MIOFH = memBase + memOF;
    baseFIFO->kf_MIOFT = memBase + memOF + 4;
    baseFIFO->kf_MIOPT = memBase + memOP;
    baseFIFO->kf_MIOPH = memBase + memOP;

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


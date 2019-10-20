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
    ULONG currSR, uPTE, lPTE, hash, mySDR, PTEG, tempPTEG, flag, checkPTEG;

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
            PTEG = (((hash << 6) & 0xffc0) | ((tempPTEG << 16) & 0x01ff0000) | (mySDR & 0xfc000000));

            for (int n=8; n>0; n--)
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

void mmuSetup(struct InitData* initData)
{
    ULONG leadZeros, ibatl, ibatu, dbatl, dbatu, batSize;
    ULONG startEffAddr, endEffAddr, physAddr, WIMG, ppKey;
    struct PPCZeroPage *myZP = 0;
    ULONG PTSizeShift        = (ULONG)(myZP->zp_MemSize);
    ULONG shiftVal           = 24;

    leadZeros   = getLeadZ(PTSizeShift);
    PTSizeShift = shiftVal - leadZeros;

    myZP->zp_PageTableSize = (1<<PTSizeShift);

    setupPT();

    ULONG myPVR = getPVR();

    if ((myPVR >> 16) == ID_MPC834X)
    {
        startEffAddr = IMMR_ADDR_DEFAULT;
    }
//    else
//    {
//        error
//    }
    endEffAddr   = startEffAddr + 0x100000;
    physAddr     = startEffAddr;
    WIMG         = PTE_CACHE_INHIBITED|PTE_GUARDED;
    ppKey        = PP_USER_RW;

    setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey);

    startEffAddr = VECTOR_TABLE_DEFAULT;
    endEffAddr   = startEffAddr + 0x20000;
    physAddr     = startEffAddr;
    WIMG         = PTE_CACHE_INHIBITED;
    ppKey        = PP_USER_RW;

    setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey);

    switch (initData->id_GfxType)
    {
        case VENDOR_ATI:
        {
            startEffAddr = initData->id_GfxConfigBase;
            endEffAddr   = startEffAddr + 0x10000;
            physAddr     = startEffAddr + 0x60000000;
            WIMG         = PTE_CACHE_INHIBITED|PTE_GUARDED;
            ppKey        = PP_USER_RW;

            setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey);

            if ((initData->id_GfxMemBase & 0xf7ffffff) || (initData->id_GfxConfigBase & 0xf7ffffff))
            {
                batSize = BAT_BL_128M;
            }
            else
            {
                batSize = BAT_BL_256M;
            }

            ibatl = ((initData->id_GfxMemBase + 0x60000000) | BAT_READ_WRITE);
            ibatu = initData->id_GfxMemBase | batSize | BAT_VALID_SUPERVISOR | BAT_VALID_USER;
            dbatl = ibatl | BAT_WRITE_THROUGH;
            dbatu = ibatu;

            setBAT1(ibatl, ibatu, dbatl, dbatu);
            mSync();

            break;
        }

        case VENDOR_3DFX:
        {
            startEffAddr = initData->id_GfxMemBase;
            endEffAddr   = startEffAddr + 0x2000000;
            if (initData->id_GfxSubType == DEVICE_VOODOO45)
            {
                endEffAddr += 0x6000000;
            }
            physAddr     = startEffAddr + 0x60000000;
            WIMG         = PTE_CACHE_INHIBITED|PTE_GUARDED;
            ppKey        = PP_USER_RW;

            setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey);

            ibatu = initData->id_GfxMemBase + 0x2000000;
            if (initData->id_GfxSubType == DEVICE_VOODOO45)
            {
                ibatu += 0x6000000;
            }
            ibatl = ibatu + 0x60000000 | BAT_READ_WRITE;
            dbatl = ibatl | BAT_WRITE_THROUGH;
            ibatu |= (BAT_BL_32M | BAT_VALID_SUPERVISOR | BAT_VALID_USER);
            dbatu = ibatu;

            setBAT1(ibatl, ibatu, dbatl, dbatu);
            mSync();

            break;
        }

        default:
        {
            //error
            break;
        }
    }

    ibatl = BAT_READ_WRITE;
    ibatu = BAT_BL_2M | BAT_VALID_SUPERVISOR;
    dbatl = ibatl;
    dbatu = ibatu;

    setBAT0(ibatl, ibatu, dbatl, dbatu);
    mSync();

    ibatl = 0x200000 | BAT_READ_WRITE;
    ibatu = (initData->id_MemBase + 0x200000) | BAT_BL_2M | BAT_VALID_SUPERVISOR | BAT_VALID_USER;
    dbatl = ibatl | BAT_CACHE_INHIBITED;
    dbatu = ibatu;

    setBAT2(ibatl, ibatu, dbatl, dbatu);

                                     //adding using second ATI card as memory
    physAddr     = 0x400000;
    startEffAddr = physAddr + (initData->id_MemBase);
    endEffAddr   = startEffAddr + (initData->id_MemSize);
    WIMG         = PTE_COPYBACK;
    ppKey        = PP_USER_RW;

    setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey);

    physAddr    = 0;

    for (int n=64; n>0; n--)
    {
        tlbIe(physAddr);
        physAddr += 0x1000;
    }

    tlbSync();

    setMSR(PSL_IR | PSL_DR | PSL_FP);

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

    mmuSetup(initData);

    myZP->zp_PPCMemBase = 0x426f6f6e;   //Boon

fakeEnd:
    goto fakeEnd;
}


// Copyright (c) 2019, 2020 Dennis van der Boon
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

#pragma pack(push,2)
#include <exec/types.h>
#include <proto/powerpc.h>
#include <powerpc/semaphoresPPC.h>
#include "libstructs.h"
#include "constants.h"
#include "internalsppc.h"
#pragma pack(pop)

/********************************************************************************************
*
*
*
*********************************************************************************************/

BOOL setupTBL(ULONG startEffAddr, ULONG endEffAddr, ULONG physAddr, ULONG WIMG, ULONG ppKey)
{
    ULONG currSR, uPTE, lPTE, hash, mySDR, PTEG, tempPTEG, flag, checkPTEG;

    while (startEffAddr < endEffAddr)
    {
        flag   = 0;
        currSR = getSRIn(startEffAddr);
        uPTE   = ((((currSR << 7) & 0x7fffff80) | ((startEffAddr >> 22) & 0x3f)) | PTE_VALID);
        lPTE   = (((physAddr & 0xfffff000) | ((WIMG << 3) & 0x78)) | PTE_REFERENCED | PTE_CHANGED | ppKey);

        hash = (((startEffAddr >> 12) & 0xffff) ^ (currSR & 0x0007ffff));

        mySDR = getSDR1();

        while (1)
        {
            tempPTEG = ((((hash >> 10) & 0x1ff) & mySDR) | ((mySDR >> 16) & 0x1ff));
            PTEG = (((hash << 6) & 0xffc0) | ((tempPTEG << 16) & 0x01ff0000) | (mySDR & 0xfe000000));

            for (int n=8; n>0; n--)
            {
                checkPTEG = *((ULONG*)(PTEG));

                if (!(checkPTEG & PTE_VALID))
                {
                    *((ULONG*)(PTEG))     = uPTE;
                    *((ULONG*)(PTEG) + 1) = lPTE;

                    startEffAddr += MMU_PAGESIZE;
                    physAddr     += MMU_PAGESIZE;
                    flag = 1;
                    break;
                }
                
                PTEG += 8;
            }

            if (flag)
            {
                break;
            }

            if (uPTE & PTE_HASHID)
            {
                return FALSE;
            }

            hash ^= 0xffffffff;
            uPTE |= PTE_HASHID;
        }

    }

    return TRUE;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

void setupPT(void)
{
    struct PPCZeroPage *myZP = 0;

    ULONG memSize = myZP->zp_MemSize;
    ULONG ptSize  = myZP->zp_PageTableSize;

    if (ptSize < 0x10000)
    {
        ptSize = 0x10000;
    }

    ULONG ptLoc    = memSize - ptSize;
    LONG  HTABMASK = 0xffff;
    ULONG mask     = (HTABMASK <<16);
    ULONG HTABORG  = (ptLoc & mask);
    ULONG testVal  = (HTABORG & mask);

    while ((testVal) && (HTABMASK))
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

/********************************************************************************************
*
*
*
*********************************************************************************************/

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
    else
    {
        initData->id_Status = 0x45727234;
        return;
    }
    endEffAddr   = startEffAddr + 0x100000;
    physAddr     = startEffAddr;
    WIMG         = PTE_CACHE_INHIBITED|PTE_GUARDED;
    ppKey        = PP_USER_RW;

    if (!(setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey)))
    {
        initData->id_Status = 0x45727231;
        return;
    }

    startEffAddr = VECTOR_TABLE_DEFAULT;
    endEffAddr   = startEffAddr + 0x20000;
    physAddr     = startEffAddr;
    WIMG         = PTE_CACHE_INHIBITED;
    ppKey        = PP_USER_RW;

    if (!(setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey)))
    {
        initData->id_Status = 0x45727231;
        return;
    }

    switch (initData->id_GfxType)
    {
        case VENDOR_ATI:
        {
            startEffAddr = initData->id_GfxConfigBase;
            endEffAddr   = startEffAddr + 0x10000;
            physAddr     = startEffAddr + OFFSET_PCIMEM;
            WIMG         = PTE_CACHE_INHIBITED|PTE_GUARDED;
            ppKey        = PP_USER_RW;

            if (!(setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey)))
            {
                initData->id_Status = 0x45727231;
                return;
            }

            if ((initData->id_GfxMemBase & 0xf7ffffff) || (initData->id_GfxConfigBase & 0xf7ffffff))
            {
                batSize = BAT_BL_128M;
            }
            else
            {
                batSize = BAT_BL_256M;
            }

            ibatl = ((initData->id_GfxMemBase + OFFSET_PCIMEM) | BAT_READ_WRITE);
            ibatu = initData->id_GfxMemBase | batSize | BAT_VALID_SUPERVISOR | BAT_VALID_USER;
            dbatl = ibatl | BAT_WRITE_THROUGH;

            setBAT1(ibatl, ibatu, dbatl, ibatu);
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
            physAddr     = startEffAddr + OFFSET_PCIMEM;
            WIMG         = PTE_CACHE_INHIBITED|PTE_GUARDED;
            ppKey        = PP_USER_RW;

            if (!(setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey)))
            {
                initData->id_Status = 0x45727231;
                return;
            }

            ibatu = initData->id_GfxMemBase + 0x2000000;
            if (initData->id_GfxSubType == DEVICE_VOODOO45)
            {
                ibatu += 0x6000000;
            }
            ibatl = ibatu + OFFSET_PCIMEM | BAT_READ_WRITE;
            dbatl = ibatl | BAT_WRITE_THROUGH;
            ibatu |= (BAT_BL_32M | BAT_VALID_SUPERVISOR | BAT_VALID_USER);

            setBAT1(ibatl, ibatu, dbatl, ibatu);
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

    setBAT0(ibatl, ibatu, ibatl, ibatu);
    mSync();

    ibatl = OFFSET_MESSAGES | BAT_READ_WRITE;
    ibatu = (initData->id_MemBase + OFFSET_MESSAGES) | BAT_BL_2M | BAT_VALID_SUPERVISOR | BAT_VALID_USER;
    dbatl = ibatl | BAT_CACHE_INHIBITED;

    setBAT2(ibatl, ibatu, dbatl, ibatu);

                                     //adding using second ATI card as memory
    physAddr     = OFFSET_SYSMEM;
    startEffAddr = physAddr + (initData->id_MemBase);
    endEffAddr   = (initData->id_MemBase) + (initData->id_MemSize);
    WIMG         = PTE_COPYBACK;
    ppKey        = PP_USER_RW;

    if (!(setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey)))
    {
        initData->id_Status = 0x45727231;
        return;
    }

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

/********************************************************************************************
*
*
*
*********************************************************************************************/

void installExceptions(void)
{
    UWORD excVector;
    ULONG ExcDataTable;
    ULONG ExcCode;

    ExcDataTable = GetExcTable();
    ExcCode      = GetVecEntry();

    for (int i=0; i<0x20; i++)
    {
        excVector = *((UWORD*)(ExcDataTable+(i*2)));
        if (excVector == 0xffff)
        {
            break;
        }
        switch (excVector)
        {
            case 0x1000:
            case 0x1100:
            case 0x1200:
            {
                *((UWORD*)(excVector)) = 0x4800;
                *((UWORD*)(excVector+2)) = ((excVector>>6)& 0xf) + OFFSET_KERNEL - excVector;
                break;
            }
            default:
            {
                int j;
                for (j=0; j<PPC_VECLEN; j++)
                {
                    *((ULONG*)(excVector+(j*4))) = *((ULONG*)(ExcCode+(j*4)));
                }
                *((UWORD*)(excVector-2+(j*4))) = 17 + OFFSET_KERNEL - excVector - (PPC_VECLEN<<2);
                break;
            }
        }
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

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

/********************************************************************************************
*
*
*
*********************************************************************************************/

void initSema(struct PrivatePPCBase* PowerPCBase, struct SignalSemaphorePPC* SemaphorePPC)
{
    NewListPPC((struct List*)&SemaphorePPC->ssppc_SS.ss_WaitQueue);
    struct LibSema* libSema = (struct LibSema*)SemaphorePPC;
	SemaphorePPC->ssppc_SS.ss_Owner = 0;
	SemaphorePPC->ssppc_SS.ss_NestCount = 0;
	SemaphorePPC->ssppc_SS.ss_QueueCount = -1;
    SemaphorePPC->ssppc_reserved = (APTR)&libSema->ls_Reserved;
                                             
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

void setupCaches(struct PrivatePPCBase* PowerPCBase)
{
    ULONG value0 = getHID0();
    value0 |= HID0_ICE | HID0_DCE | HID0_SGE | HID0_BTIC | HID0_BHTE;
    setHID0(value0);

    ULONG value1 = getHID1();

    ULONG pll;

    switch (PowerPCBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            break;
        }
        case DEVICE_MPC8343E:
        {
            PowerPCBase->pp_L2Size = 0;
            PowerPCBase->pp_CurrentL2Size = 0;
            pll = (value1 >> 25) & 0x7f;
            switch (pll)
            {
                case 0x23:
                {
                    PowerPCBase->pp_CPUSpeed = 400000000;
                    break;
                }
                case 0x25:
                {
                    PowerPCBase->pp_CPUSpeed = 333333333;
                    break;
                }
            }
        }
        case DEVICE_MPC107:
        {
            break;
        }
    }

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

__section (".setupppc","acrx") __interrupt void setupPPC(struct InitData* initData)
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
    }

    ULONG* IdleTask = (ULONG*)OFFSET_SYSMEM;
    IdleTask[0] = OPCODE_NOP;
    IdleTask[1] = OPCODE_NOP;
    IdleTask[2] = OPCODE_BRANCH - 12;

    installExceptions();

    mmuSetup(initData);

    while (initData->id_Status != 0x496e6974);

    initData->id_Status = 0x426f6f6e;   //Boon

    while (myZP->zp_Status != STATUS_INIT);

    struct PrivatePPCBase* PowerPCBase = (struct PrivatePPCBase*)myZP->zp_PowerPCBase;

    NewListPPC((struct List*)&PowerPCBase->pp_WaitingTasks);
    NewListPPC((struct List*)&PowerPCBase->pp_AllTasks);
    NewListPPC((struct List*)&PowerPCBase->pp_Snoop);
    NewListPPC((struct List*)&PowerPCBase->pp_Semaphores);
    NewListPPC((struct List*)&PowerPCBase->pp_RemovedExc);
    NewListPPC((struct List*)&PowerPCBase->pp_ReadyExc);
    NewListPPC((struct List*)&PowerPCBase->pp_InstalledExc);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcInterrupt);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcIABR);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcPerfMon);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcTrace);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcSystemCall);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcDecrementer);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcFPUn);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcProgram);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcAlign);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcIAccess);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcDAccess);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcMCheck);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcSysMan);
    NewListPPC((struct List*)&PowerPCBase->pp_ExcTherMan);
    NewListPPC((struct List*)&PowerPCBase->pp_WaitTime);
    NewListPPC((struct List*)&PowerPCBase->pp_Ports);
    NewListPPC((struct List*)&PowerPCBase->pp_NewTasks);
    NewListPPC((struct List*)&PowerPCBase->pp_ReadyTasks);
    NewListPPC((struct List*)&PowerPCBase->pp_RemovedTasks);
    NewListPPC((struct List*)&PowerPCBase->pp_MsgQueue);

    initSema(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemWaitList);
    initSema(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemTaskList);
    initSema(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSemList);
    initSema(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemPortList);
    initSema(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemSnoopList);
    initSema(PowerPCBase, (struct SignalSemaphorePPC*)&PowerPCBase->pp_SemMemory);

    PowerPCBase->pp_IdUsrTasks = 100;
    PowerPCBase->pp_EnAltivec = 1;
    PowerPCBase->pp_BusyCounter = 24;
    PowerPCBase->pp_LowActivityPri = 6000;
    PowerPCBase->pp_EnAlignExc = initData->id_Environment1 >> 8;
    PowerPCBase->pp_EnDAccessExc = initData->id_Environment2 >> 8;
    PowerPCBase->pp_CacheDisDFlushAll = initData->id_Environment2 >> 24;
    PowerPCBase->pp_DebugLevel = initData->id_Environment1 >> 16;
    PowerPCBase->pp_CPUInfo = getPVR();
    PowerPCBase->pp_PageTableSize = myZP->zp_PageTableSize;

    ULONG quantum = 0;
    ULONG busclock = 0;

    switch (PowerPCBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            break;
        }
        case DEVICE_MPC8343E:
        {
            quantum = KILLERQUANTUM;
            busclock = KILLERBUSCLOCK;

            if (loadPCI(IMMR_ADDR_DEFAULT, IMMR_RCWLR) & (1<<30))
            {
                quantum = quantum >> 1;
                busclock = busclock >> 1;
            }
            break;
        }
        case DEVICE_MPC107:
        {
            break;
        }
    }

    PowerPCBase->pp_StdQuantum = quantum;
    PowerPCBase->pp_BusClock = busclock;

    setupCaches(PowerPCBase);

    myZP->zp_Status = STATUS_READY;

    setDEC(PowerPCBase->pp_StdQuantum);
    setSRR0((ULONG)(PowerPCBase->pp_PPCMemBase) + OFFSET_SYSMEM);
    setSRR1(MACHINESTATE_DEFAULT);

    while(1); //fake end

    return;
}


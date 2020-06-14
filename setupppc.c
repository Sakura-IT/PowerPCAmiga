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

PPCSETUP BOOL setupTBL(__reg("r3") ULONG startEffAddr, __reg("r4") ULONG endEffAddr, __reg("r5") ULONG physAddr, __reg("r6") ULONG WIMG, __reg("r7") ULONG ppKey)
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

PPCSETUP void setupPT(void)
{
    ULONG ptLoc;
    struct PPCZeroPage *myZP = 0;

    ULONG memSize = myZP->zp_MemSize;
    ULONG ptSize  = myZP->zp_PageTableSize;

    if (ptSize < 0x10000)
    {
        ptSize = 0x10000;
    }

    ULONG myPVR = getPVR();

    if ((myPVR >> 16) == ID_MPC834X)
    {
        ptLoc      = 0x100000;
    }
    else
    {
        ptLoc      = memSize - ptSize;
    }

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

PPCSETUP void mmuSetup(__reg("r3") struct InitData* initData)
{
    ULONG leadZeros, ibatl, ibatu, dbatl, dbatu, batSize;
    ULONG startEffAddr, endEffAddr, physAddr, WIMG, ppKey;
    struct PPCZeroPage *myZP = 0;
    ULONG PTSizeShift        = (ULONG)(myZP->zp_MemSize);
    ULONG shiftVal           = 23;

    leadZeros = getLeadZ(PTSizeShift);

    PTSizeShift = shiftVal - leadZeros;

    myZP->zp_PageTableSize = (1<<PTSizeShift);

    setupPT();

    switch (initData->id_DeviceID)
    {
        case DEVICE_MPC8343E:
        {
            startEffAddr = IMMR_ADDR_DEFAULT;
            break;
        }
        case DEVICE_HARRIER:
        {
            startEffAddr = initData->id_MPICBase;
            endEffAddr   = startEffAddr + 0x400000;
            physAddr     = startEffAddr;
            WIMG         = PTE_CACHE_INHIBITED|PTE_GUARDED;
            ppKey        = PP_SUPERVISOR_RW;

            if (!(setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey)))
            {
                initData->id_Status = ERR_PPCMMU;
                return;
            }

            startEffAddr = PPC_XCSR_BASE;
            break;
        }
        case DEVICE_MPC107:
        {
            startEffAddr = PPC_EUMB_BASE;
            break;
        }
    }

    endEffAddr   = startEffAddr + 0x100000;
    physAddr     = startEffAddr;
    WIMG         = PTE_CACHE_INHIBITED|PTE_GUARDED;
    ppKey        = PP_USER_RW;

    if (!(setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey)))
    {
        initData->id_Status = ERR_PPCMMU;
        return;
    }

    startEffAddr = VECTOR_TABLE_DEFAULT;
    endEffAddr   = startEffAddr + 0x20000;
    physAddr     = startEffAddr;
    WIMG         = PTE_CACHE_INHIBITED;
    ppKey        = PP_USER_RW;

    if (!(setupTBL(startEffAddr, endEffAddr, physAddr, WIMG, ppKey)))
    {
        initData->id_Status = ERR_PPCMMU;
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
                initData->id_Status = ERR_PPCMMU;
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
                initData->id_Status = ERR_PPCMMU;
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
        initData->id_Status = ERR_PPCMMU;
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

PPCSETUP void installExceptions(void)
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
            case VEC_ITLBMISS:
            case VEC_DLOADTLBMISS:
            case VEC_DSTORETLBMISS :
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

PPCSETUP void setupFIFOs(__reg("r3") struct InitData* initData)
{
    ULONG memIF, memIP, memOF, memOP;
    ULONG memBase  = initData->id_MemBase;
    ULONG memFIFO  = memBase + BASE_KMSG;
    ULONG memFIFO2 = memFIFO + SIZE_KBASE;

    switch (initData->id_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            memIF    = XCSR_MIQB_DEFAULT;
            memIP    = memIF + SIZE_HFIFO;
            memOF    = memIP + SIZE_HFIFO;
            memOP    = memOF + SIZE_HFIFO;

            writememLongPPC(PPC_XCSR_BASE, XCSR_MIQB, XCSR_MIQB_DEFAULT);
            writememLongPPC(PPC_XCSR_BASE, XCSR_MIIFT, 4);
            writememLongPPC(PPC_XCSR_BASE, XCSR_MIIFH, 0);
            writememLongPPC(PPC_XCSR_BASE, XCSR_MIIPT, SIZE_HFIFO);
            writememLongPPC(PPC_XCSR_BASE, XCSR_MIIPH, SIZE_HFIFO);
            writememLongPPC(PPC_XCSR_BASE, XCSR_MIOFT, (SIZE_HFIFO * 2) + 4);
            writememLongPPC(PPC_XCSR_BASE, XCSR_MIOFH, (SIZE_HFIFO * 2));
            writememLongPPC(PPC_XCSR_BASE, XCSR_MIOPT, (SIZE_HFIFO * 3));
            writememLongPPC(PPC_XCSR_BASE, XCSR_MIOPH, (SIZE_HFIFO * 3));
            break;
        }
        case DEVICE_MPC8343E:
        {
            memIF    = FIFO_OFFSET;
            memIP    = memIF + SIZE_KFIFO;
            memOF    = memIP + SIZE_KFIFO;
            memOP    = memOF + SIZE_KFIFO;

            struct killFIFO* baseFIFO = (struct killFIFO*)(memIF + (SIZE_KFIFO *4));

            baseFIFO->kf_MIIFT = memBase + memIF + 4;
            baseFIFO->kf_MIIFH = memBase + memIF;
            baseFIFO->kf_MIIPT = memBase + memIP;
            baseFIFO->kf_MIIPH = memBase + memIP;
            baseFIFO->kf_MIOFH = memBase + memOF;
            baseFIFO->kf_MIOFT = memBase + memOF + 4;
            baseFIFO->kf_MIOPT = memBase + memOP;
            baseFIFO->kf_MIOPH = memBase + memOP;
            break;
        }
        case DEVICE_MPC107:
        {
            memIF    = MPC107_QBAR_DEFAULT;
            memIP    = memIF + SIZE_SFIFO;
            memOP    = memIP + SIZE_SFIFO;
            memOF    = memOP + SIZE_SFIFO;

            storePCI(PPC_EUMB_BASE, MPC107_QBAR, MPC107_QBAR_DEFAULT);
            storePCI(PPC_EUMB_BASE, MPC107_IFTPR, 4);
            storePCI(PPC_EUMB_BASE, MPC107_IFHPR, 0);
            storePCI(PPC_EUMB_BASE, MPC107_IPTPR, SIZE_SFIFO);
            storePCI(PPC_EUMB_BASE, MPC107_IPHPR, SIZE_SFIFO);
            storePCI(PPC_EUMB_BASE, MPC107_OPTPR, SIZE_SFIFO * 2);
            storePCI(PPC_EUMB_BASE, MPC107_OPHPR, SIZE_SFIFO * 2);
            storePCI(PPC_EUMB_BASE, MPC107_OFTPR, (SIZE_SFIFO * 3) + 4);
            storePCI(PPC_EUMB_BASE, MPC107_OFHPR, SIZE_SFIFO * 3);

            break;
        }
    }

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
        memFIFO  += MSGLEN;
        memFIFO2 += MSGLEN;
    }

    switch (initData->id_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            writememLongPPC(PPC_XCSR_BASE, XCSR_MICT, (XCSR_MICT_ENA | XCSR_MICT_QSZ_16K));
            break;
        }
        case DEVICE_MPC107:
        {
            storePCI(PPC_EUMB_BASE, MPC107_MUCR, (MPC107_MUCR_CQS_FIFO4K | MPC107_MUCR_CQE_ENABLE));
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

void setupOpenPIC(__reg("r3") struct InitData* initData)
{
    ULONG volatile value = readmemLongPPC(initData->id_MPICBase, XMPI_GLBC);
    writememLongPPC(initData->id_MPICBase, XMPI_GLBC, value | XMPI_GLBC_RESET);

    while (1)
    {
        value = readmemLongPPC(initData->id_MPICBase, XMPI_GLBC);  //different in asm version
        if (!(value & XMPI_GLBC_RESET))
        {
            writememLongPPC(initData->id_MPICBase, XMPI_GLBC, value | XMPI_GLBC_M);
            break;
        }
    }

    writememLongPPC(initData->id_MPICBase, XMPI_IFEVP, 0x00050042);
    writememLongPPC(initData->id_MPICBase, XMPI_IFEDE, XMPI_IFEDE_P0);
    writememLongPPC(initData->id_MPICBase, XMPI_P0CTP, 0);

    value = readmemLongPPC(initData->id_MPICBase, XMPI_FREP);

    while (value)
    {
        readmemLongPPC(initData->id_MPICBase, XMPI_P0IAC);
        writememLongPPC(initData->id_MPICBase, XMPI_P0EOI, 0);
        value--;
    }

    writememLongPPC(PPC_XCSR_BASE, XCSR_FEEN, XCSR_FEEN_MIP | XCSR_FEEN_MIM0);
    writememLongPPC(PPC_XCSR_BASE, XCSR_FEMA, XCSR_FEMA_MIPM0);
    writememLongPPC(PPC_XCSR_BASE, XCSR_MCSR, XCSR_MCSR_OPI);

    return;
}
/********************************************************************************************
*
*
*
*********************************************************************************************/

void setupEPIC(void)
{
    writememLongPPC(PPC_EUMB_BASE, EPIC_GCR, EPIC_GCR_RESET);
    while ((readmemLongPPC(PPC_EUMB_BASE, EPIC_GCR) & EPIC_GCR_RSTATUS));
    writememLongPPC(PPC_EUMB_BASE, EPIC_GCR, EPIC_GCR_MIXMODE);

    storePCI(PPC_EUMB_BASE, EPIC_IIVPR3, 0x80050042);

    ULONG value = readmemLongPPC(PPC_EUMB_BASE, EPIC_EICR);
    writememLongPPC(PPC_EUMB_BASE, EPIC_EICR, (value & ~(1<<11)));   //SIE = 0

    value = readmemLongPPC(PPC_EUMB_BASE, EPIC_IIVPR3);

    writememLongPPC(PPC_EUMB_BASE, EPIC_IIVPR3, (value & ~(1<<7))); //Mask M bit

    writememLongPPC(PPC_EUMB_BASE, EPIC_PCTPR, 0);

    value = (loadPCI(PPC_EUMB_BASE, EPIC_FRR) >> 16 ) & 0x7ff;

    while (value)
    {
        ULONG value2 = readmemLongPPC(PPC_EUMB_EPICPROC, EPIC_IACK);
        sync();
        writememLongPPC(PPC_EUMB_EPICPROC, EPIC_EOI, 0);
        value--;
    }

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCSETUP void initSema(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct SignalSemaphorePPC* SemaphorePPC)
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

PPCSETUP void setupCaches(__reg("r3") struct PrivatePPCBase* PowerPCBase, __reg("r4") struct InitData* initData)
{
    ULONG value0 = getHID0();
    ULONG value1 = getHID1();
    ULONG num = 0;
    ULONG pll, l2Size, l2Setting;
    struct CacheSize cz;

    switch (PowerPCBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            value0 |= HID0_ICE | HID0_DCE | HID0_SGE | HID0_BTIC | HID0_BHTE;
            setHID0(value0);

            if (getPVR() >> 24 == 0x70)
            {
                l2Size = L2_SIZE_HM;

                if (!(getL2CR() & L2CR_L2E))
                {
                    setL2CR(L2CR_L2I);
                    while (getL2CR() & L2CR_L2IP);
                    setL2CR(L2CR_L2E);
                }

                pll = value1 >> 27;

                ULONG* pllTable = getTableFX();

                while (pllTable[num])
                {
                    if (pllTable[num] == pll)
                    {
                        PowerPCBase->pp_CPUSpeed = pllTable[num+1];
                        break;
                    }
                    num +=2;
                }
            }
            else
            {
                l2Setting = L2CR_L2SIZ_1M | L2CR_L2CLK_3 | L2CR_L2RAM_BURST | L2CR_TS;

                if (!(getL2CR() & L2CR_L2E))
                {
                    setL2CR(l2Setting | L2CR_L2I);
                    while (getL2CR() & L2CR_L2IP);
                    setL2CR(l2Setting | L2CR_L2E);
                }

                l2Setting |= L2CR_L2E;

                setL2CR(l2Setting);
                getL2Size(PowerPCBase->pp_PPCMemBase + PowerPCBase->pp_PPCMemSize, (APTR)&cz);

                l2Setting &= ~L2CR_TS;
                if (cz.cz_SizeBit)
                {
                    l2Setting &= ~L2CR_L2SIZ_1M;
                    l2Setting |= cz.cz_SizeBit;
                }

                setL2CR(l2Setting);
                l2Size = cz.cz_SizeBytes;

                pll = value1 >> 28;

                ULONG* pllTable = getTable100();

                while (pllTable[num])
                {
                    if (pllTable[num] == pll)
                    {
                        PowerPCBase->pp_CPUSpeed = pllTable[num+1];
                        break;
                    }
                    num +=2;
                }
            }
            break;
        }
        case DEVICE_MPC8343E:
        {
            value0 |= HID0_ICE | HID0_DCE;
            setHID0(value0);
            l2Size = 0;
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
            value0 |= HID0_ICE | HID0_DCE | HID0_SGE | HID0_BTIC | HID0_BHTE;
            setHID0(value0);

            l2Setting = L2CR_L2SIZ_1M | L2CR_L2CLK_3 | L2CR_L2RAM_BURST | L2CR_TS;

            if (!(getL2CR() & L2CR_L2E))
            {
                setL2CR(l2Setting | L2CR_L2I);
                while (getL2CR() & L2CR_L2IP);
                setL2CR(l2Setting | L2CR_L2E);
            }

            l2Setting |= L2CR_L2E;

            setL2CR(l2Setting);
            getL2Size(PowerPCBase->pp_PPCMemBase + PowerPCBase->pp_PPCMemSize, (APTR)&cz);

            l2Setting &= ~L2CR_TS;
            if (cz.cz_SizeBit)
            {
                l2Setting &= ~L2CR_L2SIZ_1M;
                l2Setting |= cz.cz_SizeBit;
            }

            setL2CR(l2Setting);
            l2Size = cz.cz_SizeBytes;

            ULONG* pllTable;
            pll = value1 >> 28;

            if (initData->id_ConfigBase == 0x13)
            {
                pllTable = getTable100();
            }
            else
            {
                pllTable = getTable66();
            }

            while (pllTable[num])
            {
                if (pllTable[num] == pll)
                {
                    PowerPCBase->pp_CPUSpeed = pllTable[num+1];
                    break;
                }
                num +=2;
            }
            break;
        }
    }

    PowerPCBase->pp_L2Size = l2Size << 8;
    PowerPCBase->pp_CurrentL2Size = l2Size << 8;

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCSETUP void setupPPC(__reg("r3") struct InitData* initData)
{
    
    ULONG mem = 0;
    ULONG copySrc = 0;
    struct PPCZeroPage *myZP = 0;

    if (initData->id_DeviceID == DEVICE_MPC107)
    {
        struct MemSettings ms;

        writePCI(MPC107_PICR1, MPC107_PICR1_DEFAULT);
        writePCI(MPC107_PICR2, MPC107_PICR2_DEFAULT);
        writePCI16(MPC107_PMCR1, MPC107_PMCR1_DEFAULT);
        writePCI(MPC107_EUMBBAR, PPC_EUMB_BASE);

        if (initData->id_ConfigBase == 0x13)
        {
            writePCI(MPC107_MCCR4, 0x35303232);
            writePCI(MPC107_MCCR3, 0x78400000);
            writePCI(MPC107_MCCR2, 0x04400700);

            ms.ms_mccr1testhigh = 0x75880000;
            ms.ms_mccr1testlow1 = 0xaaaa;
            ms.ms_mccr1testlow2 = 0x5555;
            ms.ms_mccr1testlow3 = 0;
        }
        else
        {
            writePCI(MPC107_MCCR4, 0x00100000);
            writePCI(MPC107_MCCR3, 0x0002a29c);
            writePCI(MPC107_MCCR2, 0xe0001040); //EDO?
            writePCI(MPC107_MCCR1, 0xffe20000);


            ms.ms_mccr1testhigh = 0xffea0000;
            ms.ms_mccr1testlow1 = 0xffff;
            ms.ms_mccr1testlow2 = 0xaaaa;
            ms.ms_mccr1testlow3 = 0x5555;
        }

        writePCI(MPC107_MSAR1, 0);
        writePCI(MPC107_MESAR1, 0);
        writePCI(MPC107_MSAR2, 0);
        writePCI(MPC107_MESAR2, 0);
        writePCI(MPC107_MEAR1, -1);
        writePCI(MPC107_MEEAR1, 0);
        writePCI(MPC107_MEAR2, -1);
        writePCI(MPC107_MEEAR2, 0);
        writePCI(MPC107_MCCR1, ms.ms_mccr1testhigh);

        ms.ms_msar1 = 0;
        ms.ms_msar2 = 0;
        ms.ms_mear1 = 0;
        ms.ms_mear2 = 0;
        ms.ms_mesar1 = 0;
        ms.ms_mesar2 = 0;
        ms.ms_meear1 = 0;
        ms.ms_meear2 = 0;
        ms.ms_mben = 0;
        ms.ms_mccr1 = 0;

        detectMem((struct MemSettings*)&ms);

        writePCI(MPC107_MSAR1, ms.ms_msar1);
        writePCI(MPC107_MESAR1, ms.ms_mesar1);
        writePCI(MPC107_MSAR2, ms.ms_msar2);
        writePCI(MPC107_MESAR2, ms.ms_mesar2);
        writePCI(MPC107_MEAR1, ms.ms_mear1);
        writePCI(MPC107_MEEAR1, ms.ms_meear1);
        writePCI(MPC107_MEAR2, ms.ms_mear2);
        writePCI(MPC107_MEEAR2, ms.ms_meear2);
        writePCI8(MPC107_PGMAX, 0x32);
        writePCI8(MPC107_MBEN, ms.ms_mben);

        sync();

        writePCI(MPC107_MCCR1, ms.ms_mccr1);

        ULONG settle = 0x2ffff;

        while (settle)
        {
            settle--;
        }

        initData->id_MemSize = ms.ms_memsize;
    }

    initData->id_Status = STATUS_INIT;

    Reset();

    for (LONG i=0; i<0x40; i++)
    {
        *((ULONG*)(mem)) = 0;
        mem += 4;
    }

    if (initData->id_DeviceID == DEVICE_MPC107)
    {
        dInval(0);
        while (initData->id_Status != STATUS_MEM);
        initData->id_Status = STATUS_INIT;
    }

    myZP->zp_MemSize = initData->id_MemSize;

    setupFIFOs(initData);

    switch (initData->id_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            setupOpenPIC(initData);
            break;
        }
        case DEVICE_MPC8343E:
        {
            break;
        }
        case DEVICE_MPC107:
        {
            setupEPIC();
            break;
        }
    }

    ULONG* IdleTask = (ULONG*)OFFSET_SYSMEM;
    IdleTask[0] = OPCODE_NOP;
    IdleTask[1] = OPCODE_NOP;
    IdleTask[2] = OPCODE_BBRANCH - 8;

    installExceptions();

    mmuSetup(initData);

    while (initData->id_Status != STATUS_INIT);

    initData->id_Status = ERR_PPCOK;

    while (myZP->zp_Status != STATUS_INIT)
    {
        dInval(0);
    }

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

    PowerPCBase->pp_PPCMemSize = initData->id_MemSize;
    PowerPCBase->pp_IdUsrTasks = 100;
    PowerPCBase->pp_BusyCounter = 24;
    PowerPCBase->pp_LowActivityPri = 6000;
    PowerPCBase->pp_EnAlignExc = initData->id_Environment1 >> 8;
    PowerPCBase->pp_EnDAccessExc = initData->id_Environment2 >> 8;
    PowerPCBase->pp_CacheDisDFlushAll = initData->id_Environment2 >> 24;
    PowerPCBase->pp_DebugLevel = initData->id_Environment1 >> 16;
    PowerPCBase->pp_CPUInfo = getPVR();
    PowerPCBase->pp_PageTableSize = myZP->zp_PageTableSize;

    ULONG quantum = QUANTUM_100;
    ULONG busclock = BUSCLOCK_100;

    switch (PowerPCBase->pp_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            if ((PowerPCBase->pp_CPUInfo >> 16 & 0xfff) == 0xc)
            {
                PowerPCBase->pp_EnAltivec = -1;
            }
            break;
        }
        case DEVICE_MPC8343E:
        {
            quantum = QUANTUM_KILLER;
            busclock = BUSCLOCK_KILLER;

            if (loadPCI(IMMR_ADDR_DEFAULT, IMMR_RCWLR) & (1<<RCWLR_DDRCM))
            {
                quantum = quantum >> 1;
                busclock = busclock >> 1;
            }
            break;
        }
        case DEVICE_MPC107:
        {
            if (initData->id_ConfigBase != 0x13)
            {
                quantum = QUANTUM_66;
                busclock = BUSCLOCK_66;
            }
            if ((PowerPCBase->pp_CPUInfo >> 16 & 0xfff) == 0xc)
            {
                PowerPCBase->pp_EnAltivec = -1;
            }
            break;
        }
    }

    PowerPCBase->pp_LowerLimit = *((ULONG*)(((ULONG)PowerPCBase + _LVOInsertPPC + 2)));
    PowerPCBase->pp_UpperLimit = *((ULONG*)(((ULONG)PowerPCBase + _LVOFindNamePPC + 2)));
    PowerPCBase->pp_StdQuantum = quantum;
    PowerPCBase->pp_BusClock = busclock;

    mvfrBAT0(&PowerPCBase->pp_ExceptionBATs[0]);
    mvfrBAT1(&PowerPCBase->pp_ExceptionBATs[1]);
    mvfrBAT2(&PowerPCBase->pp_ExceptionBATs[2]);
    mvfrBAT3(&PowerPCBase->pp_ExceptionBATs[3]);

    mvfrBAT0(&PowerPCBase->pp_SystemBATs[0]);
    mvfrBAT1(&PowerPCBase->pp_SystemBATs[1]);
    mvfrBAT2(&PowerPCBase->pp_SystemBATs[2]);
    mvfrBAT3(&PowerPCBase->pp_SystemBATs[3]);

    for (int i = 0; i < 16; i++)
    {
        PowerPCBase->pp_SystemSegs[i] = getSRIn(i<<28);
    }

    ULONG* cmem = (APTR)(PowerPCBase->pp_PPCMemBase + MEM_GAP - 0xC000);
    for (int i = 0; i < 0x1000; i++)
    {
        cmem[i] = 0;
    }

    setupCaches(PowerPCBase, initData);

    myZP->zp_Status = STATUS_READY;

    setDEC(PowerPCBase->pp_StdQuantum);
    setSRR0((ULONG)(PowerPCBase->pp_PPCMemBase) + OFFSET_SYSMEM);
    setSRR1(MACHINESTATE_DEFAULT);

    return;
}


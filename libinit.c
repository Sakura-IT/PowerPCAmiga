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

#include <libraries/mediatorpci.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/mediatorpci.h>
#include <proto/expansion.h>

#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <exec/interrupts.h>

#include <dos/var.h>
#include <dos/dosextens.h>

#include <utility/tagitem.h>
#include <dos/dostags.h>

#include <powerpc/powerpc.h>
#include <powerpc/tasksPPC.h>
#include <intuition/intuition.h>
#include "librev.h"
#include "constants.h"
#include "libstructs.h"
#include "Internalsppc.h"
#include "Internals68k.h"

APTR OldLoadSeg, OldNewLoadSeg, OldAllocMem, OldAddTask, OldRemTask;
struct PPCBase* myPPCBase;

/********************************************************************************************
*
*	Standard dummy entry of a device/library
*
*********************************************************************************************/

int main (void)
{
    return -1;
}

/********************************************************************************************
*
*	Our Resident struct.
*
*********************************************************************************************/

static const struct Resident RomTag =
{
    RTC_MATCHWORD,
    (struct Resident *) &RomTag,
    (struct Resident *) &RomTag+1,
    RTW_NEVER,
    VERSION,
    NT_LIBRARY,
    LIB_PRIORITY,
    LIBNAME,
    VSTRING,
    (APTR)&LibInit
};

/********************************************************************************************
*
*	Supported ATI cards and PPC chipsets
*
*********************************************************************************************/

static const ULONG cardList[] =
{
    DEVICE_MPC107<<16|VENDOR_MOTOROLA,
    DEVICE_HARRIER<<16|VENDOR_MOTOROLA,
    DEVICE_MPC8343E<<16|VENDOR_FREESCALE,
    0
};

static const ULONG atiList[] =
{
    DEVICE_RV280PRO,
    DEVICE_RV280,
    DEVICE_RV280_2,
    DEVICE_RV280MOB,
    DEVICE_RV280SE,
    0
};

/********************************************************************************************
*
*	Exiting and clean-up
*
*********************************************************************************************/

static void CleanUp(struct InternalConsts *myConsts)
{
    struct PciBase *MediatorPCIBase;
    struct DosLibrary *DOSBase;
    struct ExpansionBase *ExpansionBase;
    struct ExecBase *SysBase = myConsts->ic_SysBase;

#if 0
    if (nameSpace)
    {
        FreeVec(nameSpace);
    }
#endif
    if (MediatorPCIBase = myConsts->ic_PciBase)
    {
        CloseLibrary((struct Library*)MediatorPCIBase);
    }

    if (DOSBase = myConsts->ic_DOSBase)
    {
        CloseLibrary((struct Library*)DOSBase);
    }

    if (ExpansionBase = myConsts->ic_ExpansionBase)
    {
        CloseLibrary((struct Library*)ExpansionBase);
    }

    return;
}

/********************************************************************************************
*
*	Setting up our library
*
*********************************************************************************************/

struct PPCBase *mymakeLibrary(struct InternalConsts *myConsts, ULONG funPointer)
{
    ULONG funSize, funOffset;
    UBYTE *baseMem;
    APTR funMem, funAddr;

    struct PPCBase *PowerPCBase = NULL;
    struct ExecBase *SysBase = myConsts->ic_SysBase;

    funSize = *((ULONG*)(funPointer - 4));

    if (!(funMem = myAllocVec32(NULL, funSize, MEMF_PUBLIC|MEMF_PPC|MEMF_REVERSE|MEMF_CLEAR)))
    {
        return NULL;
    }

    funOffset = (ULONG)funMem - (funPointer + 4);

    CopyMem((APTR)(funPointer+4), funMem, funSize);

    if (!(baseMem = (char *)AllocVec((sizeof(struct PrivatePPCBase)) + (NEGSIZEALIGN), MEMF_PUBLIC|MEMF_PPC|MEMF_REVERSE|MEMF_CLEAR)))
    {
        return NULL;
    }

    PowerPCBase = (struct PPCBase*)(baseMem + (NEGSIZEALIGN));

    PowerPCBase->PPC_LibNode.lib_PosSize = sizeof(struct PrivatePPCBase);
    PowerPCBase->PPC_LibNode.lib_NegSize = NEGSIZEALIGN;

    baseMem = (UBYTE*)PowerPCBase;

    for (int n=0; n<(TOTAL_FUNCS); n++)
    {
        funAddr = LibVectors[n];
        if (n > (NUM_OF_68K_FUNCS-1))
        {
            funAddr = (UBYTE*)funAddr + funOffset;
        }
        *((ULONG*)(baseMem - 4)) = (ULONG)funAddr;
        *((UWORD*)(baseMem - 6)) = 0x4ef9;

        baseMem -= 6;
    }

    CacheClearU();

    InitStruct(&LibInitData[0], PowerPCBase, 0);

    PowerPCBase->PPC_LibNode.lib_Node.ln_Name = LIBNAME;        //Initializer of ln_Name does not seem to work with vbcc
    PowerPCBase->PPC_LibNode.lib_IdString = VSTRING;

    AddLibrary((struct Library*)PowerPCBase);

    PowerPCBase->PPC_SysLib = SysBase;

    return PowerPCBase;
}

/********************************************************************************************
*
*	the libinit routine. Called when the library is opened.
*
*********************************************************************************************/

__entry struct PPCBase *LibInit(__reg("d0") struct PPCBase *ppcbase,
              __reg("a0") BPTR seglist, __reg("a6") struct ExecBase* __sys)
{
    struct ExecBase *SysBase;
    struct DosLibrary *DOSBase;
    struct UtilityBase *UtilityBase;
    struct PciBase *MediatorPCIBase;
    struct ExpansionBase *ExpansionBase;
    struct PPCBase *PowerPCBase;
    struct Process *myProc;
    ULONG medflag   = 0;
    ULONG gfxisati  = 0;
    ULONG gfxnotv45 = 0;
    struct ConfigDev *cd = NULL;
    struct PciDevice *ppcdevice = NULL;
    struct PciDevice *gfxdevice = NULL;
    struct MemHeader *pcimemDMAnode;
    ULONG cardNumber = 0;
    BYTE memPrio;
    ULONG card, devfuncnum, res, i, n, testLen;
    ULONG testSize, bytesFree, initPointer, kernelPointer, funPointer;
    volatile ULONG status;
    struct PPCZeroPage *myZeroPage;
    struct MemHeader *myPPCMemHeader;
    struct InitData *cardData;
    APTR nameSpace = NULL;

    struct InternalConsts consts;
    struct InternalConsts *myConsts = &consts;

    SysBase = __sys;

    myConsts->ic_SysBase = __sys;
    myConsts->ic_SegList = seglist;

    if (!(SysBase->AttnFlags & (AFF_68040|AFF_68060)))
    {
        PrintCrtErr(myConsts, "This library requires a 68LC040 or better");
        return NULL;
    }

#if 1
    if ((FindName(&SysBase->MemList,"ppc memory")) || (FindName(&SysBase->LibList,LIBNAME)))
    {
        PrintCrtErr(myConsts, "Other PPC library already active (WarpOS/Sonnet");
        return NULL;
    }
#endif

    if (!(DOSBase = (struct DosLibrary*)OpenLibrary("dos.library",37L)))
    {
        PrintCrtErr(myConsts, "Could not open dos.library V37+");
        return NULL;
    }

    myConsts->ic_DOSBase = DOSBase;

    UtilityBase = (struct UtilityBase*)OpenLibrary("utility.library",0L);

    if (!(MediatorPCIBase = (struct PciBase*)OpenLibrary("pci.library",VERPCI)))
    {
        PrintCrtErr(myConsts, "Could not open pci.library V13.8+");
        return NULL;
    }

    myConsts->ic_PciBase = MediatorPCIBase;

#if 1
    if ((MediatorPCIBase->pb_LibNode.lib_Version == VERPCI) && (MediatorPCIBase->pb_LibNode.lib_Revision < REVPCI))
    {
        PrintCrtErr(myConsts, "Could not open pci.library V13.8+");
        return NULL;
    }
#endif

    if (!(ExpansionBase = (struct ExpansionBase*)OpenLibrary("expansion.library",37L)))
    {
        PrintCrtErr(myConsts, "Could not open expansion.library V37+");;
        return NULL;
    }

    myConsts->ic_ExpansionBase = ExpansionBase;

    if (FindConfigDev(NULL, VENDOR_ELBOX, MEDIATOR_MKII))
    {
        if (!(cd = FindConfigDev(NULL, VENDOR_ELBOX, MEDIATOR_LOGIC)))
        {
            PrintCrtErr(myConsts, "Could not find a supported Mediator board");
            return NULL;
        }
        medflag = 1;
    }

    else if (FindConfigDev(NULL, VENDOR_ELBOX, MEDIATOR_1200TX))
    {
        if (!(FindConfigDev(NULL, VENDOR_ELBOX, MEDIATOR_1200LOGIC)))
        {
            PrintCrtErr(myConsts, "Could not find a supported Mediator board");
            return NULL;
        }
    }

    if (cd)
    {
        if (!(cd->cd_BoardSize == 0x20000000))
        {
            PrintCrtErr(myConsts, "Mediator WindowSize jumper incorrectly configured");
            return NULL;
        }
    }

    for (i=0; i<MAX_PCI_SLOTS; i++)
    {
        devfuncnum = i<<DEVICENUMBER_SHIFT;
        res = ReadConfigurationLong((UWORD)devfuncnum, PCI_OFFSET_ID);

        while (card = cardList[cardNumber])
        {
            if (card == res)
            {
                break;
            }
            cardNumber += 1;
        }
        if (card == res)
        {
            break;
        }
        cardNumber = 0;
    }

    if (!(card))
    {
        PrintCrtErr(myConsts, "No supported PPC PCI bridge detected");
        return NULL;
    }

    ppcdevice = FindPciDevFunc(devfuncnum);
    cardNumber = 0;

    if (!(gfxdevice = FindPciDevice(VENDOR_3DFX, DEVICE_VOODOO45, 0)))
    {
        gfxdevice = FindPciDevice(VENDOR_3DFX, DEVICE_VOODOO3, 0);
        gfxnotv45 = 1;
    }

    if (!(gfxdevice))
    {
        while (atiList[cardNumber])
        {
            if (gfxdevice = FindPciDevice(VENDOR_ATI, atiList[cardNumber], 0))
            {
                gfxisati = 1;
                break;
            }
            if (gfxdevice)
            {
                break;
            }
        cardNumber += 1;
        }
    }

    if (!gfxdevice)
    {
        PrintCrtErr(myConsts, "No supported VGA card detected");
        return NULL;
    }

    if (gfxisati)
    {
        myConsts->ic_gfxMem = gfxdevice->pd_ABaseAddress0;
        myConsts->ic_gfxSize = -((gfxdevice->pd_Size0)&-16L);
        myConsts->ic_gfxConfig = gfxdevice->pd_ABaseAddress2;
        myConsts->ic_gfxType = VENDOR_ATI;
    }
    else
    {
        myConsts->ic_gfxMem = gfxdevice->pd_ABaseAddress2;
        myConsts->ic_gfxSize = -(((gfxdevice->pd_Size2)<<1)&-16L);
        myConsts->ic_gfxConfig = 0;
        myConsts->ic_gfxType = VENDOR_3DFX;
        if (gfxnotv45)
        {
            myConsts->ic_gfxSubType = DEVICE_VOODOO3;
        }
        else
        {
            myConsts->ic_gfxSubType = DEVICE_VOODOO45;
        }
    }

    if ((!(medflag)) && (myConsts->ic_gfxMem >>31))
    {
        PrintCrtErr(myConsts, "PPCPCI environment not set in ENVARC:Mediator");
    }

    getENVs(myConsts);

    //Add ATI Radeon memory as extra memory for K1/M1
    //Need to have Voodoo as primary VGA output
    //As the list has twice the memory name, we search it twice
    //Findname only returns first in the list

    myConsts->ic_startBAT = 0;
    myConsts->ic_sizeBAT = 0;

    for (i=0; i<1; i++)
    {
        if (!(pcimemDMAnode = (struct MemHeader *)FindName(&SysBase->MemList, "pcidma memory")))
        {
            break;
        }
        if (!(pcimemDMAnode->mh_Node.ln_Succ))
        {
            break;
        }

        memPrio = -20;

        if (!(gfxisati))
        {
            testSize = 0x2000000;
            testLen = ((ULONG)pcimemDMAnode->mh_Upper)-((ULONG)pcimemDMAnode->mh_Lower);

            for (n=0; n<4; n++)
            {
                if (testLen <= testSize)
                {
                    break;
                }

                testSize = (testSize<<1);
            }

            if (0 < n < 4)
            {
                memPrio = -15;
                myConsts->ic_startBAT = (((ULONG)pcimemDMAnode->mh_Lower) & (-(testSize)));
                myConsts->ic_sizeBAT = testSize;
            }
        }

        Disable();
        Remove((struct Node *)pcimemDMAnode);
        pcimemDMAnode->mh_Node.ln_Pri = memPrio;
        Enqueue(&SysBase->MemList, (struct Node *)pcimemDMAnode);
        Enable();
    }

    initPointer   = (*((ULONG*)(seglist << 2)) << 2);                   //setup
    kernelPointer = (*((ULONG*)(initPointer)) << 2);                   //kernel
    funPointer    = (*((ULONG*)(kernelPointer)) << 2);                //functions

    switch (ppcdevice->pd_DeviceID)
    {
        case DEVICE_HARRIER:
        {
            cardData = SetupHarrier(myConsts, devfuncnum, ppcdevice, initPointer);
            break;
        }

        case DEVICE_MPC8343E:
        {
            cardData = SetupKiller(myConsts, devfuncnum, ppcdevice, initPointer);
            break;
        }

        case DEVICE_MPC107:
        {
            cardData = SetupMPC107(myConsts, devfuncnum, ppcdevice, initPointer);
            break;
        }

        default:
        {
            PrintCrtErr(myConsts, "Error setting up PPC card");
            return NULL;
        }
    }

    if (!(cardData))
    {
        return NULL;
    }

    for (i=0; i<0xEC0000; i++)
    {
        status = cardData->id_Status;

        switch (status)
        {
            case ERR_PPCOK:
            {
                break;
            }
            case ERR_PPCMMU:
            {
                PrintCrtErr(myConsts, "Error during MMU setup of PPC");
                return NULL;
            }
            case ERR_PPCMEM:
            {
                PrintCrtErr(myConsts, "No memory detected on the PPC card");
                return NULL;
            }
            case ERR_PPCCORRUPT:
            {
                PrintCrtErr(myConsts, "Memory corruption detected during setup");
                return NULL;
            }
            case ERR_PPCSETUP:
            {
                PrintCrtErr(myConsts, "General PPC setup error");
                return NULL;
            }
        }
        if (status == ERR_PPCOK)
        {
            break;
        }
    }

    if (i == 0xEC0000)
    {
        switch (status)
        {
            case STATUS_INIT:
            case ERR_PPCOK:
            {
                PrintCrtErr(myConsts, "PowerPC CPU possibly crashed during setup");
                return NULL;
            }
#if 0
            case 0xabcdabcd:
            {
                PrintCrtErr(myConsts, "PowerPC CPU not responding");
                return NULL;
            }
#endif
            default:
            {
                PrintCrtErr(myConsts, "PowerPC CPU not responding");
                return NULL;
            }
        }
    }

    myZeroPage = (struct PPCZeroPage*)cardData->id_MemBase;
    myConsts->ic_MemBase = (ULONG)myZeroPage;

    nameSpace = AllocVec(16L, MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);

    if (!(nameSpace))
    {
        PrintCrtErr(myConsts, "General memory allocation error");
        return NULL;
    }

    CopyMemQuick("ppc memory", nameSpace, 16L);

    myPPCMemHeader = (struct MemHeader*)((ULONG)myZeroPage + MEM_GAP);
    myPPCMemHeader->mh_Node.ln_Type = NT_MEMORY;
    myPPCMemHeader->mh_Node.ln_Pri = 1;
    myPPCMemHeader->mh_Node.ln_Name = nameSpace;
    myPPCMemHeader->mh_First = (struct MemChunk*)((ULONG)myPPCMemHeader + sizeof(struct MemHeader));

    bytesFree = (cardData->id_MemSize - myZeroPage->zp_PageTableSize - MEM_GAP - sizeof(struct MemHeader));

    myPPCMemHeader->mh_First->mc_Next = NULL;
    myPPCMemHeader->mh_First->mc_Bytes = bytesFree;

    myPPCMemHeader->mh_Free = bytesFree;
    myPPCMemHeader->mh_Upper = (APTR)((ULONG)myZeroPage + (cardData->id_MemSize) - (myZeroPage->zp_PageTableSize));
    myPPCMemHeader->mh_Lower = (APTR)((ULONG)myPPCMemHeader + sizeof(struct MemHeader));
    myPPCMemHeader->mh_Attributes = MEMF_PUBLIC|MEMF_FAST|MEMF_PPC;


    Disable();

    Enqueue(&SysBase->MemList, (struct Node*)myPPCMemHeader);

    PowerPCBase = mymakeLibrary(myConsts, funPointer);

    Enable();

    if (!(PowerPCBase))
    {
        PrintCrtErr(myConsts, "Error during library function setup");
        return NULL;
    }

    PowerPCBase->PPC_DosLib  = (APTR)DOSBase;
    PowerPCBase->PPC_SegList = (APTR)seglist;
    PowerPCBase->PPC_Flags   = (UBYTE)((myConsts->ic_env3 >> 22) | (myConsts->ic_env3 >> 15) | (myConsts->ic_env3 >> 8));

    myPPCBase = PowerPCBase;

    struct PrivatePPCBase* myBase = (struct PrivatePPCBase*)PowerPCBase;

    switch (ppcdevice->pd_DeviceID)
    {
        case DEVICE_HARRIER:
		{
		    break;
		}
        case DEVICE_MPC8343E:
		{
		    myBase->pp_BridgeConfig    = ppcdevice->pd_ABaseAddress0;
            break;
		}
        case DEVICE_MPC107:
		{
		    break;
		}
    }

    myBase->pp_DeviceID                = ppcdevice->pd_DeviceID;
    myBase->pp_MirrorList.mlh_Head     = (struct MinNode*)&myBase->pp_MirrorList.mlh_Tail;
    myBase->pp_MirrorList.mlh_TailPred = (struct MinNode*)&myBase->pp_MirrorList.mlh_Head;
    myBase->pp_UtilityBase             = UtilityBase;
    myBase->pp_PPCMemBase              = (ULONG)myZeroPage;

    myZeroPage->zp_SysBase      = SysBase;
    myZeroPage->zp_PPCMemHeader = myPPCMemHeader;
    myZeroPage->zp_PowerPCBase  = PowerPCBase;
    myZeroPage->zp_PPCMemBase   = (ULONG)myZeroPage;

    struct Library* WarpBase = MakeLibrary(&WarpVectors[0], &WarpInitData[0], 0 ,124, 0);

    if (!(WarpBase))
    {
        PrintCrtErr(myConsts, "Could not set up dummy warp.library");
        return NULL;
    }

    WarpBase->lib_Node.ln_Name = "warp.library";        //Initializer of ln_Name does not seem to work with vbcc
    WarpBase->lib_IdString =     "$VER: warp.library 5.1 (22.3.17)\r\n";

    AddLibrary(WarpBase);

    CopyMem((APTR)(kernelPointer + 4), (APTR)(cardData->id_MemBase + OFFSET_KERNEL), *((ULONG*)(kernelPointer - 4)));

    CacheClearU();


    struct Interrupt* myInt = AllocVec(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);
    myInt->is_Code = (APTR)&GortInt;
    myInt->is_Data = NULL;
    myInt->is_Node.ln_Pri = 100;
    myInt->is_Node.ln_Name = "Gort\0";
    myInt->is_Node.ln_Type = NT_INTERRUPT;
    AddInterrupt(ppcdevice, myInt);
    SetInterrupt(ppcdevice);

    if(!(myProc = CreateNewProcTags(
                           NP_Entry, (ULONG)&MasterControl,
                           NP_Name, "MasterControl",
                           NP_Priority, 1,
                           NP_StackSize, 0x20000,
                           TAG_DONE)))
    {
        PrintCrtErr(myConsts, "Error setting up 68K MasterControl process");
        return NULL;
    }

    myProc->pr_Task.tc_UserData = (APTR)myConsts;
    Signal((struct Task*)myProc, SIGBREAKF_CTRL_F);

    for (i=0; i<0xEC0000; i++)
    {
        if (myZeroPage->zp_Status == STATUS_READY)
        {
            break;
        }
    }
    if (i == 0xEC0000)
    {
        PrintCrtErr(myConsts, "PowerPC CPU possibly crashed during setup");
        return NULL;
    }

    Disable();

    if (!(ChangeStackRamLib(0x4000, SysBase)))
    {
        PrintCrtErr(myConsts, "Failure to increase ramlib stack size");
        return NULL;
    }

    OldLoadSeg    = SetFunction((struct Library*)DOSBase, _LVOLoadSeg,    (ULONG (*)())patchLoadSeg);
    OldNewLoadSeg = SetFunction((struct Library*)DOSBase, _LVONewLoadSeg, (ULONG (*)())patchNewLoadSeg);
    OldAddTask    = SetFunction((struct Library*)SysBase, _LVOAddTask,    (ULONG (*)())patchAddTask);
    OldRemTask    = SetFunction((struct Library*)SysBase, _LVORemTask,    (ULONG (*)())patchRemTask);
    OldAllocMem   = SetFunction((struct Library*)SysBase, _LVOAllocMem,   (ULONG (*)())patchAllocMem);

    Enable();

    struct Interrupt* myInt2 = AllocVec(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);

    if (myBase->pp_DeviceID == DEVICE_MPC8343E)
    {
        myInt2->is_Code = (APTR)&ZenInt;
        myInt2->is_Data = NULL;
        myInt2->is_Node.ln_Pri = -50;
        myInt2->is_Node.ln_Name = "Zen\0";
        myInt2->is_Node.ln_Type = NT_INTERRUPT;
        AddIntServer(INTB_VERTB, myInt2);
    }
#if 0
    struct TagItem myTags[] =
    {
        TASKATTR_CODE,   *((ULONG*)(((ULONG)PowerPCBase + _LVOSystemStart + 2))),
        TASKATTR_NAME,   (ULONG)"Kryten",
        TASKATTR_R3,     (ULONG)PowerPCBase,
        TASKATTR_SYSTEM, TRUE,
        TAG_DONE
    };

    if (!(myCreatePPCTask((struct PrivatePPCBase*)PowerPCBase, (struct TagItem*)&myTags)))
    {
        PrintError(SysBase, "Error setting up Kryten PPC process");
        return NULL;
    }
//#if 0
    struct Library* ppcemu;

    if (ppcemu = OpenLibrary("ppc.library", 46L))
    {
        if (ppcemu->lib_Revision < 41)
        {
            PrintError(SysBase, "Phase 5 ppc.library detected. Please remove it");
            return NULL;
        }
    }
#endif

    return PowerPCBase;
}

/********************************************************************************************
*
*	All the different functions of this library
*
*********************************************************************************************/

static const APTR WarpVectors[] =
{
	(APTR)warpOpen,				   //for WarpDT
	(APTR)warpClose,
	(APTR)myReserved,
	(APTR)myReserved,
    (APTR) -1
};

static const APTR LibVectors[] =
{
    (APTR)myOpen,                 //Table of 68k functions
    (APTR)myClose,
    (APTR)myExpunge,
    (APTR)myReserved,
    (APTR)myRunPPC,
	(APTR)myWaitForPPC,
	(APTR)myGetCPU,
	(APTR)myPowerDebugMode,
	(APTR)myAllocVec32,
	(APTR)myFreeVec32,
	(APTR)mySPrintF68K,
	(APTR)myAllocXMsg,
	(APTR)myFreeXMsg,
	(APTR)myPutXMsg,
	(APTR)myGetPPCState,
	(APTR)mySetCache68K,
	(APTR)myCreatePPCTask,
	(APTR)myCausePPCInterrupt,

    (APTR)myReserved,            //Table of reserved 68k functions
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,
    (APTR)myReserved,


    (APTR)myRun68K,              //Table of PPC functions
	(APTR)myWaitFor68K,
	(APTR)mySPrintF,
	(APTR)myRun68KLowLevel,
	(APTR)myAllocVecPPC,
	(APTR)myFreeVecPPC,
	(APTR)myCreateTaskPPC,
	(APTR)myDeleteTaskPPC,
	(APTR)myFindTaskPPC,
	(APTR)myInitSemaphorePPC,
	(APTR)myFreeSemaphorePPC,
	(APTR)myAddSemaphorePPC,
	(APTR)myRemSemaphorePPC,
	(APTR)myObtainSemaphorePPC,
	(APTR)myAttemptSemaphorePPC,
	(APTR)myReleaseSemaphorePPC,
	(APTR)myFindSemaphorePPC,
	(APTR)myInsertPPC,
	(APTR)myAddHeadPPC,
	(APTR)myAddTailPPC,
	(APTR)myRemovePPC,
	(APTR)myRemHeadPPC,
	(APTR)myRemTailPPC,
	(APTR)myEnqueuePPC,
	(APTR)myFindNamePPC,
	(APTR)myFindTagItemPPC,
	(APTR)myGetTagDataPPC,
	(APTR)myNextTagItemPPC,
	(APTR)myAllocSignalPPC,
	(APTR)myFreeSignalPPC,
	(APTR)mySetSignalPPC,
	(APTR)mySignalPPC,
	(APTR)myWaitPPC,
	(APTR)mySetTaskPriPPC,
	(APTR)mySignal68K,
	(APTR)mySetCache,
	(APTR)mySetExcHandler,
	(APTR)myRemExcHandler,
	(APTR)mySuper,
	(APTR)myUser,
	(APTR)mySetHardware,
	(APTR)myModifyFPExc,
	(APTR)myWaitTime,
	(APTR)myChangeStack,
	(APTR)myLockTaskList,
	(APTR)myUnLockTaskList,
	(APTR)mySetExcMMU,
	(APTR)myClearExcMMU,
	(APTR)myChangeMMU,
	(APTR)myGetInfo,
	(APTR)myCreateMsgPortPPC,
	(APTR)myDeleteMsgPortPPC,
	(APTR)myAddPortPPC,
	(APTR)myRemPortPPC,
	(APTR)myFindPortPPC,
	(APTR)myWaitPortPPC,
	(APTR)myPutMsgPPC,
	(APTR)myGetMsgPPC,
	(APTR)myReplyMsgPPC,
	(APTR)myFreeAllMem,
	(APTR)myCopyMemPPC,
	(APTR)myAllocXMsgPPC,
	(APTR)myFreeXMsgPPC,
	(APTR)myPutXMsgPPC,
	(APTR)myGetSysTimePPC,
	(APTR)myAddTimePPC,
	(APTR)mySubTimePPC,
	(APTR)myCmpTimePPC,
	(APTR)mySetReplyPortPPC,
	(APTR)mySnoopTask,
	(APTR)myEndSnoopTask,
	(APTR)myGetHALInfo,
	(APTR)mySetScheduling,
	(APTR)myFindTaskByID,
	(APTR)mySetNiceValue,
	(APTR)myTrySemaphorePPC,
	(APTR)myAllocPrivateMem,
	(APTR)myFreePrivateMem,
	(APTR)myResetPPC,
	(APTR)myNewListPPC,
	(APTR)mySetExceptPPC,
	(APTR)myObtainSemaphoreSharedPPC,
	(APTR)myAttemptSemaphoreSharedPPC,
	(APTR)myProcurePPC,
	(APTR)myVacatePPC,
	(APTR)myCauseInterrupt,
	(APTR)myCreatePoolPPC,
	(APTR)myDeletePoolPPC,
	(APTR)myAllocPooledPPC,
	(APTR)myFreePooledPPC,
	(APTR)myRawDoFmtPPC,
	(APTR)myPutPublicMsgPPC,
	(APTR)myAddUniquePortPPC,
	(APTR)myAddUniqueSemaphorePPC,
	(APTR)myIsExceptionMode,

    (APTR)CreateMsgFramePPC,     //Table of private PPC functions
    (APTR)GetMsgFramePPC,
    (APTR)SendMsgFramePPC,
    (APTR)FreeMsgFramePPC,
    (APTR)SystemStart,
    (APTR)StartTask,             //Should not be jumped to, just a holder for the address
    (APTR)EndTask,

    (APTR) -1
};

/********************************************************************************************
*
*	Reading and handling of environment variables
*
*********************************************************************************************/

ULONG doENV(struct InternalConsts* myConsts, UBYTE* envstring)
{
    LONG res;
    ULONG buffer;
    struct DosLibrary *DOSBase = myConsts->ic_DOSBase;

    res = GetVar((CONST_STRPTR)envstring, (STRPTR)&buffer, sizeof(ULONG), GVF_GLOBAL_ONLY);

    if ((res < 0) || (res > 1))
    {
        buffer = 0;
    }
    else
    {
        buffer = (buffer >> 24) & 0x7;
    }
    return buffer;
}

void getENVs(struct InternalConsts* myConsts)
{
    myConsts->ic_env1 = 0;
    myConsts->ic_env2 = 0;
    myConsts->ic_env3 = 0;

    myConsts->ic_env1 |= (doENV(myConsts,"sonnet/EnEDOMem") << 24);
    myConsts->ic_env1 |= (doENV(myConsts,"sonnet/Debug") << 16);
    myConsts->ic_env1 |= (doENV(myConsts,"sonnet/EnAlignExc") << 8);
    myConsts->ic_env1 |= doENV(myConsts,"sonnet/DisL2Cache");

    myConsts->ic_env2 |= (doENV(myConsts,"sonnet/DisL2Flush") <<24);
    myConsts->ic_env2 |= (doENV(myConsts,"sonnet/EnPageSetup") << 16);
    myConsts->ic_env2 |= (doENV(myConsts,"sonnet/EnDAccessExc") << 8);
    myConsts->ic_env2 |= doENV(myConsts,"sonnet/SetCMemDiv");

    myConsts->ic_env3 |= (doENV(myConsts,"sonnet/DisHunkPatch") <<24);
    myConsts->ic_env3 |= (doENV(myConsts,"sonnet/SetCPUComm") << 16);
    myConsts->ic_env3 |= (doENV(myConsts,"sonnet/EnStackPatch") << 8);

    return;
}

/********************************************************************************************
*
*	Various support routines for the K1/M1 set-up.
*
*********************************************************************************************/

ULONG readmemLong(ULONG Base, ULONG offset)
{
    ULONG res;
    res = *((ULONG*)(Base + offset));
    return res;
}

void resetKiller(struct InternalConsts* myConsts, ULONG configBase, ULONG ppcmemBase)
{
    ULONG res;

    struct ExecBase* SysBase = myConsts->ic_SysBase;

    CacheClearU();

    writememLong(configBase, IMMR_RPR, KILLER_RESET);

    res = readmemLong(configBase, IMMR_RCER);

    writememLong(configBase, IMMR_RCR, res);

    for (res=0; res<0x10000; res++);

    writememLong(ppcmemBase, 0x1f0, 0); //debugdebug??

    res = readmemLong(configBase, IMMR_RSR);

    writememLong(configBase, IMMR_RSR, res);

    return;
}

void writememLong(ULONG Base, ULONG offset, ULONG value)
{
    *((ULONG*)(Base + offset)) = value;
    return;
}

/********************************************************************************************
*
*	Setting up the K1/M1 card
*
*********************************************************************************************/

struct InitData* SetupKiller(struct InternalConsts* myConsts, ULONG devfuncnum,
                             struct PciDevice* ppcdevice, ULONG initPointer)
{
    UWORD res;
    ULONG ppcmemBase, configBase, fakememBase, vgamemBase, winSize, startAddress, segSize;
    struct InitData* killerData;

    struct PciBase* MediatorPCIBase = myConsts->ic_PciBase;
    struct ExecBase* SysBase = myConsts->ic_SysBase;

    res = (ReadConfigurationWord(devfuncnum, PCI_OFFSET_COMMAND)
          | BUSMASTER_ENABLE);

    WriteConfigurationWord(devfuncnum, PCI_OFFSET_COMMAND, res);

    if (!(ppcmemBase = AllocPCIBAR(0xfc, 1, ppcdevice)))
    {
        PrintCrtErr(myConsts, "Could not allocate sufficient PCI memory");
        return FALSE;
    }

    configBase = ppcdevice->pd_ABaseAddress0;

    writememLong(configBase, IMMR_PIBAR0, (ppcmemBase >> 12));

    writememLong(configBase, IMMR_PITAR0, 0);
    writememLong(configBase, IMMR_PIWAR0,
                PIWAR_EN|PIWAR_PF|PIWAR_RTT_SNOOP|
                PIWAR_WTT_SNOOP|PIWAR_IWS_64MB);

    writememLong(configBase, IMMR_POCMR0, 0);
    writememLong(configBase, IMMR_POCMR1, 0);
    writememLong(configBase, IMMR_POCMR2, 0);
    writememLong(configBase, IMMR_POCMR3, 0);
    writememLong(configBase, IMMR_POCMR4, 0);
    writememLong(configBase, IMMR_POCMR5, 0);

    vgamemBase = (myConsts->ic_gfxMem & (~(1<25)));

    startAddress = (MediatorPCIBase->pb_MemAddress);
    writememLong(configBase, IMMR_PCILAWBAR1,
                (startAddress+0x60000000));

    winSize = POCMR_EN|POCMR_CM_128MB;

    if (!(myConsts->ic_gfxMem & (~(1<25))))
    {
        if (myConsts->ic_gfxSize & (~(1<28)))
        {
            winSize = POCMR_EN|POCMR_CM_256MB;
        }
        else if (myConsts->ic_gfxSize & (~(1<26)))
        {
            winSize = POCMR_EN|POCMR_CM_64MB;
        }
    }
    writememLong(configBase, IMMR_PCILAWAR1, (LAWAR_EN|LAWAR_512MB));

    if (myConsts->ic_gfxSize & (~(1<27)))
    {
        if (myConsts->ic_gfxMem & (~(1<27)))
        {
            startAddress += myConsts->ic_gfxSize;
        }
    }
    
    writememLong(configBase, IMMR_POBAR0, (startAddress >> 12));
    writememLong(configBase, IMMR_POTAR0, (vgamemBase >> 12));
    writememLong(configBase, IMMR_POCMR0, winSize);

    if (myConsts->ic_gfxConfig)
    {
        writememLong(configBase, IMMR_POBAR2, ((myConsts->ic_gfxConfig + 0x60000000) >> 12));
        writememLong(configBase, IMMR_POTAR2, (myConsts->ic_gfxConfig >> 12));
        writememLong(configBase, IMMR_POCMR2, (POCMR_EN|POCMR_CM_64KB));
    }

    if (myConsts->ic_sizeBAT)
    {

        winSize = POCMR_EN|POCMR_CM_256MB;

        if (!(myConsts->ic_sizeBAT & (~(1<28))))
        {
            winSize = POCMR_EN|POCMR_CM_64MB;
            if (!(myConsts->ic_sizeBAT & (~(1<26))))
            {
                winSize = POCMR_EN|POCMR_CM_128MB;
            }
        }

        writememLong(configBase, IMMR_POBAR1, ((myConsts->ic_startBAT + 0x60000000) >> 12));
        writememLong(configBase, IMMR_POTAR1, (myConsts->ic_startBAT >> 12));
        writememLong(configBase, IMMR_POCMR1, winSize);
    }

    writememLong(ppcmemBase, 0xfc, 0x48000000);
    writememLong(ppcmemBase, 0x100, 0x4bfffffc);

    resetKiller(myConsts, configBase, ppcmemBase);

    fakememBase = ppcmemBase + OFFSET_ZEROPAGE;
    killerData = ((struct InitData*)(fakememBase + OFFSET_KERNEL));

    segSize = *((ULONG*)(initPointer - 4));

    CopyMemQuick((APTR)(initPointer+4), (APTR)(killerData), segSize);

    killerData->id_Status        = 0xabcdabcd;
    killerData->id_MemBase       = ppcmemBase;
    killerData->id_MemSize       = 0x4000000;
    killerData->id_GfxMemBase    = myConsts->ic_gfxMem;
    killerData->id_GfxMemSize    = myConsts->ic_gfxSize;
    killerData->id_GfxType       = myConsts->ic_gfxType;
    killerData->id_GfxSubType    = myConsts->ic_gfxSubType;
    killerData->id_GfxConfigBase = myConsts->ic_gfxConfig;
    killerData->id_Environment1  = myConsts->ic_env1;
    killerData->id_Environment2  = myConsts->ic_env2;
    killerData->id_Environment3  = myConsts->ic_env3;
    killerData->id_StartBat      = myConsts->ic_startBAT;
    killerData->id_SizeBat       = myConsts->ic_sizeBAT;

    writememLong(ppcmemBase, 0x100, 0x48012f00);
    writememLong(configBase, IMMR_IMMRBAR, IMMR_ADDR_DEFAULT);

    resetKiller(myConsts, configBase, ppcmemBase);

    writememLong(configBase, IMMR_SIMSR_L, SIMSR_L_MU);

    return killerData;
}

/********************************************************************************************
*
*   Setting up cards based on the Harrier chipset (PowerPlus III).
*
*********************************************************************************************/

struct InitData* SetupHarrier(struct InternalConsts* myConsts, ULONG devfuncnum,
                              struct PciDevice* ppcdevice, ULONG initPointer)
{
    return NULL;
}

/********************************************************************************************
*
*	Setting up cards based on the MPC107 chipset.
*
*********************************************************************************************/

struct InitData* SetupMPC107(struct InternalConsts* myConsts, ULONG devfuncnum,
                             struct PciDevice* ppcdevice, ULONG initPointer)
{
    return NULL;
}

/********************************************************************************************
*
*	Routines for ouputting error messages.
*
*********************************************************************************************/

void PrintCrtErr(struct InternalConsts* myConsts, UBYTE* crterrtext)
{
    PrintError(myConsts->ic_SysBase, crterrtext);
    CleanUp(myConsts);
    return;
}

void PrintError(struct ExecBase* SysBase, UBYTE* errortext)
{
    struct IntuitionBase* IntuitionBase;
    struct EasyStruct MyReq =
        {
        (ULONG)sizeof(struct EasyStruct),
        (ULONG)0,
        (UBYTE*)LIBNAME,
        errortext,
        (UBYTE*)"Continue",
        };

    if (!(IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library",33L)))
    {
        Alert(0x84010000);
        return;
    }

    EasyRequestArgs(NULL, &MyReq, NULL, 0L);

    CloseLibrary((struct Library*)IntuitionBase);

    return;
}


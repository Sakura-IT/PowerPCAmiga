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
#include <proto/powerpc.h>
#include <powerpc/powerpc.h>
#include "constants.h"
#include "libstructs.h"
#include "Internalsppc.h"
#pragma pack(pop)

/********************************************************************************************
*
*	Entry point after kernel.s. This directs to the correct exception code.
*
*********************************************************************************************/

PPCKERNEL void Exception_Entry(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe)
{

    PowerPCBase->pp_ExceptionMode = -1;
    PowerPCBase->pp_Quantum = PowerPCBase->pp_StdQuantum;
    PowerPCBase->pp_CPUSDR1 = getSDR1();
    PowerPCBase->pp_CPUHID0 = getHID0();
    PowerPCBase->pp_L2State = getL2State();

    switch (iframe->if_ExceptionVector)
    {
        case VEC_EXTERNAL:
        {
            switch (PowerPCBase->pp_DeviceID)
	        {
		        case DEVICE_HARRIER:
		        {
			        break;
		        }

		        case DEVICE_MPC8343E:
		        {
                    if (loadPCI(IMMR_ADDR_DEFAULT, IMMR_IMISR) & IMMR_IMISR_IDI)
                    {
                        storePCI(IMMR_ADDR_DEFAULT, IMMR_IDR, 1);
                        CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcInterrupt);
                    }
                    struct MsgFrame* msgFrame;

                    while (msgFrame = libGetMsgFramePPC())
                    {
                        AddTailPPC((struct List*)&PowerPCBase->pp_MsgQueue , (struct Node*)msgFrame);
                    }

                    if (loadPCI(IMMR_ADDR_DEFAULT, IMMR_IMISR) & IMMR_IMISR_IM0I)
                    {
                        storePCI(IMMR_ADDR_DEFAULT, IMMR_IMISR, IMMR_IMISR_IM0I);
                    }
                    break;
		        }

		        case DEVICE_MPC107:
		        {
			        break;
		        }

		        default:
		        {
			        break;
		        }
	        }

            ULONG currAddress = iframe->if_Context.ec_UPC.ec_SRR0;
            if ((PowerPCBase->pp_LowerLimit <= currAddress) && (currAddress < PowerPCBase->pp_UpperLimit))
            {
                PowerPCBase->pp_Quantum = 0x1000;
                break;
            }

            if ((PowerPCBase->pp_Mutex) || (PowerPCBase->pp_FlagForbid))
            {
                PowerPCBase->pp_Quantum = 0x1000;
                break;
            }

            struct TaskPPC* currTask = PowerPCBase->pp_ThisPPCProc;

            if (currTask)
            {
                if (currTask->tp_Task.tc_State == TS_ATOMIC)
                {
                    break;
                }
            }

            HandleMsgs(PowerPCBase);
            TaskCheck(PowerPCBase);
            SwitchPPC(PowerPCBase, iframe);
            break;
        }
        case VEC_DECREMENTER:
        {
            CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcDecrementer);

            struct ExcData* eData;

            while (eData = (struct ExcData*)RemHeadPPC((struct List*)&PowerPCBase->pp_ReadyExc))
            {
                if (eData->ed_ExcID & EXCF_MCHECK)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcMCheck, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_DACCESS)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcDAccess, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_IACCESS)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcIAccess, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_INTERRUPT)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcInterrupt, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_ALIGN)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcAlign, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_PROGRAM)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcProgram, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_FPUN)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcFPUn, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_DECREMENTER)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcDecrementer, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_SC)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcSystemCall, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_TRACE)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcTrace, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_PERFMON)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcPerfMon, (struct Node*)eData);
                if (eData->ed_ExcID & EXCF_IABR)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcIABR, (struct Node*)eData);
                eData->ed_Flags |= EXCF_ACTIVE;
            }

            while (eData = (struct ExcData*)RemHeadPPC((struct List*)&PowerPCBase->pp_RemovedExc))
            {
                ULONG nodeAddr = (ULONG)&eData->ed_LastExc;
                for (int i=0; i<12; i++)
                {
                    nodeAddr += 4;
                    ULONG actNode = *((ULONG*)(nodeAddr));
                    if (actNode)
                    {
                        RemovePPC((struct Node*)actNode);
                    }
                }
                struct ExcData* lastExc = eData->ed_LastExc;
                lastExc->ed_Flags &= ~EXCF_ACTIVE;
            }

            ULONG currAddress = iframe->if_Context.ec_UPC.ec_SRR0;
            if ((PowerPCBase->pp_LowerLimit <= currAddress) && (currAddress < PowerPCBase->pp_UpperLimit))
            {
                PowerPCBase->pp_Quantum = 0x1000;
                break;
            }

            if ((PowerPCBase->pp_Mutex) || (PowerPCBase->pp_FlagForbid))
            {
                PowerPCBase->pp_Quantum = 0x1000;
                break;
            }

            struct TaskPPC* currTask = PowerPCBase->pp_ThisPPCProc;

            if (currTask)
            {
                if (currTask->tp_Task.tc_State == TS_ATOMIC)
                {
                    break;
                }
            }

            HandleMsgs(PowerPCBase);
            TaskCheck(PowerPCBase);
            SwitchPPC(PowerPCBase, iframe);
            break;
        }
        case VEC_DATASTORAGE:
        {
            if (PowerPCBase->pp_EnDAccessExc)  //not standard WOS behaviour
            {
                CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcDAccess);
            }

            if ((iframe->if_Context.ec_DAR == 4) || ((iframe->if_Context.ec_DAR < 0xfffff800) && (iframe->if_Context.ec_DAR > 0x800)))
            {
                if (iframe->if_Context.ec_UPC.ec_SRR0 > 0x10000)
                {
                    if(!(PowerPCBase->pp_DataExcLow += 1))
                    {
                        PowerPCBase->pp_DataExcHigh += 1;
                    }
                    struct DataMsg myData;
                    ULONG Status = DoDataStore(iframe, iframe->if_Context.ec_UPC.ec_SRR0, &myData);
                    if (Status)
                    {
                        struct MsgFrame* myFrame = libCreateMsgFramePPC();
                        myFrame->mf_Identifier = myData.dm_Type;
                        myFrame->mf_Arg[1] = myData.dm_Address;
                        myFrame->mf_Arg[0] = myData.dm_Value;
                        libSendMsgFramePPC(myFrame);
                        if (!(myData.dm_LoadFlag))
                        {
                            while (myFrame->mf_Identifier != ID_DONE);
                            if (!(FinDataStore(myFrame->mf_Arg[0], iframe, iframe->if_Context.ec_UPC.ec_SRR0, &myData)))
                            {
                                CommonExcError(PowerPCBase, iframe);
                                break;
                            }
                        }
                        iframe->if_Context.ec_UPC.ec_SRR0  += 4;
                        break;
                    }
                    else
                    {
                        CommonExcError(PowerPCBase, iframe);
                    }
                    break;
                }
            }
            CommonExcError(PowerPCBase, iframe);
            break;
        }
        case VEC_PROGRAM:
        {
            if (iframe->if_Context.ec_GPR[4] == SUPERKEY)
            {
                iframe->if_Context.ec_UPC.ec_SRR0  += 4;
                iframe->if_Context.ec_SRR1         &= ~PSL_PR;
                iframe->if_Context.ec_GPR[3]        = 0;            //Set SuperKey
            }
            else
            {
                CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcProgram);
            }
            break;
        }
        case VEC_ALIGNMENT:
        {

            if (PowerPCBase->pp_EnAlignExc)
            {
                CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcAlign);
            }
            else
            {
                if (DoAlign(iframe, iframe->if_Context.ec_UPC.ec_SRR0))
                {
                    iframe->if_Context.ec_UPC.ec_SRR0  += 4;
                    if (!(PowerPCBase->pp_AlignmentExcLow += 1))
                    {
                        PowerPCBase->pp_AlignmentExcHigh += 1;
                    }
                }
                else
                {
                    CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcAlign);
                }
            }
            break;
        }
        case VEC_ALTIVECUNAV:
        {
            iframe->if_Context.ec_ExcID = EXCF_ALTIVECUNAV;
            iframe->if_ExcNum = EXCB_ALTIVECUNAV;
            if (PowerPCBase->pp_EnAltivec)
            {
                iframe->if_Context.ec_SRR1 |= PSL_VEC;
            }
            else
            {
                CommonExcError(PowerPCBase, iframe);
            }
            break;
        }
        case VEC_MACHINECHECK:
        {
            CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcMCheck);
            break;
        }
        case VEC_INSTSTORAGE:
        {
            CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcIAccess);
            break;
        }
        case VEC_FPUNAVAILABLE:
        {
            CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcFPUn);
            break;
        }
        case VEC_SYSTEMCALL:
        {
            CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcSystemCall);
            break;
        }
        case VEC_TRACE:
        {
            CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcTrace);
            break;
        }
        case VEC_PERFMONITOR:
        {
            CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcPerfMon);
            break;
        }
        case VEC_IBREAKPOINT:
        {
            CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcIABR);
            break;
        }
        default:
        {
            CommonExcError(PowerPCBase, iframe);
        }
    }
    PowerPCBase->pp_ExceptionMode = 0;
    setDEC(PowerPCBase->pp_Quantum);
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCFUNCTION void TaskCheck(struct PrivatePPCBase* PowerPCBase)
{
    ULONG mask;
    struct TaskPPC* currTask = PowerPCBase->pp_ThisPPCProc;

    if(currTask)
    {
        if ((PowerPCBase->pp_TaskExcept) && (PowerPCBase->pp_TaskExcept == currTask) &&
            (currTask->tp_Task.tc_ExceptData) && (mask = currTask->tp_Task.tc_SigRecvd & currTask->tp_Task.tc_SigExcept))
        {
            currTask->tp_Task.tc_SigRecvd &= ~mask;
            currTask->tp_Task.tc_SigExcept &= ~mask;

            ULONG (*ExcHandler)(__reg("r2") ULONG, __reg("r3") ULONG) = currTask->tp_Task.tc_ExceptCode;
            ULONG tempR2 = getR2();
            ULONG signal = ExcHandler((ULONG)currTask->tp_Task.tc_ExceptData, mask);
            storeR2(tempR2);

            currTask->tp_Task.tc_SigExcept |= signal;
        }
    }

    if (!(PowerPCBase->pp_FlagWait))
    {
        struct WaitTime* currWait = (struct WaitTime*)PowerPCBase->pp_WaitTime.mlh_Head;
        struct WaitTime* nxtWait;

        while (nxtWait = (struct WaitTime*)currWait->wt_Node.ln_Succ)
        {
            if ((currWait->wt_TimeUpper < getTBU()) || ((currWait->wt_TimeUpper == getTBU()) && (currWait->wt_TimeLower < getTBL())))
            {
                if ((currTask) && (currTask == currWait->wt_Task))
                {
                    currWait->wt_Task->tp_Task.tc_State = TS_RUN;
                }
                else
                {
                    currWait->wt_Task->tp_Task.tc_State = TS_READY;
                }
                currWait->wt_Task->tp_Task.tc_SigRecvd |= SIGF_WAIT;
                RemovePPC((struct Node*)currWait);
            }
            currWait = nxtWait;
        }
    }

    struct TaskPPC* currWTask = (struct TaskPPC*)PowerPCBase->pp_WaitingTasks.mlh_Head;
    struct TaskPPC* nxtWTask;

    while (nxtWTask = (struct TaskPPC*)currWTask->tp_Task.tc_Node.ln_Succ)
    {
        if ((currWTask->tp_Task.tc_State == TS_READY) || (currWTask->tp_Task.tc_SigWait & currWTask->tp_Task.tc_SigRecvd))
        {
            currWTask->tp_Task.tc_State = TS_READY;
            RemovePPC((struct Node*)currWTask);
            AddTailPPC((struct List*)&PowerPCBase->pp_ReadyTasks, (struct Node*)currWTask); //Enqueue on pri?
        }
        currWTask = nxtWTask;
    }
    return;
}

/********************************************************************************************
*
*	This function dispatches new PPC tasks, switches between tasks and redirect signals.
*
*********************************************************************************************/

PPCKERNEL void HandleMsgs(struct PrivatePPCBase* PowerPCBase)
{
    struct MsgFrame* currMsg = (struct MsgFrame*)PowerPCBase->pp_MsgQueue.mlh_Head;
    struct MsgFrame* nxtMsg;
    while (nxtMsg = (struct MsgFrame*)currMsg->mf_Message.mn_Node.ln_Succ)
    {
        switch (currMsg->mf_Identifier)
        {
            case ID_TPPC:
            case ID_DONE:
            case ID_END:
            case ID_DNLL:
            {
                struct TaskPPC* myTask = currMsg->mf_PPCTask;
                if (!(myTask))
                {
                    RemovePPC((struct Node*)currMsg);
                    AddTailPPC((struct List*)&PowerPCBase->pp_NewTasks, (struct Node*)currMsg);
                }
                else
                {
                    struct MsgPortPPC* myPort = myTask->tp_Msgport;
                    if (myPort->mp_Semaphore.ssppc_SS.ss_QueueCount == -1)
                    {
                        RemovePPC((struct Node*)currMsg);

                        ULONG signal = 1 << (myPort->mp_Port.mp_SigBit);
                        myTask->tp_Task.tc_SigRecvd |= signal;

                        AddTailPPC(&myPort->mp_Port.mp_MsgList, (struct Node*)currMsg);
                        if (currMsg->mf_Identifier == ID_DONE)
                        {
                            myTask->tp_Task.tc_SigAlloc = currMsg->mf_Arg[1];
                        }
                        if (myTask == PowerPCBase->pp_ThisPPCProc)
                        {
                            myTask->tp_Task.tc_State = TS_RUN;
                        }
                        else
                        {
                            myTask->tp_Task.tc_State = TS_READY;
                        }
                    }
                }
                break;
            }
            case ID_XMSG:
            {
                struct MsgPortPPC* myPort = (struct MsgPortPPC*)currMsg->mf_Message.mn_ReplyPort;
                if (myPort->mp_Semaphore.ssppc_SS.ss_QueueCount != -1)
                {
                    break;
                }

                struct MsgFrame* oldMsg = (struct MsgFrame*)currMsg->mf_Arg[0];

                RemovePPC((struct Node*)currMsg);
                libFreeMsgFramePPC(currMsg);

                myPort = (struct MsgPortPPC*)oldMsg->mf_Message.mn_ReplyPort;
                struct TaskPPC* sigTask = (struct TaskPPC*)myPort->mp_Port.mp_SigTask;

                if (!(sigTask))
                {
                    break;
                }

                ULONG signal = 1 << (myPort->mp_Port.mp_SigBit);
                sigTask->tp_Task.tc_SigRecvd |= signal;
                AddTailPPC(&myPort->mp_Port.mp_MsgList, (struct Node*)oldMsg);

                if (sigTask == PowerPCBase->pp_ThisPPCProc)
                {
                    sigTask->tp_Task.tc_State = TS_RUN;
                }
                else
                {
                    sigTask->tp_Task.tc_State = TS_READY;
                }
                break;
            }
            case ID_XPPC:
            {
                struct MsgPortPPC* myPort = (struct MsgPortPPC*)currMsg->mf_Arg[0];
                if (myPort->mp_Semaphore.ssppc_SS.ss_QueueCount != -1)
                {
                    break;
                }

                struct Node* xMsg = (struct Node*)currMsg->mf_Arg[1];

                RemovePPC((struct Node*)currMsg);
                libFreeMsgFramePPC(currMsg);

                struct TaskPPC* sigTask = (struct TaskPPC*)myPort->mp_Port.mp_SigTask;

                if (!(sigTask))
                {
                    break;
                }

                ULONG signal = 1 << (myPort->mp_Port.mp_SigBit);
                sigTask->tp_Task.tc_SigRecvd |= signal;
                AddTailPPC(&myPort->mp_Port.mp_MsgList, xMsg);

                if (sigTask == PowerPCBase->pp_ThisPPCProc)
                {
                    sigTask->tp_Task.tc_State = TS_RUN;
                }
                else
                {
                    sigTask->tp_Task.tc_State = TS_READY;
                }
                break;
            }
            case ID_LLPP:
            {
                struct PrivateTask* myTask = (struct PrivateTask*)PowerPCBase->pp_ThisPPCProc;
                if (myTask->pt_Mirror68K == (struct Task*)currMsg->mf_Arg[0])
                {
                    myTask->pt_Task.tp_Task.tc_State = TS_RUN;
                    myTask->pt_Task.tp_Task.tc_SigRecvd |= currMsg->mf_Signals;
                }
                else
                {
                    struct PrivateTask* nxtTask;
                    struct PrivateTask* currTask = (struct PrivateTask*)PowerPCBase->pp_WaitingTasks.mlh_Head;
                    while (nxtTask = (struct PrivateTask*)currTask->pt_Task.tp_Task.tc_Node.ln_Succ)
                    {
                        if (currTask->pt_Mirror68K == (struct Task*)currMsg->mf_Arg[0])
                        {
                            currTask->pt_Task.tp_Task.tc_State = TS_READY;
                            currTask->pt_Task.tp_Task.tc_SigRecvd |= currMsg->mf_Signals;
                            break;
                        }
                        currTask = nxtTask;
                    }
                    if (!(nxtTask))
                    {
                        currTask = (struct PrivateTask*)PowerPCBase->pp_ReadyTasks.mlh_Head;
                        while (nxtTask = (struct PrivateTask*)currTask->pt_Task.tp_Task.tc_Node.ln_Succ)
                        {
                            if (currTask->pt_Mirror68K == (struct Task*)currMsg->mf_Arg[0])
                            {
                                currTask->pt_Task.tp_Task.tc_SigRecvd |= currMsg->mf_Signals;
                                break;
                            }
                        }
                        currTask = nxtTask;
                    }
                }
                RemovePPC((struct Node*)currMsg);
                libFreeMsgFramePPC(currMsg);
                break;
            }
            default:
            {
                RemovePPC((struct Node*)currMsg);
                libFreeMsgFramePPC(currMsg);
            }
        }
        currMsg = nxtMsg;
    }
    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCKERNEL void SwitchPPC(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe)
{
    struct TaskPPC* currTask = PowerPCBase->pp_ThisPPCProc;
    while (1)
    {
        if (!(currTask))
        {
            if (currTask = (struct TaskPPC*)RemHeadPPC((struct List*)&PowerPCBase->pp_NewTasks))
            {
                DispatchPPC(PowerPCBase, iframe, (struct MsgFrame*)currTask);
                break;
            }
            if (currTask = (struct TaskPPC*)RemHeadPPC((struct List*)&PowerPCBase->pp_ReadyTasks))
            {
                currTask->tp_Task.tc_State = TS_RUN;
                PowerPCBase->pp_ThisPPCProc = currTask;
                CopyMemPPC((APTR)currTask->tp_ContextMem, (APTR)iframe, sizeof(struct iframe));
                currTask->tp_Task.tc_SPReg = (APTR)iframe->if_Context.ec_GPR[1];
                break;
            }
            iframe->if_Context.ec_SRR1 = MACHINESTATE_DEFAULT;
            iframe->if_Context.ec_UPC.ec_SRR0 = (ULONG)(PowerPCBase->pp_PPCMemBase) + OFFSET_SYSMEM;
            iframe->if_Context.ec_GPR[1] = (ULONG)(PowerPCBase->pp_PPCMemBase) + MEM_GAP - 512;
            break;
        }
        if (currTask->tp_Task.tc_State == TS_REMOVED)
        {
            AddTailPPC((struct List*)&PowerPCBase->pp_RemovedTasks, (struct Node*)currTask);
            currTask = NULL;
            PowerPCBase->pp_ThisPPCProc = currTask;
        }
        else if (currTask->tp_Task.tc_State == TS_CHANGING)
        {
            currTask->tp_Task.tc_State = TS_WAIT;
            CopyMemPPC((APTR)iframe, (APTR)currTask->tp_ContextMem, sizeof(struct iframe));
            AddTailPPC((struct List*)&PowerPCBase->pp_WaitingTasks, (struct Node*)currTask);
            currTask = NULL;
            PowerPCBase->pp_ThisPPCProc = currTask;
        }
        else
        {
            if (currTask = (struct TaskPPC*)RemHeadPPC((struct List*)&PowerPCBase->pp_NewTasks))
            {
                struct TaskPPC* oldTask = PowerPCBase->pp_ThisPPCProc;
                oldTask->tp_Task.tc_State = TS_READY;
                AddTailPPC((struct List*)&PowerPCBase->pp_ReadyTasks, (struct Node*)oldTask);
                CopyMemPPC((APTR)iframe, (APTR)oldTask->tp_ContextMem, sizeof(struct iframe));
                DispatchPPC(PowerPCBase, iframe, (struct MsgFrame*)currTask);
            }
            else if (currTask = (struct TaskPPC*)RemHeadPPC((struct List*)&PowerPCBase->pp_ReadyTasks))
            {
                struct TaskPPC* oldTask = PowerPCBase->pp_ThisPPCProc;
                oldTask->tp_Task.tc_State = TS_READY;
                currTask->tp_Task.tc_State = TS_RUN;
                AddTailPPC((struct List*)&PowerPCBase->pp_ReadyTasks, (struct Node*)oldTask);
                PowerPCBase->pp_ThisPPCProc = currTask;
                CopyMemPPC((APTR)iframe, (APTR)oldTask->tp_ContextMem, sizeof(struct iframe));
                CopyMemPPC((APTR)currTask->tp_ContextMem, (APTR)iframe, sizeof(struct iframe));
                currTask->tp_Task.tc_SPReg = (APTR)iframe->if_Context.ec_GPR[1];
            }
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

PPCKERNEL void DispatchPPC(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe, struct MsgFrame* myFrame)
{
    struct NewTask* newTask = (struct NewTask*)myFrame->mf_Arg[0];

    PowerPCBase->pp_IdUsrTasks += 1;

    newTask->nt_Task.pt_Task.tp_Id = PowerPCBase->pp_IdUsrTasks;
    newTask->nt_Task.pt_Task.tp_PowerPCBase = PowerPCBase;
    newTask->nt_Task.pt_Task.tp_Task.tc_Node.ln_Type = NT_PPCTASK;
    newTask->nt_Task.pt_Task.tp_Task.tc_State = TS_RUN;
    newTask->nt_Task.pt_Task.tp_ContextMem = &newTask->nt_Context;
    newTask->nt_Task.pt_Task.tp_BATStorage = &newTask->nt_BatStore;
    newTask->nt_Task.pt_Task.tp_Link.tl_Task = newTask;
    newTask->nt_Task.pt_Task.tp_Link.tl_Sig = 0xfff;
    newTask->nt_Task.pt_Task.tp_StackMem = (APTR)(myFrame->mf_Arg[0] + 2048);
    newTask->nt_Task.pt_Mirror68K = (struct Task*)myFrame->mf_Arg[2];
    newTask->nt_Task.pt_MirrorPort = myFrame->mf_MirrorPort;
    newTask->nt_Task.pt_Task.tp_Task.tc_Node.ln_Name = (APTR)&newTask->nt_Name;
    newTask->nt_Task.pt_Task.tp_StackSize = (ULONG)myFrame->mf_Message.mn_Node.ln_Name;
    newTask->nt_Task.pt_Task.tp_Task.tc_SPLower = newTask->nt_Task.pt_Task.tp_StackMem;
    newTask->nt_Task.pt_Task.tp_Task.tc_SPUpper = (APTR)((ULONG)newTask->nt_Task.pt_Task.tp_StackMem + newTask->nt_Task.pt_Task.tp_StackSize);
    newTask->nt_Task.pt_Task.tp_Task.tc_SPReg   = (APTR)((ULONG)newTask->nt_Task.pt_Task.tp_Task.tc_SPUpper - 32);

    NewListPPC((struct List*)&newTask->nt_Task.pt_Task.tp_Task.tc_MemEntry);
    newTask->nt_Task.pt_Task.tp_Task.tc_SigAlloc = myFrame->mf_Arg[1];
    PowerPCBase->pp_ThisPPCProc = (struct TaskPPC*)newTask;

    newTask->nt_Task.pt_Task.tp_TaskPtr = &newTask->nt_TaskPtr;
    newTask->nt_TaskPtr.tptr_Task = (struct TaskPPC*)newTask;
    newTask->nt_TaskPtr.tptr_Node.ln_Name = newTask->nt_Task.pt_Task.tp_Task.tc_Node.ln_Name;
    AddTailPPC((struct List*)&PowerPCBase->pp_AllTasks, (struct Node*)&newTask->nt_TaskPtr);

    PowerPCBase->pp_NumAllTasks += 1;

    NewListPPC((struct List*)&newTask->nt_Task.pt_Task.tp_TaskPools);
    NewListPPC((struct List*)&newTask->nt_Port.mp_IntMsg);
    NewListPPC((struct List*)&newTask->nt_Port.mp_Port.mp_MsgList);
    newTask->nt_Port.mp_Port.mp_SigBit = SIGB_DOS;
    NewListPPC((struct List*)&newTask->nt_Port.mp_Semaphore.ssppc_SS.ss_WaitQueue);
    newTask->nt_Port.mp_Semaphore.ssppc_SS.ss_Owner = NULL;
    newTask->nt_Port.mp_Semaphore.ssppc_SS.ss_NestCount = 0;
    newTask->nt_Port.mp_Semaphore.ssppc_SS.ss_QueueCount = -1;
    newTask->nt_Port.mp_Semaphore.ssppc_reserved = &newTask->nt_SSReserved1;
    newTask->nt_Port.mp_Port.mp_SigTask = newTask;
    newTask->nt_Port.mp_Port.mp_Flags = PA_SIGNAL;
    newTask->nt_Port.mp_Port.mp_Node.ln_Type = NT_MSGPORTPPC;
    newTask->nt_Task.pt_Task.tp_Msgport = &newTask->nt_Port;

    struct iframe* newFrame = (struct iframe*)&newTask->nt_Context;

    newFrame->if_Context.ec_SRR1 = MACHINESTATE_DEFAULT;
    newFrame->if_Context.ec_UPC.ec_SRR0 = *((ULONG*)((ULONG)PowerPCBase + 2 + _LVOStartTask));
    newFrame->if_Context.ec_GPR[1] = (ULONG)newTask->nt_Task.pt_Task.tp_Task.tc_SPReg;
    newFrame->if_Context.ec_GPR[3] = (ULONG)PowerPCBase;
    newFrame->if_Context.ec_GPR[4] = (ULONG)myFrame;

    CopyMemPPC(&PowerPCBase->pp_SystemBATs, &newFrame->if_BATs, sizeof(struct BATArray) * 4);
    CopyMemPPC(&PowerPCBase->pp_SystemBATs, newTask->nt_Task.pt_Task.tp_BATStorage, sizeof(struct BATArray) * 4);

    for (int i = 0; i < 16; i++)
    {
        newFrame->if_Segments[i] = PowerPCBase->pp_SystemSegs[i];
    }

    CopyMemPPC((APTR)newFrame, (APTR)iframe, sizeof(struct iframe));

    return;
}

/********************************************************************************************
*
*
*
*********************************************************************************************/

PPCKERNEL void CommonExcHandler(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe, struct List* excList)
{
    struct ExcData* nxtNode;
    struct ExcData* currNode = (struct ExcData*)excList->lh_Head;
    ULONG status = EXCRETURN_NORMAL;

    while (nxtNode = (struct ExcData*)currNode->ed_Node.ln_Succ)
    {
        if (currNode->ed_ExcID & iframe->if_Context.ec_ExcID)
        {
            if ((currNode->ed_Flags & EXCF_GLOBAL) || ((currNode->ed_Flags & EXCF_LOCAL) && (currNode->ed_Task) && (currNode->ed_Task == PowerPCBase->pp_ThisPPCProc)))
            {
                if (currNode->ed_Flags & EXCF_LARGECONTEXT)
                {
                    ULONG (*ExcHandler)(__reg("r2") ULONG, __reg("r3") struct EXCContext*) = currNode->ed_Code;
                    ULONG tempR2 = getR2();
                    status = ExcHandler(currNode->ed_Data, &iframe->if_Context);
                    storeR2(tempR2);
                }
                else if (currNode->ed_Flags & EXCF_SMALLCONTEXT)
                {
                    status = SmallExcHandler(currNode, iframe);
                }
                if (status == EXCRETURN_ABORT)
                {
                    break;
                }
            }
        }
        currNode = nxtNode;
    }

    if ((status == EXCRETURN_NORMAL) && (iframe->if_Context.ec_ExcID != EXCF_DACCESS))
    {
        CommonExcError(PowerPCBase, iframe);
    }
    return;
}

/********************************************************************************************
*
*     Entry point to print an exception error in a window.
*
*********************************************************************************************/

PPCKERNEL void CommonExcError(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe)
{
    if ((iframe->if_Context.ec_ExcID == EXCF_INTERRUPT) || (iframe->if_Context.ec_ExcID == EXCF_DECREMENTER)
     || (iframe->if_Context.ec_ExcID == EXCF_TRACE)     || (iframe->if_Context.ec_ExcID == EXCF_PERFMON))
    {
        return;
    }

    ULONG* errorData = (ULONG*)(PowerPCBase->pp_PPCMemBase + (ULONG)(FIFO_END + 0x100));
    errorData[0]  = (ULONG)PowerPCBase->pp_ThisPPCProc->tp_Task.tc_Node.ln_Name;
    errorData[1]  = (ULONG)PowerPCBase->pp_ThisPPCProc;
    errorData[2]  = iframe->if_ExcNum;
    errorData[3]  = iframe->if_Context.ec_UPC.ec_SRR0;
    errorData[4]  = iframe->if_Context.ec_SRR1;
    errorData[5]  = getMSR();
    errorData[6]  = getHID0();
    errorData[7]  = getPVR();
    errorData[8]  = iframe->if_Context.ec_DAR;
    errorData[9]  = iframe->if_Context.ec_DSISR;
    errorData[10] = getSDR1();
    errorData[11] = getDEC();
    errorData[12] = getTBU();
    errorData[13] = getTBL();
    errorData[14] = iframe->if_Context.ec_XER;
    errorData[15] = iframe->if_Context.ec_CR;
    errorData[16] = iframe->if_Context.ec_FPSCR;
    errorData[17] = iframe->if_Context.ec_LR;
    errorData[18] = iframe->if_Context.ec_CTR;
    errorData[19] = iframe->if_Context.ec_GPR[0];
    errorData[20] = iframe->if_Context.ec_GPR[1];
    errorData[21] = iframe->if_Context.ec_GPR[2];
    errorData[22] = iframe->if_Context.ec_GPR[3];
    errorData[23] = iframe->if_BATs[0].ba_ibatu;;
    errorData[24] = iframe->if_BATs[0].ba_ibatl;
    errorData[25] = iframe->if_Context.ec_GPR[4];
    errorData[26] = iframe->if_Context.ec_GPR[5];
    errorData[27] = iframe->if_Context.ec_GPR[6];
    errorData[28] = iframe->if_Context.ec_GPR[7];
    errorData[29] = iframe->if_BATs[1].ba_ibatu;
    errorData[30] = iframe->if_BATs[1].ba_ibatl;
    errorData[31] = iframe->if_Context.ec_GPR[8];
    errorData[32] = iframe->if_Context.ec_GPR[9];
    errorData[33] = iframe->if_Context.ec_GPR[10];
    errorData[34] = iframe->if_Context.ec_GPR[11];
    errorData[35] = iframe->if_BATs[2].ba_ibatu;
    errorData[36] = iframe->if_BATs[2].ba_ibatl;
    errorData[37] = iframe->if_Context.ec_GPR[12];
    errorData[38] = iframe->if_Context.ec_GPR[13];
    errorData[39] = iframe->if_Context.ec_GPR[14];
    errorData[40] = iframe->if_Context.ec_GPR[15];
    errorData[41] = iframe->if_BATs[3].ba_ibatu;
    errorData[42] = iframe->if_BATs[3].ba_ibatl;
    errorData[43] = iframe->if_Context.ec_GPR[16];
    errorData[44] = iframe->if_Context.ec_GPR[17];
    errorData[45] = iframe->if_Context.ec_GPR[18];
    errorData[46] = iframe->if_Context.ec_GPR[19];
    errorData[47] = iframe->if_BATs[0].ba_dbatu;
    errorData[48] = iframe->if_BATs[0].ba_dbatl;
    errorData[49] = iframe->if_Context.ec_GPR[20];
    errorData[50] = iframe->if_Context.ec_GPR[21];
    errorData[51] = iframe->if_Context.ec_GPR[22];
    errorData[52] = iframe->if_Context.ec_GPR[23];
    errorData[53] = iframe->if_BATs[1].ba_dbatu;
    errorData[54] = iframe->if_BATs[1].ba_dbatl;
    errorData[55] = iframe->if_Context.ec_GPR[24];
    errorData[56] = iframe->if_Context.ec_GPR[25];
    errorData[57] = iframe->if_Context.ec_GPR[26];
    errorData[58] = iframe->if_Context.ec_GPR[27];
    errorData[59] = iframe->if_BATs[2].ba_dbatu;
    errorData[60] = iframe->if_BATs[2].ba_dbatl;
    errorData[61] = iframe->if_Context.ec_GPR[28];
    errorData[62] = iframe->if_Context.ec_GPR[29];
    errorData[63] = iframe->if_Context.ec_GPR[30];
    errorData[64] = iframe->if_Context.ec_GPR[31];
    errorData[65] = iframe->if_BATs[3].ba_dbatu;
    errorData[66] = iframe->if_BATs[3].ba_dbatl;

    struct MsgFrame* myFrame = libCreateMsgFramePPC();
    myFrame->mf_Identifier = ID_CRSH;
    libSendMsgFramePPC(myFrame);
    PowerPCBase->pp_ThisPPCProc->tp_Task.tc_State = TS_REMOVED;
    PowerPCBase->pp_ThisPPCProc->tp_Flags |= TASKPPCF_CRASHED;
    SwitchPPC(PowerPCBase, iframe);
    while(1);
}

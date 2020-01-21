// Copyright (c) 2020 Dennis van der Boon
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

#include "constants.h"
#include "libstructs.h"
#include "Internalsppc.h"
#include <proto/powerpc.h>
#include <powerpc/powerpc.h>

/********************************************************************************************
*
*	Entry point after kernel.s. This directs to the correct exception code.
*
*********************************************************************************************/

PPCKERNEL void Exception_Entry(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe)
{
    PowerPCBase->pp_ExceptionMode = -1;
    PowerPCBase->pp_Quantum = PowerPCBase->pp_StdQuantum;

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
                    storePCI(IMMR_ADDR_DEFAULT, IMMR_IMISR, IMMR_IMISR_IM0I);
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
            HandleMsgs(PowerPCBase, iframe);
            SwitchPPC();
            break;
        }
        case VEC_DECREMENTER:
        {
            struct ExcData* eData;

            while (eData = (struct ExcData*)RemHeadPPC((struct List*)&PowerPCBase->pp_ReadyExc))
            {
                if (eData->ed_ExcID & 1<<EXCB_MCHECK)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcMCheck, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_DACCESS)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcDAccess, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_IACCESS)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcIAccess, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_INTERRUPT)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcInterrupt, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_ALIGN)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcAlign, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_PROGRAM)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcProgram, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_FPUN)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcFPUn, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_DECREMENTER)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcDecrementer, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_SC)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcSystemCall, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_TRACE)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcTrace, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_PERFMON)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcPerfMon, (struct Node*)eData);
                if (eData->ed_ExcID & 1<<EXCB_IABR)
                    EnqueuePPC((struct List*)&PowerPCBase->pp_ExcIABR, (struct Node*)eData);
                eData->ed_Flags |= 1<<EXCB_ACTIVE;
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
                lastExc->ed_Flags &= ~(1<EXCB_ACTIVE);
            }

            HandleMsgs(PowerPCBase, iframe);
            SwitchPPC();
            break;
        }
        case VEC_DATASTORAGE:
        {
            if ((iframe->if_Context.ec_DAR == 4) || (iframe->if_Context.ec_UPC.ec_SRR0 > 0x10000) ||
            ((iframe->if_Context.ec_DAR < 0xfffff800) && (iframe->if_Context.ec_DAR < 0x800)))
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
                    if (myData.dm_LoadFlag)
                    {
                         while (myFrame->mf_Identifier != ID_DONE);
                         FinDataStore(myFrame->mf_Arg[0], iframe, iframe->if_Context.ec_UPC.ec_SRR0, &myData);
                    }
                    iframe->if_Context.ec_UPC.ec_SRR0  += 4;
                }
                else
                {
                    CommonExcError(PowerPCBase, iframe);
                }
            break;
            }
            CommonExcHandler(PowerPCBase, iframe,(struct List*)&PowerPCBase->pp_ExcDAccess);
            break;
        }
        case VEC_PROGRAM:
        {                
            if (PowerPCBase->pp_SuperAddress == iframe->if_Context.ec_UPC.ec_PC)
            {
                iframe->if_Context.ec_UPC.ec_SRR0  += 4;
                iframe->if_Context.ec_SRR1         &= ~PSL_PR;
                iframe->if_Context.ec_GPR[0]        = 0;            //Set SuperKey
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
                if(!(PowerPCBase->pp_AlignmentExcLow += 1))
                {
                    PowerPCBase->pp_AlignmentExcHigh += 1;
                }
                if(!(DoAlign(iframe, iframe->if_Context.ec_UPC.ec_SRR0)))
                {
                    CommonExcHandler(PowerPCBase, iframe, (struct List*)&PowerPCBase->pp_ExcAlign);
                }
            }
            break;
        }
        case VEC_ALTIVECUNAV:
        {
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
        default:
        {
            break;
        }
    }

    PowerPCBase->pp_ExceptionMode = 0;
    setDEC(PowerPCBase->pp_Quantum);
    return;
}

/********************************************************************************************
*
*	This function dispatches new PPC tasks, switches between tasks and redirect signals.
*
*********************************************************************************************/

PPCKERNEL void HandleMsgs(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe)
{
    if (PowerPCBase->pp_Mutex)
    {
        PowerPCBase->pp_Quantum = 0x1000;
        return;
    }

    struct MsgFrame* currMsg = (struct MsgFrame*)PowerPCBase->pp_MsgQueue.mlh_Head;
    struct MsgFrame* nxtMsg;
    while (nxtMsg = (struct MsgFrame*)currMsg->mf_Message.mn_Node.ln_Succ)
    {
        switch (currMsg->mf_Identifier)
        {
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
                    struct MsgPortPPC* myPort = myTask->tp_Msgport; //WIP
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
            case ID_LLPP:
            {
                struct PrivateTask* myTask = (struct PrivateTask*)PowerPCBase->pp_ThisPPCProc;
                if (myTask->pt_Mirror68K == (struct Task*)currMsg->mf_Arg[1])
                {
                    myTask->pt_Task.tp_Task.tc_State = TS_RUN;
                    myTask->pt_Task.tp_Task.tc_SigRecvd |= currMsg->mf_Arg[0];
                }
                else
                {
                    struct PrivateTask* nxtTask;
                    struct PrivateTask* currTask = (struct PrivateTask*)PowerPCBase->pp_WaitingTasks.mlh_Head;
                    while (nxtTask = (struct PrivateTask*)currTask->pt_Task.tp_Task.tc_Node.ln_Succ)
                    {
                        if (currTask->pt_Mirror68K == (struct Task*)currMsg->mf_Arg[1])
                        {
                            currTask->pt_Task.tp_Task.tc_State = TS_READY;
                            currTask->pt_Task.tp_Task.tc_SigRecvd |= currMsg->mf_Arg[0];
                            break;
                        }
                        currTask = nxtTask;
                    }
                    if (!(nxtTask))
                    {
                        currTask = (struct PrivateTask*)PowerPCBase->pp_ReadyTasks.mlh_Head;
                        while (nxtTask = (struct PrivateTask*)currTask->pt_Task.tp_Task.tc_Node.ln_Succ)
                        {
                            if (currTask->pt_Mirror68K == (struct Task*)currMsg->mf_Arg[1])
                            {
                                currTask->pt_Task.tp_Task.tc_SigRecvd |= currMsg->mf_Arg[0];
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

PPCKERNEL void SwitchPPC(void)
{
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
    ULONG status = 0;

    while (nxtNode = (struct ExcData*)currNode->ed_Node.ln_Succ)
    {
        if (currNode->ed_ExcID & iframe->if_Context.ec_ExcID)
        {
            if ((currNode->ed_Flags & EXCF_GLOBAL) || ((currNode->ed_Flags & EXCF_LOCAL) && (currNode->ed_Task) && (currNode->ed_Task != PowerPCBase->pp_ThisPPCProc)))
            {
                if (currNode->ed_Flags & EXCF_LARGECONTEXT)
                {
                    ULONG (*ExcHandler)(__reg("r2") ULONG, __reg("r3") struct EXCContext*) = currNode->ed_Code;
                    ULONG tempR2 = getR2();
                    status = ExcHandler(currNode->ed_Data, &iframe->if_Context);
                    storeR2(tempR2);
                }
                else if (currNode->ed_Flags &EXCF_SMALLCONTEXT)
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
    return;
}

/********************************************************************************************
*
*     Entry point to print an exception error in a window.
*
*********************************************************************************************/

PPCKERNEL void CommonExcError(struct PrivatePPCBase* PowerPCBase, struct iframe* iframe)
{
    return;
}

; Copyright (c) 2019, 2020 Dennis van der Boon
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
        	
FUNC_CNT	SET	   -30	   * Skip 4 standard vectors
FUNCDEF		MACRO
_LVO\1		EQU	   FUNC_CNT
FUNC_CNT	SET	   FUNC_CNT-6  * Standard offset-6 bytes each
    		ENDM

            include exec/exec_lib.i
            include exec/execbase.i
            include exec/tasks.i
            include exec/memory.i
            include dos/dosextens.i
            include powerpc/powerpc.i

            XREF _ChangeStackRamLib, _Run68KCode, _myRunPPC, _MyCopy, _GetClock
            XDEF _myCRunPPC

;********************************************************************************************

_MyCopy     move.l (a0)+,(a1)+
            dbf d0,_MyCopy
            rts

_GetClock   move.b $bfe701,2(a0)
            move.b $bfe601,3(a0)
            move.w #0,0(a0)
            rts

;********************************************************************************************

_myRunPPC   movem.l d1-a6,-(a7)
            bsr _myCRunPPC
            movem.l (a7)+,d1-a6
            rts

;********************************************************************************************

_ChangeStackRamLib
            movem.l d1-a6,-(a7)
            move.l d0,d6
            sub.l a1,a1
            jsr _LVOFindTask(a6)
            tst.l d0
            beq PatchExit

            move.l d0,a3
            move.l TC_SPUPPER(a3),d0
            move.l TC_SPLOWER(a3),d1
            sub.l d1,d0
            cmp.l d6,d0                  ;don't make stack smaller!
            blt.s DoStackMagic

            moveq.l #-1,d0
            bra.s PatchExit

DoStackMagic	
            move.l d6,d0
            move.l #MEMF_PUBLIC,d1
            jsr _LVOAllocVec(a6)

            tst.l d0
            beq.s PatchExit

            move.l d0,-(a7)
            add.l d6,d0
            move.l d0,-(a7)
            move.l TC_SPUPPER(a3),d1
            move.l TC_SPREG(a3),d2
            move.l d2,a0
            sub.l d2,d1
            sub.l d1,d0
            move.l d0,a1
            move.l a1,-(a7)
            move.l d1,d0

            jsr _LVOCopyMem(a6)          ;Copy stack to new spot

            move.l (a7)+,TC_SPREG(a3)
            move.l (a7)+,d0
            move.l TC_SPUPPER(a3),d2
            move.l d0,TC_SPUPPER(a3)
            move.l (a7)+,TC_SPLOWER(a3)
            move.l a7,d1
            move.l d0,d3
            sub.l d1,d2
            sub.l d2,d3
            cmp.b #NT_PROCESS,LN_TYPE(a3)
            bne.s NotAProc

		    lsr.l #2,d0
            move.l d0,pr_StackBase(a3)
            move.l d6,pr_StackSize(a3)
NotAProc    move.l d3,a7                 ;Set new stack pointer
            moveq.l #-1,d0
PatchExit   movem.l (a7)+,d1-a6
            rts


;********************************************************************************************

_Run68KCode
            movem.l d0-a6,-(a7)               ;a0 = PPStruct  a6 = SysBase
            lea TempSysB(pc),a2
            move.l a6,(a2)

            btst #AFB_FPU40,AttnFlags+1(a6)
            beq.s NoFPU1

            fmove.d fp0,-(a7)
            fmove.d fp1,-(a7)
            fmove.d fp2,-(a7)
            fmove.d fp3,-(a7)
            fmove.d fp4,-(a7)
            fmove.d fp5,-(a7)
            fmove.d fp6,-(a7)
            fmove.d fp7,-(a7)

            lea PP_FREGS(a0),a2

            fmove.d (a2)+,fp0
            fmove.d (a2)+,fp1
            fmove.d (a2)+,fp2
            fmove.d (a2)+,fp3
            fmove.d (a2)+,fp4
            fmove.d (a2)+,fp5
            fmove.d (a2)+,fp6
            fmove.d (a2)+,fp7

NoFPU1      link a1,#-16
            lea -16(a1),a1
            move.l a0,-(a7)
            move.l a0,a5
            tst.l PP_STACKPTR(a0)
            beq NoStckPtr

            move.l PP_STACKSIZE(a0),d0
            beq NoStckPtr

            move.l a1,a2
            move.l a1,a3
            move.l #$2e7a0008,(a2)+
            move.w #$4ef9,(a2)+
            move.l #xBack,(a2)+
            move.l a7,(a2)+

            move.l PP_STACKPTR(a0),a0
            lea 24(a0),a0                     ;Offset needs to be 24 according to Run68K docs
            addq.l #3,d0
            and.l #$fffffffc,d0               ;Make it 4 aligned
            move.l a7,a1
            sub.l d0,a1
            move.l a1,a7
            jsr _LVOCopyMem(a6)
            jsr _LVOCacheClearU(a6)

            move.l a3,-(a7)                   ;Return function
            bra StckPtr

NoStckPtr   pea xBack(pc)
StckPtr     move.l PP_CODE(a5),a0
            add.l PP_OFFSET(a5),a0
            move.l a0,-(a7)
            lea PP_REGS(a5),a6
            movem.l (a6)+,d0-a5
            move.l (a6),a6
            rts

xBack       move.l a6,-(a7)
            move.l 4(a7),a6
            lea PP_REGS+56(a6),a6
            movem.l d0-a5,-(a6)
            move.l a6,a0
            move.l (a7)+,a6
            move.l a6,PP_REGS+56(a0)

            move.l TempSysB(pc),a6
            move.l (a7)+,a3
            btst #AFB_FPU40,AttnFlags+1(a6)
            beq.s NoFPU2

            lea PP_FREGS(a3),a3
            fmove.d fp0,(a3)+
            fmove.d fp1,(a3)+
            fmove.d fp2,(a3)+
            fmove.d fp3,(a3)+
            fmove.d fp4,(a3)+
            fmove.d fp5,(a3)+
            fmove.d fp6,(a3)+
            fmove.d fp7,(a3)+

NoFPU2      move.l a7,a1
            lea 16(a1),a1
            unlk a1
            move.l TempSysB(pc),a6
            btst #AFB_FPU40,AttnFlags+1(a6)
            beq.s NoFPU3
		
            fmove.d (a7)+,fp7
            fmove.d (a7)+,fp6
            fmove.d (a7)+,fp5
            fmove.d (a7)+,fp4
            fmove.d (a7)+,fp3
            fmove.d (a7)+,fp2
            fmove.d (a7)+,fp1
            fmove.d (a7)+,fp0

NoFPU3      movem.l (a7)+,d0-a6
            rts

;********************************************************************************************

TempSysB    dc.l 0

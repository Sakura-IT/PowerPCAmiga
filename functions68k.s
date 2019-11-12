; Copyright (c) 2019 Dennis van der Boon
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

            include	exec/exec_lib.i
            include	exec/execbase.i
            include powerpc/powerpc.i

            XREF _Run68KCode

;********************************************************************************************

_Run68KCode
            movem.l d0-a6,-(a7)         ;a0 = PPStruct      a6 = SysBase
            lea TempSysB(pc),a2
            move.l a6,(a2)

            btst #AFB_FPU40,AttnFlags+1(a6)
            beq.s NoFPU1

            fmovem.x fp0-fp7,-(a7)

    		lea PP_FREGS(a0),a2

            fmovem.x (a2)+,fp0-fp7

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
		    lea 24(a0),a0				;Offset needs to be 24 according to Run68K docs
		    addq.l #3,d0
		    and.l #$fffffffc,d0			;Make it 4 aligned
		    move.l a7,a1
		    sub.l d0,a1
		    move.l a1,a7
		    jsr _LVOCopyMem(a6)
            jsr _LVOCacheClearU(a6)

            move.l a3,-(a7)				;Return function
		    bra StckPtr

NoStckPtr	pea xBack(pc)
StckPtr		move.l PP_CODE(a5),a0
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
		    move.l a6,PP_REGS+60(a0)

            move.l TempSysB(pc),a6
            btst #AFB_FPU40,AttnFlags+1(a6)
		    beq.s NoFPU2
		
            move.l (a7)+,a3
		    lea PP_FREGS+64(a3),a3
            fmovem.x fp0-fp7,-(a3)

NoFPU2      move.l TempSysB(pc),a6
		    btst #AFB_FPU40,AttnFlags+1(a6)
		    beq.s NoFPU3
		
            fmovem.x (a7)+,fp0-fp7

NoFPU3      move.l a7,a1
		    lea 16(a1),a1
		    unlk a1

            movem.l (a7)+,d0-a6
            rts

;********************************************************************************************

TempSysB    dc.l 0

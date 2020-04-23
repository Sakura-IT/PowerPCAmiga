## Copyright (c) 2019, 2020 Dennis van der Boon
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.


.macro loadreg register, value
	.if 	(\value >= -0x8000) && (\value <= 0x7fff)
        	li      \register, \value
	.else
		lis     \register, \value@h
		ori     \register, \register, \value@l
	.endif
.endm

.set IF_GAP,                    256
.set IF_VPR,                    0                 #Align this with the if_Frame struct in libstructs.h
.set IF_VSCR,                   32*16
.set IF_VRSAVE,                 IF_VSCR+16
.set IF_PAD,                    IF_VRSAVE+4
.set IF_CONTEXT,                IF_PAD+4
.set IF_CONTEXT_EXCID,          IF_CONTEXT
.set IF_CONTEXT_SRR0,           IF_CONTEXT+4
.set IF_CONTEXT_SRR1,           IF_CONTEXT+8
.set IF_CONTEXT_DAR,            IF_CONTEXT+12
.set IF_CONTEXT_DSISR,          IF_CONTEXT+16
.set IF_CONTEXT_CR,             IF_CONTEXT+20
.set IF_CONTEXT_CTR,            IF_CONTEXT+24
.set IF_CONTEXT_LR,             IF_CONTEXT+28
.set IF_CONTEXT_XER,            IF_CONTEXT+32
.set IF_CONTEXT_FPSCR,          IF_CONTEXT+36
.set IF_CONTEXT_GPR,            IF_CONTEXT+40
.set IF_CONTEXT_FPR,            IF_CONTEXT+40+128
.set IF_BATS,                   IF_CONTEXT+40+128+256
.set IF_SEGMENTS,               IF_CONTEXT+40+128+256+64
.set IF_ALIGNSTORE,             IF_SEGMENTS+64
.set IF_EXCEPTIONVECTOR,        IF_ALIGNSTORE+8
.set IF_EXCNUM,                 IF_EXCEPTIONVECTOR+4
.set IF_SIZE,                   IF_EXCNUM+4

.set PP_FLAGS,                  8
.set PP_STACK,                  12
.set PP_STACKSIZE,              16
.set PP_REGS,                   20
.set PP_FREGS,                  80

.set PPB_LINEAR,                1
.set PPB_THROW,                 2

.set WAITTIME_UPPER,            14                #Align with struct WaitTime in libstructs.h
.set WAITTIME_LOWER,            18

.set GPR0,                      0*4
.set GPR1,                      1*4
.set GPR2,                      2*4
.set GPR3,                      3*4
.set GPR4,                      4*4

.set PPC_VECLEN,                3
.set PowerPCBase,               16                #Align this number with the zp struct in libstructs.h

.set HID0,                      1008
.set HID0_NHR,	                0x00010000

.set PSL_FP,	                0x00002000        #/* B.6. floating point enable */
.set PSL_IP,	                0x00000040        #/* ..6. interrupt prefix */
.set PSL_TGPR,               	0x00020000        #/* ..6. temp. gpr remapping (mpc603e) */
.set PSL_IR,	                0x00000020        #/* .468 instruction address relocation */
.set PSL_DR,                    0x00000010        #/* .468 data address relocation */

.set DMISS,                     976
.set DCMP,                      977
.set HASH1,                     978
.set HASH2,                     979
.set IMISS,                     980
.set ICMP,                      981
.set RPA,                       982

.set HID0_ICFI,                 0x00000800

.set PTE_GUARDED,               0x00000001
.set PTE_REFERENCED,            0x00000100
.set PTE_CHANGED,               0x00000080
.set PTE_HASHID,                0x00000040

.set DSISR_NOTFOUND,            0x40000000
.set DSISR_PROTECT,             0x08000000

.set SRR1_KEY,	                0x00080000

.set ibat4u,560
.set ibat4l,561
.set ibat5u,562
.set ibat5l,563
.set ibat6u,564
.set ibat6l,565
.set ibat7u,566
.set ibat7l,567
.set dbat4u,568
.set dbat4l,569
.set dbat5u,570
.set dbat5l,571
.set dbat6u,572
.set dbat6l,573
.set dbat7u,574
.set dbat7l,575

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

.include    constantsppc.i

#*********************************************************

.global     _setupPPC, _Reset, _GetExcTable, _GetVecEntry
.global     _getTableFX, _getTable100, _getTable66
.global     _getL2Size, _detectMem

#*********************************************************

.section "setupppc","acrx"

        bl	.SkipCom		    #0x3000	System initialization

        #below is a mirror of struct InitData in libstruct.h

        .long   0               #Used for initial communication
        .long   0               #MemStart
        .long   0               #MemLen
        .long   0               #RTGBase
        .long   0               #RTGLen
        .long   0               #RTGType
        .long   0               #RTGConfig
        .long   0               #Options1
        .long   0               #Options2
        .long   0               #Options3
        .long   0               #DeviceID
        .long   0               #ConfigBase
        .long   0               #MsgBase
        .long   0               #MPIC
        .long   0               #StartBAT
        .long   0               #SizeBAT

#*********************************************************

.SkipCom:	
        mflr	r3
        subi    r3,r3,4         #Points to initialization data
        subi    r1,r3,256       #0x3000 bytes of initial stack

        b   _setupPPC

#*********************************************************

_Reset:
        mflr    r6
        mfmsr	r5
        andi.	r5,r5,PSL_IP
        mtmsr	r5              #Clear MSR, keep Interrupt Prefix for now
        isync

        li      r3,0            #Zero-out registers
        mtsprg0	r3
        mtsprg1 r3
        mtsprg2	r3
        mtsprg3	r3

        loadreg	r3,HID0_NHR     #Set HID0 to known state
        mfspr	r4,HID0
        and     r3,r4,r3        #Clear other bits
        mtspr	HID0,r3
        sync

        loadreg	r3,PSL_FP       #Set MPU/MSR to a known state. Turn on FP
        or      r3,r5,r3
        mtmsr 	r3
        isync

        mtfsfi  7,0             #Init the floating point control/status register
        mtfsfi  6,0
        mtfsfi  5,0
        mtfsfi  4,0
        mtfsfi  3,0
        mtfsfi  2,0
        mtfsfi  1,0
        mtfsfi  0,0
        isync

        bl	ifpdr_value         #Initialize floating point data regs to known state

        .long   0x3f800000      #Value of 1.0

ifpdr_value:
        mflr	r3
        lfs     f0,0(r3)
        lfs     f1,0(r3)
        lfs     f2,0(r3)
        lfs     f3,0(r3)
        lfs     f4,0(r3)
        lfs     f5,0(r3)
        lfs     f6,0(r3)
        lfs     f7,0(r3)
        lfs     f8,0(r3)
        lfs     f9,0(r3)
        lfs     f10,0(r3)
        lfs     f11,0(r3)
        lfs     f12,0(r3)
        lfs     f13,0(r3)
        lfs     f14,0(r3)
        lfs     f15,0(r3)
        lfs     f16,0(r3)
        lfs     f17,0(r3)
        lfs     f18,0(r3)
        lfs     f19,0(r3)
        lfs     f20,0(r3)
        lfs     f21,0(r3)
        lfs     f22,0(r3)
        lfs     f23,0(r3)
        lfs     f24,0(r3)
        lfs     f25,0(r3)
        lfs     f26,0(r3)
        lfs     f27,0(r3)
        lfs     f28,0(r3)
        lfs     f29,0(r3)
        lfs     f30,0(r3)
        lfs     f31,0(r3)
        sync

        li      r3,0            #Clear BAT and Segment mapping registers
        mtibatu 0,r3
        mtibatu 1,r3
        mtibatu 2,r3
        mtibatu 3,r3
        mtibatl 0,r3
        mtibatl 1,r3
        mtibatl 2,r3
        mtibatl 3,r3
        mtdbatu 0,r3
        mtdbatu 1,r3
        mtdbatu 2,r3
        mtdbatu 3,r3
        mtdbatl 0,r3
        mtdbatl 1,r3
        mtdbatl 2,r3
        mtdbatl 3,r3

        mfpvr	r4
        rlwinm	r5,r4,16,16,31
        cmplwi	r5,0x8083
        beq     .ExtraBats

        rlwinm	r5,r4,8,24,31
        cmpwi	r5,0x70
        bne     .SkipExtraBats

.ExtraBats:	
        mtspr	ibat4u,r3
        mtspr	ibat5u,r3
        mtspr	ibat6u,r3
        mtspr	ibat7u,r3
        mtspr	ibat4l,r3
        mtspr	ibat5l,r3
        mtspr	ibat6l,r3
        mtspr	ibat7l,r3
        mtspr	dbat4u,r3
        mtspr	dbat5u,r3
        mtspr	dbat6u,r3
        mtspr	dbat7u,r3
        mtspr	dbat4l,r3
        mtspr	dbat5l,r3
        mtspr	dbat6l,r3
        mtspr	dbat7l,r3

.SkipExtraBats:	
        isync
        sync
        sync

        mtsr	0,r3
        mtsr	1,r3
        mtsr	2,r3
        mtsr	3,r3
        mtsr	4,r3
        mtsr	5,r3
        mtsr	6,r3
        mtsr	7,r3
        mtsr	8,r3
        mtsr	9,r3
        mtsr	10,r3
        mtsr	11,r3
        mtsr	12,r3
        mtsr	13,r3
        mtsr	14,r3
        mtsr	15,r3
		
        isync
        sync
        sync

        mttbu   r3
        mttbl   r3

        mtlr	r6
        blr
#*********************************************************

_getL2Size:
        mr      r6,r3                   #Determine size of L2 Cache
        mr      r3,r4
        li      r5,0
        mr      r8,r5
        lis     r4,0

        subis   r6,r6,0x40              #Substract 4 MB
        lis     r5,L2_SIZE_1M_U         #Size of memory to write to

.L2SzWriteLoop:
        dcbz    r4,r6
        stwx    r4,r4,r6
        dcbf    r4,r6
        addi    r4,r4,L2_ADR_INCR
        cmpw    r4,r5
        blt     .L2SzWriteLoop

        lis     r4,0

.L2SzReadLoop:
        lwzx    r7,r4,r6
        cmpw    r4,r7
        bne     .L2SkipCount
        addi    r8,r8,1                 #Count cache lines

.L2SkipCount:
        dcbi    r4,r6
        addi    r4,r4,L2_ADR_INCR
        cmpw    r4,r5
        blt     .L2SzReadLoop

#       lis     r7,L2CR_L2SIZ_2M@h
#       cmpwi   r8,L2_SIZE_2M
#       beq     .L2SizeDone

        lis     r7,L2CR_L2SIZ_1M@h
        cmpwi   r8,L2_SIZE_1M
        beq     .L2SizeDone

        lis     r7,L2CR_L2SIZ_HM@h
        cmpwi   r8,L2_SIZE_HM
        beq     .L2SizeDone

        lis     r7,L2CR_L2SIZ_QM@h
        cmpwi   r8,L2_SIZE_QM
        beq     .L2SizeDone

        lis     r7,0
        mr      r8,r5

.L2SizeDone:
        stw     r7,0(r3)
        stw     r8,4(r3)
        blr

#*********************************************************

_getTable100:
        mflr    r4
        bl      .EndTable

        .long	0b1110,350000000,0b1010,400000000
        .long	0b0111,450000000,0b1011,500000000
        .long	0b1001,550000000,0b1101,600000000
        .long	0b0101,650000000,0b0010,700000000
        .long	0b0001,750000000,0b1100,800000000
        .long	0b0000,900000000,0

#*********************************************************

_getTableFX:
        mflr    r4
        bl      .EndTable

        .long	0b01110,700000000,0b01111,750000000
        .long	0b10000,800000000,0b10001,850000000
        .long	0b10010,900000000,0b10011,950000000
        .long	0b10100,1000000000,0

#*********************************************************

_getTable66:
        mflr    r4
        bl      .EndTable

        .long	0b1101,400000000,0b0001,500000000
        .long	0b0101,433333333,0b0010,466666666
        .long	0b1100,533333333,0

#*********************************************************

_GetVecEntry:
        mflr    r4
        bl      .EndTable

_VecEntry:
        mtsprg3 r0
        mflr    r0
        bl      0x0
.EndEntry:

#*********************************************************

_GetExcTable:
        mflr    r4
        bl      .EndTable

        .word 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700, 0x0800, 0x0900, 0x0c00
        .word 0x0d00, 0x0e00, 0x0f00, 0x0f20, 0x1000, 0x1100, 0x1200, 0x1300, 0x1400, 0x1600
        .word 0x1700, -1

.EndTable:
        mflr    r3
        mtlr    r4
        blr

#*********************************************************

_detectMem:
        stwu    r1,-256(r1)
        stw     r13,100(r1)
        stw     r14,104(r1)
        stw     r15,108(r1)
        stw     r16,112(r1)
        stw     r17,116(r1)
        stw     r18,120(r1)
        stw     r19,124(r1)
        stw     r20,128(r1)
        stw     r21,132(r1)
        stw     r22,136(r1)
        stw     r23,140(r1)
        stw     r24,144(r1)
        stw     r25,148(r1)
        stw     r26,152(r1)
        stw     r27,156(r1)
        stw     r28,160(r1)
        stw     r29,164(r1)
        stw     r30,168(r1)
        stw     r31,172(r1)

        mr      r15,r3
        lwz     r26,MS_MCCR1TESTHIGH(r3)
        lwz     r28,MS_MCCR1TESTLOW1(r3)
        lwz     r30,MS_MCCR1TESTLOW2(r3)
        lwz     r31,MS_MCCR1TESTLOW3(r3)

		li      r3,0
        lis     r4,0x426f
        ori     r4,r4,0x6f6e
		li      r5,1
		li      r8,0
		li      r9,0
		li      r10,0
		li      r11,0
		li      r12,0
		mr      r13,r26
		li      r14,0
		li      r16,0
		li      r17,0
		li      r18,0
		li      r19,0
        lis     r22,CMD_BASE@h

loc_3BD8:
        setpcireg MPC107_MBEN		   #Memory Bank Enable register
		mr      r25,r5
        configwrite8                   #enable Bank as given in r5

		stw     r4,0(r3)               #try to store "Boon" at address 0x0
		eieio

		stw     r3,4(r3)               #try to store 0x0 at 0x4
		eieio
		lwz     r7,0(r3)               #read from 0x0
		cmplw   r4,r7                  #is it "Boon", long compare
		bne     loc_4184

		or      r14,r14,r5             #continue if found
		setpcireg MPC107_MCCR1         #0x800000f0
		or      r25,r26,r28
        configwrite32                  #set all banks to 12 or 13 row bits

		lis     r6,0x40
		stw     r3,0(r6)		#set 0x400000 to 0x0
		lis     r6,0x80
		stw     r3,0(r6)		#set 0x800000 to 0x0
		lis     r6,0x100
		stw     r3,0(r6)		#set 0x1000000 to 0x0
		lis     r6,0x200
		stw     r3,0(r6)		#set 0x2000000 to 0x0
		lis     r6,0x400
		stw     r3,0(r6)		#set 0x4000000 to 0x0
		lis     r6,0x800
		stw     r3,0(r6)		#set 0x8000000 to 0x0
		eieio
		stw     r4,0(r3)		#set 0x0 to "Boon"
		eieio
		lis     r6,0x40
		lwz     r7,0(r6)		#read from 0x400000
		cmplw   r4,r7			#is it "Boon"
		beq     loc_3CBC		#if yes goto loc_3CBC
		lis     r6,0x80
		lwz     r7,0(r6)		#read form 0x800000
		cmplw   r4,r7			#is it "Boon"
		beq     loc_3E24		#if yes goto loc_3E24
		lis     r6,0x100
		lwz     r7,0(r6)		#read from 0x1000000
		cmplw   r4,r7			#is it "Boon"
		beq     loc_3E24		#if yes goto loc_3E24
		lis     r6,0x200
		lwz     r7,0(r6)		#read from 0x2000000
		cmplw   r4,r7
		beq     loc_3E24		#if its "Boon" goto loc_3E24
		lis     r6,0x400
		lwz     r7,0(r6)		#read from 0x4000000
		cmplw   r4,r7
		beq     loc_3E24		#if its "Boon" goto loc_3E24
		lis     r6,0x800
		lwz     r7,0(r6)		#read from 0x8000000
		cmplw   r4,r7
		beq     loc_3E24		#if its "Boon" goto loc_3E24
		lis     r6,0x1000
		cmpwi   r7,0
		beq     loc_3E24
		b       loc_4184		#goto loc_4184

#********************************************************************************************
loc_3CBC:					#CODE XREF: findSetMem+1D0
		or      r25,r26,r30
        configwrite32
		lis     r6,0x20			#continue tests
		stw     r3,0(r6)
		lis     r6,0x40
		stw     r3,0(r6)
		lis     r6,0x80
		stw     r3,0(r6)
		lis     r6,0x100
		stw     r3,0(r6)
		lis     r6,0x200
		stw     r3,0(r6)
		eieio
		stw     r4,0(r3)
		eieio
		lis     r6,0x20
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3D50
		lis     r6,0x40
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3E24
		lis     r6,0x80
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3E24
		lis     r6,0x100
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3E24
		lis     r6,0x200
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3E24
		b       loc_4184

#********************************************************************************************
loc_3D50:					#CODE XREF: findSetMem+274
		or      r25,r26,r31
        configwrite32
		lis     r6,0x10			#continue tests
		stw     r3,0(r6)
		lis     r6,0x20
		stw     r3,0(r6)
		lis     r6,0x40
		stw     r3,0(r6)
		lis     r6,0x80
		stw     r3,0(r6)
		eieio
		stw     r4,0(r3)
		eieio
		lis     r6,0x10
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3DCC
		lis     r6,0x20
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3E24
		lis     r6,0x40
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3E24
		lis     r6,0x80
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3E24
		b       loc_4184

#********************************************************************************************
loc_3DCC:					#CODE XREF: findSetMem+300
		lis     r6,8
		stw     r3,0(r6)
		lis     r6,0x10
		stw     r3,0(r6)
		lis     r6,0x20
		stw     r3,0(r6)
		eieio
		stw     r4,0(r3)
		eieio
		lis     r6,8
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_4184
		lis     r6,0x10
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3E24
		lis     r6,0x20
		lwz     r7,0(r6)
		cmplw   r4,r7
		beq     loc_3E24
		b       loc_4184

#********************************************************************************************
loc_3E24:					#CODE XREF: findSetMem+1E0
						#findSetMem+1F0 ...
		cmplwi  r5,1
		bne     loc_3E84
		mr      r7,r8
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		or      r9,r9,r7
		mr      r7,r8
		srwi    r7,r7,28
		andi.   r7,r7,3
		or      r16,r16,r7
		add     r8,r8,r6
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		or      r11,r11,r7
		mr      r7,r8
		addi    r7,r7,0xFF
		srwi    r7,r7,28
		andi.   r7,r7,3
		or      r18,r18,r7
		andi.   r25,r25,3
		or      r13,r13,r25
		b       loc_4184

#********************************************************************************************
loc_3E84:					#CODE XREF: findSetMem+394
		cmplwi  r5,2
		bne     loc_3EF4
		mr      r7,r8
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,8
		or      r9,r9,r7
		mr      r7,r8
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,8
		or      r16,r16,r7
		add     r8,r8,r6
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,8
		or      r11,r11,r7
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,8
		or      r18,r18,r7
		andi.   r25,r25,0xC
		or      r13,r13,r25
		b       loc_4184

#********************************************************************************************
loc_3EF4:					#CODE XREF: findSetMem+3F4
		cmplwi  r5,4
		bne     loc_3F64
		mr      r7,r8
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,16
		or      r9,r9,r7
		mr      r7,r8
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,16
		or      r16,r16,r7
		add     r8,r8,r6
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,16
		or      r11,r11,r7
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,16
		or      r18,r18,r7
		andi.   r25,r25,0x30
		or      r13,r13,r25
		b       loc_4184

#********************************************************************************************
loc_3F64:					#CODE XREF: findSetMem+464
		cmplwi  r5,8
		bne     loc_3FD4
		mr      r7,r8
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,24
		or      r9,r9,r7
		mr      r7,r8
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,24
		or      r16,r16,r7
		add     r8,r8,r6
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,24
		or      r11,r11,r7
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,24
		or      r18,r18,r7
		andi.   r25,r25,0xC0
		or      r13,r13,r25
		b       loc_4184

#********************************************************************************************
loc_3FD4:					#CODE XREF: findSetMem+4D4
		cmplwi  r5,0x10
		bne     loc_4034
		mr      r7,r8
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		or      r10,r10,r7
		mr      r7,r8
		srwi    r7,r7,28
		andi.   r7,r7,3
		or      r17,r17,r7
		add     r8,r8,r6
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		or      r12,r12,r7
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,28
		andi.   r7,r7,3
		or      r19,r19,r7
		andi.   r25,r25,0x300
		or      r13,r13,r25
		b       loc_4184

#********************************************************************************************
loc_4034:
		cmplwi  r5,0x20
		bne     loc_40A4
		mr      r7,r8
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,8
		or      r10,r10,r7
		mr      r7,r8
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,8
		or      r17,r17,r7
		add     r8,r8,r6
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,8
		or      r12,r12,r7
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,8
		or      r19,r19,r7
		andi.   r25,r25,0xC00
		or      r13,r13,r25
		b       loc_4184

#********************************************************************************************
loc_40A4:
		cmplwi  r5,0x40
		bne     loc_4114
		mr      r7,r8
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,16
		or      r10,r10,r7
		mr      r7,r8
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,16
		or      r17,r17,r7
		add     r8,r8,r6
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,16
		or      r12,r12,r7
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,16
		or	    r19,r19,r7
		andi.   r25,r25,0x3000
		or      r13,r13,r25
		b       loc_4184

#********************************************************************************************
loc_4114:
		cmplwi  r5,0x80
		bne     loc_4184
		mr      r7,r8
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,24
		or      r10,r10,r7
		mr      r7,r8
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,24
		or      r17,r17,r7
		add     r8,r8,r6
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,20
		andi.   r7,r7,0xFF
		slwi    r7,r7,24
		or      r12,r12,r7
		mr      r7,r8
		addi    r7,r7,-1
		srwi    r7,r7,28
		andi.   r7,r7,3
		slwi    r7,r7,24
		or      r19,r19,r7
		andi.   r25,r25,0xc000
		or      r13,r13,r25
		b       loc_4184

#********************************************************************************************
loc_4184:
		slwi    r5,r5,1
		cmplwi  r5,0x100
		bne     loc_3BD8

        stw     r9,MS_MSAR1(r15)
        stw     r10,MS_MSAR2(r15)
        stw     r11,MS_MEAR1(r15)
        stw     r12,MS_MEAR2(r15)
        stw     r16,MS_MESAR1(r15)
        stw     r17,MS_MESAR2(r15)
        stw     r18,MS_MEEAR1(r15)
        stw     r19,MS_MEEAR2(r15)
        stw     r14,MS_MBEN(r15)
        stw     r13,MS_MCCR1(r15)
        stw     r8,MS_MEMSIZE(r15)

        lwz     r13,100(r1)
        lwz     r14,104(r1)
        lwz     r15,108(r1)
        lwz     r16,112(r1)
        lwz     r17,116(r1)
        lwz     r18,120(r1)
        lwz     r19,124(r1)
        lwz     r20,128(r1)
        lwz     r21,132(r1)
        lwz     r22,136(r1)
        lwz     r23,140(r1)
        lwz     r24,144(r1)
        lwz     r25,148(r1)
        lwz     r26,152(r1)
        lwz     r27,156(r1)
        lwz     r28,160(r1)
        lwz     r29,164(r1)
        lwz     r30,168(r1)
        lwz     r31,172(r1)
        lwz     r1,0(r1)

		blr

#********************************************************************************************

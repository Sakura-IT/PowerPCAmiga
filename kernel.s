## Copyright (c) 2019 Dennis van der Boon
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

.global     _ExcFPUnav, _ExcAlignment, _ExcInsStor,	 _ExcDatStor, _ExcTrace
.global     _ExcBreakPoint,	_ExcDec, _ExcPrIvate, _ExcMachCheck, _ExcSysCall
.global     _ExcPerfMon, _ExcSysMan, _ExcTherMan, _ExcVMXUnav, _ExcExternal

.section "kernel","acrx"

.ExceptionTable:

	    	b	_ExcMachCheck	           #00 - 0x0200 + 0x3000
	    	b	_ExcDatStor		       	   #04   0x0300
	    	b	_ExcInsStor		       	   #08   0x0400
            b   _ExcExternal               #0c   0x0500
	    	b	_ExcAlignment	       	   #10   0x0600
	    	b	_ExcPrIvate			       #14   0x0700
            b	_ExcFPUnav		       	   #18   0x0800
	    	b	_ExcDec			    	   #1c   0x0900
	    	b	_ExcSysCall		       	   #20   0x0c00
	    	b	_ExcTrace		       	   #24   0x0d00
	    	b	_ExcPerfMon		       	   #28   0x0f00
	    	b	_ExcVMXUnav	       		   #2c   0x0f20
	    	b	ExcITLBMiss     		   #30   0x1000
	    	b	ExcDLoadTLBMiss	 		   #34   0x1100
	    	b	ExcDStoreTLBMiss 		   #38   0x1200
	    	b	_ExcBreakPoint	       	   #3c   0x1300
	    	b	_ExcSysMan	       		   #40   0x1400
	    	b	_ExcTherMan	       		   #44   0x1700

#********************************************************************************************

ExcITLBMiss:
		mfspr	r2,HASH1				#get first pointer
		li	    r1,8					#load 8 for counter
		mfctr	r0	    				#save counter
		mfspr	r3,ICMP					#get first compare value
		addi	r2,r2,-8				#pre dec the pointer
im0:
		mtctr	r1	    				#load counter
im1:
		lwzu	r1,8(r2)				#get next pte
		cmpw	r1,r3 					#see if found pte
		bdnzf	eq,im1					#dec count br if cmp ne and if count not zero
		bne	instrSecHash     			#if not found set up second hash or exit

		lwz	    r1,4(r2)				#load tlb entry lower-word
		andi.	r3,r1,PTE_GUARDED<<3	#check G bit
		bne	doISIp					    #if guarded, take an ISI

		mtctr	r0 					    #restore counter
		mfspr	r0,IMISS 				#get the miss address for the tlbl
		mfsrr1	r3     					#get the saved cr0 bits
		mtcrf	0x80,r3   				#restore CR0
		mtspr	RPA,r1    				#set the pte
		ori	    r1,r1,PTE_REFERENCED	#set reference bit
		srwi	r1,r1,8    				#get byte 7 of pte
		tlbli	r0    					#load the itlb
		stb	    r1,6(r2)     		    #update page table
		rfi     					    #return to executing program

instrSecHash:
		andi.	r1,r3,PTE_HASHID  		#see if we have done second hash
		bne	doISI        				#if so, go to ISI interrupt

		mfspr	r2,HASH2     			#get the second pointer
		ori	    r3,r3,PTE_HASHID      	#change the compare value
		li	    r1,8    				#load 8 for counter
		addi	r2,r2,-8   				#pre dec for update on load
		b 	im0    					    #try second hash

doISIp:
		mfsrr1	r3   					#get srr1
		andi.	r2,r3,0xffff 			#clean upper srr1
		addis	r2,r2,DSISR_PROTECT@h	#or in srr<4> = 1 to flag prot violation
		b 	isi1
doISI:
 		mfsrr1	r3    					#get srr1
 		andi.	r2,r3,0xffff			#clean srr1
		addis	r2,r2,DSISR_NOTFOUND@h	#or in srr1<1> = 1 to flag not found
isi1:
		mtctr	r0 					    #restore counter
		mtsrr1	r2 					    #set srr1
		mfmsr	r0   					#get msr
		xoris	r0,r0,PSL_TGPR@h  		#flip the msr<tgpr> bit
		mtcrf	0x80,r3      			#restore CR0
		mtmsr	r0      				#flip back to the native gprs
		b	_ExcInsStor        	    	#go to instr. access interrupt

#********************************************************************************************

ExcDLoadTLBMiss:
		mfspr	r2,HASH1				#get first pointer
		li	    r1,8   					#load 8 for counter
		mfctr	r0   					#save counter
		mfspr	r3,DCMP   				#get first compare value
		addi	r2,r2,-8  				#pre dec the pointer
dm0:
		mtctr	r1  					#load counter
dm1:
		lwzu	r1,8(r2)				#get next pte
		cmpw	r1,r3 					#see if found pte
		bdnzf	eq,dm1 					#dec count br if cmp ne and if count not zero
		bne	dataSecHash 				#if not found set up second hash or exit

		lwz	    r1,4(r2) 				#load tlb entry lower-word
		mtctr	r0   					#restore counter
		mfspr	r0,DMISS  				#get the miss address for the tlbld
		mfsrr1	r3   					#get the saved cr0 bits
		mtcrf	0x80,r3 				#restore CR0
		mtspr	RPA,r1  				#set the pte
		ori	    r1,r1,PTE_REFERENCED   	#set reference bit
		srwi	r1,r1,8  				#get byte 7 of pte
		tlbld	r0    					#load the dtlb
		stb	    r1,6(r2) 				#update page table
		rfi          					#return to executing program

dataSecHash:
 		andi.	r1,r3,PTE_HASHID     	#see if we have done second hash
		bne	doDSI       				#if so, go to DSI interrupt

		mfspr	r2,HASH2   				#get the second pointer
		ori	    r3,r3,PTE_HASHID     	#change the compare value
		li	    r1,8   					#load 8 for counter
		addi	r2,r2,-8  				#pre dec for update on load
		b	dm0  	    				#try second hash

#********************************************************************************************

ExcDStoreTLBMiss:
		mfspr	r2,HASH1				#get first pointer
		li	    r1,8 					#load 8 for counter
		mfctr	r0   					#save counter
		mfspr	r3,DCMP 				#get first compare value
		addi	r2,r2,-8				#pre dec the pointer
ceq0:
		mtctr	r1    					#load counter
ceq1:
		lwzu	r1,8(r2)				#get next pte
		cmpw	r1,r3    				#see if found pte
		bdnzf	eq,ceq1 				#dec count br if cmp ne and if count not zero
		bne	cEq0SecHash 				#if not found set up second hash or exit

		lwz	    r1,4(r2)   				#load tlb entry lower-word
		andi.	r3,r1,PTE_CHANGED   	#check the C-bit
		beq	cEq0ChkProt 				#if (C==0) go check protection modes
ceq2:
		mtctr	r0       				#restore counter
		mfspr	r0,DMISS 				#get the miss address for the tlbld
		mfsrr1	r3      				#get the saved cr0 bits
		mtcrf	0x80,r3   				#restore CR0
		mtspr	RPA,r1  				#set the pte
		tlbld	r0     					#load the dtlb
		rfi     					    #return to executing program

cEq0SecHash:
		andi.	r1,r3,PTE_HASHID		#see if we have done second hash
		bne	doDSI    			    	#if so, go to DSI interrupt

		mfspr	r2,HASH2 				#get the second pointer
		ori	    r3,r3,PTE_HASHID      	#change the compare value
		li	    r1,8					#load 8 for counter
		addi	r2,r2,-8				#pre dec for update on load
		b	ceq0  				    	#try second hash

cEq0ChkProt:
		rlwinm.	r3,r1,30,0,1 			#test PP
    	bge-    chk0    			    #if (PP == 00 or PP == 01) goto chk0:

		andi.	r3,r1,1 				#test PP[0]
		beq+	chk2   					#return if PP[0] == 0
		b	doDSIp 					    #else DSIp

chk0:
  		mfsrr1	r3      				#get old msr
		andis.	r3,r3,SRR1_KEY@h 		#test the KEY bit (SRR1-bit 12)
		beq	chk2      			    	#if (KEY==0) goto chk2:
		b	doDSIp   			    	#else DSIp
chk2:
   		ori	    r1,r1,(PTE_REFERENCED|PTE_CHANGED)	#set reference and change bit
		sth	r1,6(r2)		    		#update page table
		b	ceq2  				    	#and back we go

doDSI:
 		mfsrr1	r3         				#get srr1
		rlwinm	r1,r3,9,6,6  			#get srr1<flag> to bit 6
		addis	r1,r1,DSISR_NOTFOUND@h	#or in dsisr<1> = 1 to flag not found
		b	dsi1
doDSIp:
    	mfsrr1	r3         				#get srr1
		rlwinm	r1,r3,9,6,6    			#get srr1<flag> to bit 6
		addis	r1,r1,DSISR_PROTECT@h   #or in dsisr<4> = 1 to flag prot violation
dsi1:
  		mtctr	r0           			#restore counter
		andi.	r2,r3,0xffff     		#clear upper bits of srr1
		mtsrr1	r2              		#set srr1
		mtdsisr	r1         				#load the dsisr
		mfspr	r1,DMISS     			#get miss address
		rlwinm.	r2,r2,0,31,31 			#test LE bit
		beq	dsi2     				    #if little endian then:

		xori	r1,r1,0x07   			#de-mung the data address
dsi2:
  		mtdar	r1       				#put in dar
		mfmsr	r0               		#get msr
		xoris	r0,r0,PSL_TGPR@h    	#flip the msr<tgpr> bit
		mtcrf	0x80,r3     			#restore CR0
		mtmsr	r0    					#flip back to the native gprs
		isync
		sync
		sync
		b	_ExcDatStor			    	#branch to DSI interrupt

#********************************************************************************************


// Copyright (c) 2019 Dennis van der Boon
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

#define PPC_VECLEN              3   //Number of instructions of VecEntry (setupppc.s)
#define NUM_OF_68K_FUNCS        49
#define NUM_OF_PPC_FUNCS        95
#define TOTAL_FUNCS             (NUM_OF_68K_FUNCS+NUM_OF_PPC_FUNCS)
#define NEGSIZE                 TOTAL_FUNCS*6

#define PPERR_MISCERR           3

#define ID_PPC                  0x5F505043
#define ID_TPPC                 0x54505043
#define ID_FREE                 0x46524545
#define ID_LLPP                 0x4C4C5050
#define ID_FPPC                 0x46505043
#define ID_T68K                 0x5436384B
#define ID_DONE                 0x444F4E45

#define BASE_KMSG               0x200000
#define SIZE_KBASE              0xc0000
#define SIZE_KFIFO              0x10000
#define FIFO_OFFSET             0x380000

#define HW_CPUTYPE		        11	//Private
#define HW_SETDEBUGMODE		    12	//Private
#define HW_PPCSTATE		        13	//Private

#define ID_MPC834X  			0x8083

#define MEMF_PPC                0x2000
#define MEM_GAP                 0x480000
#define TF_PPC			        1<<2
#define VENDOR_ELBOX            0x89e
#define MEDIATOR_MKII           33
#define MEDIATOR_1200TX         60
#define MEDIATOR_LOGIC          161
#define MEDIATOR_1200LOGIC      188

#define MAX_PCI_SLOTS           6
#define PCI_OFFSET_ID           0
#define PCI_OFFSET_COMMAND      4
#define DEVICENUMBER_SHIFT      3
#define BUSNUMBER_SHIFT         8
#define BUSMASTER_ENABLE        1<<2

#define DEVICE_MPC107           0x0004
#define VENDOR_MOTOROLA         0x1057
#define DEVICE_HARRIER          0x480B
#define DEVICE_MPC8343E         0x0086
#define VENDOR_FREESCALE        0x1957

#define VENDOR_ATI              0x1002
#define VENDOR_3DFX             0x121a
#define DEVICE_VOODOO45         0x0009
#define DEVICE_VOODOO3          0x0005
#define DEVICE_RV280PRO         0x5960
#define DEVICE_RV280            0x5961
#define DEVICE_RV280_2          0x5962
#define DEVICE_RV280MOB         0x5C63
#define DEVICE_RV280SE          0x5964

#define OFFSET_KERNEL           0x3000
#define OFFSET_ZEROPAGE         0x10000
#define OFFSET_MESSAGES         0x200000
#define OFFSET_SYSMEM           0x400000
#define OFFSET_PCIMEM           0x60000000
#define VECTOR_TABLE_DEFAULT    0xfff00000

#define MMU_PAGESIZE            0x1000

// MSR settings

#define	   PSL_VEC	  0x02000000  /* ..6. AltiVec vector unit available */
#define	   PSL_SPV	  0x02000000  /* B... (e500) SPE enable */
#define	   PSL_UCLE	  0x00400000  /* B... user-mode cache lock enable */
#define	   PSL_POW	  0x00040000  /* ..6. power management */
#define	   PSL_WE	  PSL_POW	  /* B4.. wait state enable */
#define	   PSL_TGPR	  0x00020000  /* ..6. temp. gpr remapping (mpc603e) */
#define	   PSL_CE	  PSL_TGPR	  /* B4.. critical interrupt enable */
#define	   PSL_ILE	  0x00010000  /* ..6. interrupt endian mode (1 == le) */
#define	   PSL_EE	  0x00008000  /* B468 external interrupt enable */
#define	   PSL_PR	  0x00004000  /* B468 privilege mode (1 == user) */
#define	   PSL_FP	  0x00002000  /* B.6. floating point enable */
#define	   PSL_ME	  0x00001000  /* B468 machine check enable */
#define	   PSL_FE0	  0x00000800  /* B.6. floating point mode 0 */
#define	   PSL_SE	  0x00000400  /* ..6. single-step trace enable */
#define	   PSL_DWE	  PSL_SE	  /* .4.. debug wait enable */
#define	   PSL_UBLE	  PSL_SE	  /* B... user BTB lock enable */
#define	   PSL_BE	  0x00000200  /* ..6. branch trace enable */
#define	   PSL_DE	  PSL_BE	  /* B4.. debug interrupt enable */
#define	   PSL_FE1	  0x00000100  /* B.6. floating point mode 1 */
#define	   PSL_IP	  0x00000040  /* ..6. interrupt prefix */
#define	   PSL_IR	  0x00000020  /* .468 instruction address relocation */
#define	   PSL_IS	  PSL_IR	  /* B... instruction address space */
#define	   PSL_DR	  0x00000010  /* .468 data address relocation */
#define	   PSL_DS	  PSL_DR	  /* B... data address space */
#define	   PSL_PM	  0x00000008  /* ..6. Performance monitor */
#define	   PSL_PMM	  PSL_PM	  /* B... Performance monitor */
#define	   PSL_RI	  0x00000002  /* ..6. recoverable interrupt */
#define	   PSL_LE	  0x00000001  /* ..6. endian mode (1 == le) */

// general BAT defines for bit settings to compose BAT regs
// represent all the different block lengths
// The BL field	is part of the Upper Bat Register

#define BAT_BL_128K      		0x00000000
#define BAT_BL_256K	      		0x00000004
#define BAT_BL_512K		      	0x0000000C
#define BAT_BL_1M		      	0x0000001C
#define BAT_BL_2M		      	0x0000003C
#define BAT_BL_4M		      	0x0000007C
#define BAT_BL_8M		      	0x000000FC
#define BAT_BL_16M		      	0x000001FC
#define BAT_BL_32M		      	0x000003FC
#define BAT_BL_64M		      	0x000007FC
#define BAT_BL_128M		      	0x00000FFC
#define BAT_BL_256M		      	0x00001FFC

// supervisor/user valid mode definitions  - Upper BAT
#define BAT_VALID_SUPERVISOR    0x00000002
#define BAT_VALID_USER		    0x00000001
#define BAT_INVALID		        0x00000000

// WIMG bit settings  - Lower BAT
#define BAT_WRITE_THROUGH	    0x00000040
#define BAT_CACHE_INHIBITED	    0x00000020
#define BAT_COHERENT	        0x00000010
#define BAT_GUARDED		        0x00000008

// General PTE bits
#define PTE_REFERENCED		    0x00000100
#define PTE_CHANGED		        0x00000080
#define PTE_HASHID		        0x00000040
#define PTE_VALID               0x80000000

// PageTable Access bits
#define PP_USER_RW              2
#define PP_SUPERVISOR_RW        0

// WIMG bit settings  - Lower PTE
#define PTE_WRITE_THROUGH  		0x00000008
#define PTE_CACHE_INHIBITED  	0x00000004
#define PTE_COHERENT      		0x00000002
#define PTE_GUARDED		      	0x00000001
#define PTE_COPYBACK	      	0x00000000

// Protection bits - Lower BAT
#define BAT_NO_ACCESS      		0x00000000
#define BAT_READ_ONLY	      	0x00000001
#define BAT_READ_WRITE		  	0x00000002

// IMMR offsets
#define IMMR_ADDR_DEFAULT       0xFF400000
#define IMMR_IMMRBAR            0x0

#define IMMR_RSR                0x910
#define IMMR_RPR                0x918
#define IMMR_RCR                0x91C
#define IMMR_RCER               0x920

#define IMMR_PCILAWBAR0         0x60
#define IMMR_PCILAWAR0          0x64
#define IMMR_PCILAWBAR1         0x68
#define IMMR_PCILAWAR1          0x6C

#define IMMR_IMR0               0x8050

#define IMMR_PITAR0             0x8568
#define IMMR_PIBAR0             0x8570
#define IMMR_PIWAR0             0x8578

#define IMMR_POTAR0             0x8400
#define IMMR_POTAR1             0x8418
#define IMMR_POTAR2             0x8430
#define IMMR_POBAR0             0x8408
#define IMMR_POBAR1             0x8420
#define IMMR_POBAR2             0x8438

#define IMMR_POCMR0             0x8410
#define IMMR_POCMR1             0x8428
#define IMMR_POCMR2             0x8440
#define IMMR_POCMR3             0x8458
#define IMMR_POCMR4             0x8470
#define IMMR_POCMR5             0x8488

#define PIWAR_EN                0x80000000
#define PIWAR_PF                0x20000000
#define PIWAR_RTT_SNOOP         0x00060000
#define PIWAR_WTT_SNOOP         0x00006000
#define PIWAR_IWS_64MB          0x00000019

#define POCMR_EN                0x80000000
#define POCMR_CM_256MB          0x000F0000
#define POCMR_CM_128MB          0x000F8000
#define POCMR_CM_64MB           0x000FC000
#define POCMR_CM_64KB           0x000FFFF0

#define LAWAR_EN                0x80000000
#define LAWAR_64MB              0x00000019
#define LAWAR_128MB             0x0000001a
#define LAWAR_256MB             0x0000001b
#define LAWAR_512MB             0x0000001c

#define IMMR_SIMSR_L            0x724
#define SIMSR_L_MU              0x04000000


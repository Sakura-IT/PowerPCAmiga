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

.macro loadreg register, value
	.if 	(\value >= -0x8000) && (\value <= 0x7fff)
        	li      \register, \value
	.else
		lis     \register, \value@h
		ori     \register, \register, \value@l
	.endif
.endm

.set HID0,                      1008
.set HID0_NHR,			        0x00010000

.set PSL_FP,	                0x00002000        #/* B.6. floating point enable */
.set PSL_IP,	                0x00000040	      #/* ..6. interrupt prefix */

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

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

.global     _setupPPC

.section "setupppc","acrx"

            bl	.SkipCom		#0x3000	System initialization

.long		0					#Used for initial communication
.long		0					#MemStart
.long		0					#MemLen
.long		0					#RTGBase
.long		0					#RTGLen
.long		0					#RTGType
.long		0					#RTGConfig
.long		0					#Options1
.long		0					#Options2
.long		0					#Options3
.long		0					#XMPI Address
.long		0					#StartBAT
.long		0					#SizeBAT

.SkipCom:   mflr	r3
            subi    r3,r3,4
            b   _setupPPC


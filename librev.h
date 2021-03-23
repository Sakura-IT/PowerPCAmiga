// Copyright (c) 2019-2021 Dennis van der Boon
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

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/initializers.h>

#define VERSION     17
#define REVISION    14

#define LIBNAME "powerpc.library"
#define VSTRING "$VER: powerpc.library 17.14 beta (23.03.21)\r\n"

#define VERPCI      13
#define REVPCI      8

#define VERPCIP     4
#define REVPCIP     0

#define LIB_PRIORITY 0

const UWORD LibInitData[] = {
INITBYTE(8, NT_LIBRARY),
INITBYTE(14, LIBF_SUMMING|LIBF_CHANGED),
INITWORD(20, VERSION),
INITWORD(22, REVISION),
(ULONG) 0};

const UWORD WarpInitData[] = {
INITBYTE(8, NT_LIBRARY),
INITBYTE(14, LIBF_SUMMING|LIBF_CHANGED),
INITWORD(20, 5),
INITWORD(22, 1),
(ULONG) 0};


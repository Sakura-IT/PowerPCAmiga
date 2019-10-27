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

#define PPCEXCEPTION __section ("kernel","acrx") __entry __interrupt

PPCEXCEPTION void ExcFPUnav(void)
{
    return;
}

PPCEXCEPTION void ExcAlignment(void)
{
    return;
}

PPCEXCEPTION void ExcInsStor(void)
{
    return;
}

PPCEXCEPTION void ExcDatStor(void)
{
    return;
}

PPCEXCEPTION void ExcTrace(void)
{
    return;
}

PPCEXCEPTION void ExcBreakPoint(void)
{
    return;
}

PPCEXCEPTION void ExcDec(void)
{
    return;
}

PPCEXCEPTION void ExcPrIvate(void)
{
    return;
}

PPCEXCEPTION void ExcMachCheck(void)
{
    return;
}

PPCEXCEPTION void ExcSysCall(void)
{
    return;
}

PPCEXCEPTION void ExcPerfMon(void)
{
    return;
}

PPCEXCEPTION void ExcSysMan(void)
{
    return;
}

PPCEXCEPTION void ExcTherMan(void)
{
    return;
}

PPCEXCEPTION void ExcVMXUnav(void)
{
    return;
}

PPCEXCEPTION void ExcITLBMiss(void)
{
    return;
}

PPCEXCEPTION void ExcDLoadTLBMiss(void)
{
    return;
}

PPCEXCEPTION void ExcDStoreTLBMiss(void)
{
    return;
}

PPCEXCEPTION void ExcExternal(void)
{
    return;
}

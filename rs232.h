/*
    Cross-platform serial / RS232 library
    Version 0.1, 16/06/2015
    -> All platforms header
    -> rs232.h
    The MIT License (MIT)

    Copyright (c) 2007 - 2015 Frédéric Meslin
    Contact: fredericmeslin@hotmail.com, @marzacdev

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#ifndef RS232_H
#define RS232_H

    #include <stdint.h>
    #include <stdlib.h>

/*****************************************************************************/
    int comEnumerate();
    int comGetNoPorts();

    const char * comGetPortName(int index);
    int comFindPort(const char * name);
    const char * comGetInternalName(int index);

/*****************************************************************************/
    int  comOpen(int index, int baudrate);
    void comClose(int index);
    int  comWrite(int index, const char * buffer, size_t len);
    int  comRead(int index, char * buffer, size_t len);

#endif // RS232_H

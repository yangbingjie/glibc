/* Copyright (C) 1991-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Torbjorn Granlund (tege@sics.se),
   with help from Dan Sahlin (dan@sics.se);
   commentary by Jim Blandy (jimb@ai.mit.edu).

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <string.h>
#include <stdlib.h>

#undef strlen // #undef是预处理器标识符，表示取消strlen的定义

#ifndef STRLEN
# define STRLEN strlen
#endif

/* Return the length of the null-terminated string STR.  Scan for
   the null terminator quickly by testing four bytes at a time.  */
// 返回非空字符串的长度，通过一次检查4个字节来快速找到空字符

// size_t 是为了解决移植性问题，它是sizeof运算符结果的类型size_t = typeof(sizeof(X))，不同平台的size_t会用不同的类型实现。
// size_t定义在<stddef.h>, <stdio.h>, <stdlib.h>, <string.h>, <time.h>和<wchar.h>这些标准C头文件中，这里的size_t定义在 glibc/string/string.h里
size_t 
STRLEN (const char *str)
{
  const char *char_ptr; // 遍历的指针
  const unsigned long int *longword_ptr; // 字符串较长时遍历的指针
  unsigned long int longword, himagic, lomagic;

  /* Handle the first few characters by reading one character at a time.
     Do this until CHAR_PTR is aligned on a longword boundary.  */
  // sizeof (longword) - 1) = 7， 二进制为00...0111
  // 起始地址有可能不是字节对齐的，先处理这些字符，让 chart_ptr 按照 sizeof (unsigned long) 字节大小进行位对齐
  // 刚开始的遍历每次检查一个字符是不是空字符，直到char_ptr指向的地址末尾有3个0
  // 循环的次数每次运行都是不固定的，取决于分配的内存地址
  // 可以预见的是，当数组长度大于7时，一定会跳出循环
  for (char_ptr = str; ((unsigned long int) char_ptr
			& (sizeof (longword) - 1)) != 0;
       ++char_ptr)
    if (*char_ptr == '\0') // 如果遇到空字符，则返回字符串长度
      return char_ptr - str;

  /* All these elucidatory comments refer to 4-byte longwords,
     but the theory applies equally well to 8-byte longwords.  */
  longword_ptr = (unsigned long int *) char_ptr;

  /* Bits 31, 24, 16, and 8 of this number are zero.  Call these bits
     the "holes."  Note that there is a hole just to the left of
     each byte, with an extra at the end:

     bits:  01111110 11111110 11111110 11111111
     bytes: AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD

     The 1-bits make sure that carries propagate to the next 0-bit.
     The 0-bits provide holes for carries to fall into.  */
  himagic = 0x80808080L; // 10000000 10000000 100000000 10000000
  lomagic = 0x01010101L; // 00000001 00000001 000000001 00000001
  if (sizeof (longword) > 4)
    {
      /* 64-bit version of the magic.  */
      /* Do the shift in two steps to avoid a warning if long has 32 bits.  */
      // 10000000 10000000 100000000 10000000 10000000 10000000 100000000 10000000
      himagic = ((himagic << 16) << 16) | himagic;
      // 00000001 00000001 000000001 00000001 00000001 00000001 000000001 00000001
      lomagic = ((lomagic << 16) << 16) | lomagic;
    }
  if (sizeof (longword) > 8)
    abort ();

  /* Instead of the traditional loop which tests each character,
     we will test a longword at a time.  The tricky part is testing
     if *any of the four* bytes in the longword in question are zero.  */
  // 每次检查所讨论的longword中八个字节里是否有0
  for (;;)
    {
      printf("longword_ptr before %#x\n", longword_ptr);
      longword = *longword_ptr++; // 先取地址再++，因为结合方向为右到左。longword：当前待检查的四个字节，即字符串中的四个字符
      printf("longword_ptr after %#x\n", longword_ptr);
      printf("Longword %#x\n", longword, longword);

      // & himagic 表示只关注每个字节的最高位
      // longword分三种情况
      // 1. longword = 1XXXXXXX，则 ~longword = 0YYYYYYY
      //    也就是 ~longword & himagic = 00000000 if条件恒为false
      // 2. longword = 0XXXXXXX 且不等于0，longword - lomagic = 0ZZZZZZZ >= 0
      //     (longword - lomagic) & himagic = 0ZZZZZZZ & 10000000 = 00000000 if条件恒为false
      // 3. longword = 00000000，~longword & himagic = 10000000
      //     longword - lomagic = 00000000 - 10000000
      //     无符号数减法需要用到补码（正数的补码不变，负数的补码是对应正数的反码再加1）
      //     longword - lomagic = 00000000 - 10000000 = 补码 00000000 + 补码(-10000000)
      //                        = 00000000 + (~10000000 + 1) = 00000000 + (01111111 + 1)
      //                        = 00000000 + 10000000 = 10000000
      //     因此 (longword - lomagic) & ~longword & himagic = 10000000 & 11111111 & 10000000
      //                                                     = 10000000 > 0 if条件为true
      
      // 同时判断longword的四个字节，可以加快速度
      // 如果四个字节都为0，说明此时longword里已经没有任何字符了
      if (((longword - lomagic) & ~longword & himagic) != 0)
   {
    /* Which of the bytes was the zero?  If none of them were, it was
       a misfire; continue the search.  */

    // longword_ptr - 1：前面longword_ptr++，这里找\0的时候再-1
	  const char *cp = (const char *) (longword_ptr - 1);

	  if (cp[0] == 0)
	    return cp - str;
	  if (cp[1] == 0)
	    return cp - str + 1;
	  if (cp[2] == 0)
	    return cp - str + 2;
	  if (cp[3] == 0)
	    return cp - str + 3;
	  if (sizeof (longword) > 4)
	    {
	      if (cp[4] == 0)
		return cp - str + 4;
	      if (cp[5] == 0)
		return cp - str + 5;
	      if (cp[6] == 0)
		return cp - str + 6;
	      if (cp[7] == 0)
		return cp - str + 7;
	    }
	}
    }
}
libc_hidden_builtin_def (strlen) // 基于 strlen 符号, 重新定义一个符号别名 __GI_strlen. 

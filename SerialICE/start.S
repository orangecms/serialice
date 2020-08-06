/*
 * SerialICE
 *
 * Copyright (C) 2009 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.
 */

#include "serialice.h"

.align 4096
.section ".text"
.code16
.type	_start16, @function
_start16:
	cli
	movl	%eax, %ebp

	movb	$0x1d, %al
	outb	%al, $0x80

	xorl	%eax, %eax
	movl	%eax, %cr3
	movw	%cs, %ax
	shlw	$4, %ax

	movw	%cs, %ax
	shlw	$4, %ax
	movw	$idt16_off, %bx
	subw	%ax, %bx
	lidt	%cs:(%bx)
	movw	$gdt16ptr_off, %bx
	subw	%ax, %bx
	lgdtl	%cs:(%bx)

	movl	%cr0, %eax
	andl	$0x7ffaffd1, %eax
	orl	$0x60000001, %eax
	movl	%eax, %cr0

	movl	%ebp, %eax
	ljmpl	$0x08, $_start32

.align 4
.code32
.type	_start32, @function
_start32:
	movl	%eax, %ebp

	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs

	movb	$0x1e, %al
	outb	%al, $0x80

	movl	%ebp, %eax
	movl	$0x2000, %esp

	jmp	main

.align 4
.globl	idt16
idt16:
	.word	0
	.long	0
	.word	0

.globl	gdt16ptr
gdt16ptr:
	.word	gdt16_end - gdt16 - 1
	.long	gdt16

.align 4
gdt16:
	.word	0x0000, 0x0000
	.byte	0x00, 0x00, 0x00, 0x00

	.word	0xffff, 0x0000
	.byte	0x00, 0x9b, 0xcf, 0x00

	.word	0xffff, 0x0000
	.byte	0x00, 0x93, 0xcf, 0x00
gdt16_end:

.section ".reset", "ax", %progbits
.code16
.globl	_reset
.type	_reset, @function
_reset:
	.byte	0xe9
	.int 	_start16 - ( . + 2 )
	.byte 0,0,0,0,0,0,0,0,0,0,0

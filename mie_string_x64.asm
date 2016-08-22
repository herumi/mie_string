;
; for Win64
; nasm -f win64 -D_WIN64 mie_string_x86.asm
; for Linux 64
; nasm -f elf64 mie_string_x86.asm
;
global mie_findCharRangeSSE
global mie_findCharRangeAVX
global mie_findCharAnySSE
global mie_findCharAnyAVX

default rel

segment .data
align 32
shiftPtn:
db 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
db 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80

%define AVX 1
%define SSE 2

%macro _movdqu 3
	%if %1 == AVX
		vmovdqu %2, %3
	%else
		movdqu %2, %3
	%endif
%endmacro

%macro _movdqa 3
	%if %1 == AVX
		vmovdqa %2, %3
	%else
		movdqa %2, %3
	%endif
%endmacro

%macro _pcmpestri 4
	%if %1 == AVX
		vpcmpestri %2, %3, %4
	%else
		pcmpestri %2, %3, %4
	%endif
%endmacro

%macro _pshufb 3
	%if %1 == AVX
		vpshufb %2, %3
	%else
		pshufb %2, %3
	%endif
%endmacro

; findChar(text, textSize, key, keySize);
; Linux     rdi     rsi    rdx    rcx
; Win       rcx     rdx    r8     r9
; pcmpestri _text   rdx           rax
;                 _textSize

%ifdef _WIN64
	%define _text r9
	%define _textSize r8
%else
	%define _text rdi
	%define _textSize rsi
%endif

%macro findCharGeneric 2
%ifdef _WIN64
	_movdqu %1, xmm1, [r8]
	mov eax, r9d
	mov r8, rdx
	mov r9, rcx
%else
	_movdqu %1, xmm1, [rdx]
	mov	eax, ecx
	mov rdx, rsi
%endif
	cmp rdx, 16
	jb .size_lt_16
	and rdx, ~15
.lp:
	_pcmpestri %1, xmm1, [_text], %2
	cmp ecx, 16
	jne .found
	add _text, 16
	sub rdx, 16
	jnz .lp

	mov rdx, _textSize
	and edx, 15

.size_lt_16:
	test edx, edx
	je .notfound
	mov	rcx, _text
	and	ecx, 4095
	cmp	ecx, 4080
	jbe	.load
	add	ecx, edx
	cmp	ecx, 4096
	ja	.load
	mov	rcx, _text
	and	rcx, -16
	_movdqa %1, xmm0, [rcx]
	mov rcx, _text
	and ecx, 15
	mov _textSize, shiftPtn
	_movdqu %1, xmm2, [_textSize + rcx]
	_pshufb %1, xmm0, xmm2
	jmp .last
.load:
	_movdqu %1, xmm0, [_text]
.last:
	_pcmpestri %1, xmm1, xmm0, %2
	jnc .notfound
.found:
	lea rax, [_text + rcx]
	ret
.notfound:
	xor eax, eax
	ret
%endmacro

segment .text
mie_findCharAnyAVX:
	findCharGeneric AVX, 0

mie_findCharRangeAVX:
	findCharGeneric AVX, 4

mie_findCharAnySSE:
	findCharGeneric SSE, 0

mie_findCharRangeSSE:
	findCharGeneric SSE, 4

global mie_findCharRangeSSE
global mie_findCharRangeAVX
global mie_findCharAnySSE
global mie_findCharAnyAVX

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

%macro findCharGeneric 2
	_movdqu %1, xmm1, [rdx]
	mov	eax, ecx
	mov rdx, rsi
	cmp rdx, 16
	jb .size_lt_16
	and rdx, ~15
.lp:
	_pcmpestri %1, xmm1, [rdi], %2
	cmp ecx, 16
	jne .found
	add rdi, 16
	sub rdx, 16
	jnz .lp

	mov edx, esi
	and edx, 15

.size_lt_16:
	test edx, edx
	je .notfound
	mov	ecx, edi
	and	ecx, 4095
	cmp	ecx, 4080
	jbe	.load
	add	ecx, edx
	cmp	ecx, 4096
	ja	.load
	mov	rcx, rdi
	and	rcx, -16
	_movdqa %1, xmm0, [rcx]
	mov ecx, edi
	and ecx, 15
	_movdqu %1, xmm2, [shiftPtn + rcx]
	_pshufb %1, xmm0, xmm2
	jmp .last
.load:
	_movdqu %1, xmm0, [rdi]
.last:
	_pcmpestri %1, xmm1, xmm0, %2
	jnc .notfound
.found:
	lea rax, [rdi + rcx]
	ret
.notfound:
	xor eax, eax
	ret
%endmacro

%macro findCharGeneric2 2
	_movdqu %1, xmm1, [rdx]
	mov	eax, ecx
	mov rdx, rsi
	jmp .L0
	align 16
.next:
	add	rdi, 16
	sub	rdx, 16
.L0:
	cmp	rdx, 16
	jb .size_lt_16
.loop:
	_movdqu %1, xmm0, [rdi]
.L1:
	_pcmpestri %1, xmm1, xmm0, %2
	ja .next
	jnc .notfound
	lea rax, [rdi + rcx]
	ret
.notfound:
	xor	eax, eax
	ret
.size_lt_16:
	test rdx, rdx
	je	.notfound
	mov	rcx, rdi
	and	ecx, 4095
	cmp	ecx, 4080
	jbe	.loop
	add	ecx, edx
	cmp	ecx, 4096
	ja	.loop
	mov	rcx, rdi
	and	rcx, -16
	_movdqa %1, xmm0, [rcx]
	mov	ecx, edi
	and	ecx, 15
	_movdqu %1, xmm2, [shiftPtn + rcx]
	_pshufb %1, xmm0, xmm2
	jmp .L1
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

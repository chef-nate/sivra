movups xmm0, [rdi]
movups xmm1, [rsi]
subps xmm0, xmm1
mulps xmm0, xmm0
maxps xmm0, [rcx]
mulps xmm0, [rdx]

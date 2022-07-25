
# input 
#   @param1: context_buffer 
# rbx rsp rbp r12 r13 r14 r15 pc
#if defined(__amd64__) || defined(__x86_64__)
    .text 
    .align 4
    .globl save_context
    .type save_context, @function
save_context:
    pop  %rsi # pc
    movq %rbx, (%rdi)
    movq %rsp, 8(%rdi) # restore sp
    push %rsi
    movq %rbp, 16(%rdi)
    movq %r12, 24(%rdi)
    movq %r13, 32(%rdi)
    movq %r14, 40(%rdi)
    movq %r15, 48(%rdi)
    movq %rsi, 56(%rdi)
    xorl %eax, %eax
    ret 
    
    .size save_context,.-save_context


# input 
#   @param1: context_buffer 
#   @param2: retcode, default 1
    .text
    .align 4 
    .global restore_context
    .type restore_context,@function
restore_context:
    movq (%rdi), %rbx
    movq 8(%rdi), %rsp 
    movq 16(%rdi), %rbp
    movq 24(%rdi), %r12 
    movq 32(%rdi), %r13 
    movq 40(%rdi), %r14 
    movq 48(%rdi), %r15
    movl %esi, %eax 
    jmp *56(%rdi)
    .size restore_context,.-restore_context

# input 
#   @param1: context_buffer 
#   @param2: stack address
    .text 
    .align 4 
    .global replace_stack
    .type replace_stack,@function 
replace_stack:
    movq %rsi, 8(%rdi)
    ret 
    .size replace_stack,.-replace_stack
#else 
    #error architecture not support yet
#endif


# Pez - a stack machine

I have played with stack machines [here](https://github.com/joeldbaptista/eel).
In this project we implement a stack machine in just C, for the fun of it. I have
also used other references, such as "Crafting Interpreters" by R. Nystrom, which
implements a stack machine in the second part of the book.

## Scope

Pez is a toy: the target is a machine simple enough to hand-write and run
programs like iterative Fibonacci, nothing more.

- No addressable memory. Registers and the stack are the only places data
  lives.
- No I/O during execution. The VM writes the final register/stack state to
  `out`; results are read out of a register.
- Function calls via `CALL`/`RET` share the same stack `PUSH`/`POP`/
  arithmetic use for return addresses — there is no separate call
  stack. This means an unbalanced `PUSH`/`POP` inside a called routine
  can corrupt its own return address; neither the assembler nor the VM
  detects this.

## Components of the project

1. **Assembler** The assembler converts the stack machine source code into binary
data, that can be executed by the stack machine itself. The assembly files have
the extention `pez`. 

2. **Virtual machine** The virtual machine that executes the binary. To run the
compiled version, do: `pez -i program -o out`, where `program` is the stack machine
program, and `out` is a file with final state of the machine. 

## Structures

0. Word size: 64 bits
1. Registers: r0, r1, r2, ..., r15, plus others of utilities
2. Stack

## Operations

1. PUSH r - push register r onto the stack
2. POP r - pop the top of the stack into register r
3. PUSHI imm - push an immediate constant onto the stack
4. JMP label - unconditional jump
5. CALL label - push return address, jump to label
6. RET - pop return address, jump there
7. CMP - pop a, pop b, push (a - b)
8. JIEZ label - pop; jump if == 0
9. JIGZ label - pop; jump if > 0
10. JILZ label - pop; jump if < 0
11. JGEZ label - pop; jump if >= 0
12. JLEZ label - pop; jump if <= 0
13. ADD - pop a, pop b, push (a + b)
14. SUB - pop a, pop b, push (a - b)
15. MUL - pop a, pop b, push (a * b)
16. DIV - pop a, pop b, push (a / b)
17. REM - pop a, pop b, push (a % b)
18. HALT - stop execution

## Example: Fibonacci

Computes fib(10) into r0.

```
main:
    PUSHI 10
    POP  r2    ; r2 = N
    PUSHI 0
    POP  r0    ; r0 = fib(i)
    PUSHI 1
    POP  r1    ; r1 = fib(i+1)
loop:
    PUSH r2
    PUSHI 0
    CMP
    JIEZ end   ; if r2 == 0, done
    PUSH r0
    PUSH r1
    ADD
    POP  r3    ; r3 = r0 + r1
    PUSH r1
    POP  r0    ; r0 = r1
    PUSH r3
    POP  r1    ; r1 = r3
    PUSHI 1
    PUSH r2
    SUB
    POP  r2    ; r2 = r2 - 1
    JMP  loop
end:
    HALT
```

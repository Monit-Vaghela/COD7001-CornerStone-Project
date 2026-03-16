# To be Decided
1. Compiler Stages
2. Json File for behaviour of circuit (defined)
3. Circuit Builder flow (Done)
4. Simulation Flow (Done)


## Things to do

## Monit
- Make a single class for gates which inherits process
- Make 2 simulators (sequential and parallel)
- add Nand Nor
- fix dflipflop with compiler 

## Anunay
- json files to be created in the same folder as input.[overwrite if already exists]
- add Nand, Nor


# Parser Testing
1. Incomplete quotations, brackets, parenthesis.
    1.1 : missing " [passed]
    1.2 : '' [does not parse, fine desig choice] [passed]
    1.3 : missing [] [passed]
    1.4 : missing {} [passsed]
    1.5 : missing ; [passed]

2. missing keyword [passed]
3. not following grammer {} instead of [] [passed]
4. entity name not matching for architecture and simulation
    4.1 : architecture name not matching [passed]
    4.2 : simulator name not matching [passed]
5. architecture or simulation missing
    5.1 : architecture missing [passed]
    5.2 : simulator missing [passed]
6. outputs missing, inputs missing
    6.1 : missing outputs, inputs [passed]
    6.2 : empty inputs, outputs [passed]
7. signal mismatching
    7.1 : missing mapping [passed]
    7.2 : many to one mapping ["x" < = "a" and "x" <= "b"] [passed]
    7.3 : mapped to a signal that does not exist [passed]
8. behaviour
    8.1 : no behaviour [passed]
    8.2 : empty behaviour [passed]
9. behaviour expression issues
    9.1 : single input in or, and, xor, xnor [passed]
    9.2 : Checking intermediate generation [to be tested]
    9.3 : behaviour expression must be mapped to a output signal only [passed]
    9.4 : many to one mapping on a single output is not possinble [passed]
    9.5 : every output must be mapped [passed]
10. process must only define initialization of input mapped signals only [passed]
11. signals value can only be 0 or 1 [failed]
12. timestamp can only be >= 1. [failed]
13. used port in behaviour or simulator
    13.1 : in behaviour [passed]
    13.2 : in simulator [passed]
14. checking expression precedence [passed]
# Bugs Encounterd
| Sr. No | Where | Breif description to recreate | Fixed |
|--------|---------|--------------|-------|
|  1  |   all      |    dflipflop does not work with compiler logic          |       |


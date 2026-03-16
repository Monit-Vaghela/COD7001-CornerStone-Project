# To be Decided
1. Compiler Stages
2. Json File for behaviour of circuit (defined)
3. Circuit Builder flow (Done)
4. Simulation Flow (Done)


## Things to do

## Monit
- Make a single class for gates which inherits process [Done]
- Make 2 simulators (sequential and parallel) [Done]
- add Nand Nor [Done]
- fix dflipflop with compiler 
- add new parallel logic

## Anunay
- json files to be created in the same folder as input.[overwrite if already exists]
- add Nand, Nor


# Parser Testing
1. Incomplete quotations, brackets, parenthesis.
    * missing " [passed]
    * '' [does not parse, fine desig choice] [passed]
    * : missing [] [passed]
    * : missing {} [passsed]
    * : missing ; [passed]

2. missing keyword [passed]
3. not following grammer {} instead of [] [passed]
4. entity name not matching for architecture and simulation
    *  architecture name not matching [passed]
    *  simulator name not matching [passed]
5. architecture or simulation missing
    * architecture missing [passed]
    * simulator missing [passed]
6. outputs missing, inputs missing
    * missing outputs, inputs [passed]
    * empty inputs, outputs [passed]
7. signal mismatching
    * missing mapping [passed]
    * many to one mapping ["x" < = "a" and "x" <= "b"] [passed]
    * mapped to a signal that does not exist [passed]
8. behaviour
    * no behaviour [passed]
    * empty behaviour [passed]
9. behaviour expression issues
    * single input in or, and, xor, xnor [passed]
    * Checking intermediate generation [to be tested]
    * behaviour expression must be mapped to a output signal only [passed]
    * many to one mapping on a single output is not possinble [passed]
    * every output must be mapped [passed]
10. process must only define initialization of input mapped signals only [passed]
11. signals value can only be 0 or 1 [failed]
12. timestamp can only be >= 1. [failed]
13. used port in behaviour or simulator
    * in behaviour [passed]
    * in simulator [passed]
14. checking expression precedence [passed]
# Bugs Encounterd
| Sr. No | Where | Breif description to recreate | Fixed |
|--------|---------|--------------|-------|
|  1  |   all      |    dflipflop does not work with compiler logic          |       |


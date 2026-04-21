# To be Decided
1. Change simulator to incorporate combinational circuits properly
2. add enitity Embedding in compiler stage
3. parallelization.


## Things to do

## Monit
- Convert logic of behaviour process to code
- write test cases for 
    1) dflipflop
    2) srLatch
    3) 4-bit adder
    4) register file (64 reg - 8 bit each)
- make dashboard.
- make ppt and video

## Anunay
- Clean up of code (remove comments which seems AI like and is not mentioned in the report to be AI generated)
- makeFile : add this
```Makefile
g++ -O2 -std=c++17 src/comparator.cpp -o comparator
```
- make a proper sh file
- it must run like this
```bash
run.sh <cirFile>.cir <tesFile>.tes
```
- now what should it do

```bash
./compiler <cirFile>.cir <tesFile>.tes
./circuitBuilder <cirFile>_cir.json <tesFile>_tes.json
./comparator output/tracerSeq.bin output/tracerPar.bin
cd dashboard
python3 -m http.server 8080
```


- make a proper readme explaining
```md
1. folder structure
2. how to run multiple components
* how to compiler
* how to use circuitBuilder
* how to run comparator
* how to use dashboard and what it represents
```

### changes needed in dash board.
- files it uses 
1. `include/circuitBuilder/utils/DashBoardJson.h`
2. `src/circuitBuilder/utils/DashBoardJson.h`
3. `src/simulator/seq/sequential.cpp`
4. `src/simulator/par/v2.cpp`
5. `dashboard/dashboard.html`
- how to run dashboard
```bash
cd dashboard
python3 -m http.server 8080
```
- click on the link and open dashborad.html

- the 4 graph represent that how much threads are utilized but it is weird beacause it takes a threshold and checks if we have more than that then it is 100% utilized
- we need to remove it, currently the seq and parallel version stores the time take in executing a single delta cycle, which is being used in speed up, now we will break this time into 2 times
    * the time taken to execute in parallel
    * the time take to syncronize

- the 3rd graph will show the speed up on the total time to execute a single delta cycle 
- but the 4th graph will show the difference caused due to overhead caused due to synchronization.
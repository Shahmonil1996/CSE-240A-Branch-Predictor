# CSE-240A-Branch-Predictor

The purpose of this project is to simulate multiple Branch Predictors and calculate the mispredction rates for given traces.

We have implemented the following Predictors : 
1. GSHARE (with 2 bit Counters)
2. TOURNAMENT (with all 2 bit Counters)
3. Tour-SHARE (i.e. Replaced Global Predictor in Tournament with GSHARE to improve the midprediction rates and 3 bit local Counter) 
4. Local Predictor / Yeh-Patt (Local Pattern Table + Local Counter from Tournament predictor)
5. Perceptron Predictor (https://www.cs.utexas.edu/~lin/papers/hpca01.pdf)
6. TAGE Predictor (part from https://www.irisa.fr/caps/people/seznec/L-TAGE.pdf and part from https://jilp.org/vol7/v7paper10.pdf)


Usage : (from src directory - run "make" command first in the src directory to compile and generate executable)
bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --custom:0
bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --custom:0
bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --custom:0
bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --custom:0
bunzip2 -kc ../traces/int_1.bz2 | ./predictor --custom:0
bunzip2 -kc ../traces/int_2.bz2 | ./predictor --custom:0

bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --tournament:9:10:10
bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --tournament:9:10:10
bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --tournament:9:10:10
bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --tournament:9:10:10
bunzip2 -kc ../traces/int_1.bz2 | ./predictor --tournament:9:10:10
bunzip2 -kc ../traces/int_2.bz2 | ./predictor --tournament:9:10:10

bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --gshare:15
bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --gshare:15
bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --gshare:15
bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --gshare:15
bunzip2 -kc ../traces/int_1.bz2 | ./predictor --gshare:15
bunzip2 -kc ../traces/int_2.bz2 | ./predictor --gshare:15

More References : 
https://www.cs.cmu.edu/afs/cs/academic/class/15740-s17/www/lectures/L19-BranchPrediction.pdf
https://jilp.org/vol8/v8paper1.pdf
https://jilp.org/vol7/v7paper10.pdf
https://www.irisa.fr/caps/people/seznec/L-TAGE.pdf
https://jilp.org/cbp/traces-with-values/

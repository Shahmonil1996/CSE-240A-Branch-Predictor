# CSE-240A-Branch-Predictor

The purpose of this project is to simulate multiple Branch Predictors and calculate the mispredction rates for given traces.

We have implemented the following Predictors : 
1. GSHARE (with 2 bit Counters)
2. TOURNAMENT (with all 2 bit Counters)
3. Tour-SHARE (i.e. Replaced Global Predictor in Tournament with GSHARE to improve the midprediction rates and 3 bit local Counter) 
4. Local Predictor / Yeh-Patt (Local Pattern Table + Local Counter from Tournament predictor)
5. Perceptron Predictor (https://www.cs.utexas.edu/~lin/papers/hpca01.pdf)
6. TAGE Predictor (part from https://www.irisa.fr/caps/people/seznec/L-TAGE.pdf and part from https://jilp.org/vol7/v7paper10.pdf)
*Note - Tage Predictor is not completely Explained almost anywhere, so the implementation might be different from others...


Usage : (run "make" command first in the src directory to compile and generate executable, next run any predictor using the example below)
# To run CUSTOM TourSHARE PREDICTOR 
(To run Local predictor use custom:1, to run perceptron predictor use custom:2, to run Tage predictor use custom:3)

bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --custom:0

bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --custom:0

bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --custom:0

bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --custom:0

bunzip2 -kc ../traces/int_1.bz2 | ./predictor --custom:0

bunzip2 -kc ../traces/int_2.bz2 | ./predictor --custom:0
# To run Tournament Predictor

bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --tournament:9:10:10

bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --tournament:9:10:10

bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --tournament:9:10:10

bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --tournament:9:10:10

bunzip2 -kc ../traces/int_1.bz2 | ./predictor --tournament:9:10:10

bunzip2 -kc ../traces/int_2.bz2 | ./predictor --tournament:9:10:10

# To run GSHARE Predictor
bunzip2 -kc ../traces/fp_1.bz2 | ./predictor --gshare:15

bunzip2 -kc ../traces/fp_2.bz2 | ./predictor --gshare:15

bunzip2 -kc ../traces/mm_1.bz2 | ./predictor --gshare:15

bunzip2 -kc ../traces/mm_2.bz2 | ./predictor --gshare:15

bunzip2 -kc ../traces/int_1.bz2 | ./predictor --gshare:15

bunzip2 -kc ../traces/int_2.bz2 | ./predictor --gshare:15

# More References : 
https://www.cs.cmu.edu/afs/cs/academic/class/15740-s17/www/lectures/L19-BranchPrediction.pdf

https://jilp.org/vol8/v8paper1.pdf

https://jilp.org/vol7/v7paper10.pdf

https://www.irisa.fr/caps/people/seznec/L-TAGE.pdf

https://jilp.org/cbp/traces-with-values/

Few Notes about TAGE  : 
1. We have implemented 7 Tage Tables with 512 entries each to fit within 64kb budget
2. Prediction counter is 3 bits, usefulness counter is 2 bits
3. Usefulness Counter is updated after every 256k predictions as mentioned in the paper
4. History lengths are 2,4,8,16,32,64,128 for our tables
5. Total History Length for us is 131
6. Bimodal Table is 4K with 2 bit counter, we don't have any other counter for bimodal like in Tage Paper PDF above
7. Tag is computed using (pc,CSR1,(CSR2<<1)) & IndexMaskofTable (ie 9'h1FF for us)
8. For Tables where geometric history is higher than index length, the history is folded as mentioned in paper
9. Some enhancements from other implementations we saw was to XOR the pc two times with the history to index into  the Tage Tables.. 
  The second PC that is XORed here is actually shifted by some bits to give weightage to the table computation index ( higher the table number, higher the computation, so more bits to consider for the second pc XOR)
10. CSR is implemented by taking the MSB going out and XORing with incoming bit.

#include <stdio.h>
#include "predictor.h"
#include <math.h>

#define TAGE_TABLE_ENTRIES  512
#define TAGE_TABLE_NUMS       7
#define TAGE_BIMODAL_SIZE    12
#define TAGE_BIMODAL_TABLE_SIZE  4096
#define TAGE_TABLE_IDX_LENGTH 9
#define TAGE_HISTORY_LENGTH   131


const char *studentName = "Monil Shah";
const char *studentID   = "A59012111";
const char *email       = "m3shah@ucsd.edu";

const char *studentName2 = "Vakul Saxena";
const char *studentID2   = "A59012111";
const char *email2       = "m3shah@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int lhCounter;
int ghCounter;
int choiceCounter;
int bpType;       // Branch Prediction Type
int customType;
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//


//Gshare variables
//////////////////////////////
int gshare_global_register_mask;
int gshare_global_register = 0;
long unsigned int gshare_page_size;
uint8_t  gshare_prediction;
uint8_t * gshare_page_history_table;

//Tournament Variables
//////////////////////////////
uint32_t tournament_global_table_size;
uint32_t tournament_local_table_size;
uint32_t tournament_local_counter_size;
uint32_t * tournament_global_counter_table;
uint32_t * tournament_choice_table;
uint32_t * tournament_local_counter_table;
uint32_t tournament_local_address_mask;
uint32_t tournament_global_history_mask;
uint32_t tournament_local_pointer_mask;
uint32_t tournament_global_history;
uint32_t * tournament_local_history_table;
int tournament_pc_local, tournament_global_index;
int tournament_global_counter = 0, tournament_choice_pattern =0;
int tournament_local_counter = 0, tournament_local_pointer = 0;
uint8_t tournament_local_prediction=0, tournament_global_prediction=0, tournament_prediction = 0;

//Custom variables
////////////////////////////////////
//
uint32_t custom_global_table_size;
uint32_t custom_local_table_size;
uint32_t custom_local_counter_size;
uint32_t * custom_global_counter_table;
uint32_t * custom_choice_table;
uint32_t * custom_local_counter_table;
uint32_t custom_local_address_mask;
uint32_t custom_global_history_mask;
uint32_t custom_local_pointer_mask;
uint32_t custom_global_history;
uint32_t * custom_local_history_table;
int custom_pc_local, custom_global_index;
int custom_global_counter = 0, custom_choice_pattern =0;
int custom_local_counter = 0, custom_local_pointer = 0;
uint8_t custom_local_prediction=0, custom_global_prediction=0, custom_prediction = 0;
uint32_t ** custom_perceptron_table;
uint32_t custom_perceptron_table_size;
uint32_t custom_pc_index_mask;
uint32_t * perceptron_selected;
uint32_t pc_index_for_perceptron_table;
uint8_t * custom_global_history_register;
uint8_t custom_prediction;
uint32_t perceptron_size;
int custom_threshold;
int yout;
int custom_global_register_mask;
int custom_global_register = 0;
uint32_t custom_perceptron_max_val, custom_perceptron_min_val;

typedef struct tableEntryStruct{
  uint8_t  predCounter;
  uint8_t  usefulnessCounter;
  uint16_t tag;
} tableEntryStr;

typedef struct tableTableStruct{
  tableEntryStr tableEntry[TAGE_TABLE_ENTRIES];
  uint16_t csr1;
  uint16_t csr2;
  uint8_t  tagLength;
  uint8_t  historyLength;
  uint16_t tagMask;
  uint16_t csr2Mask;
} tageTable;

uint32_t  tageglobalhistory[TAGE_HISTORY_LENGTH];
uint16_t tagehistoryLength = TAGE_HISTORY_LENGTH;
uint16_t tageBimodalIndexMask;
uint16_t  tageTableIndexLength = TAGE_TABLE_IDX_LENGTH;
uint16_t  tageTableIndexMask;
uint16_t tageTableIndexBits = TAGE_TABLE_IDX_LENGTH;
uint8_t  tagTagLengths[TAGE_TABLE_NUMS];
uint16_t tageBimodalWidth = TAGE_BIMODAL_SIZE;
uint32_t  tageTagMatch[TAGE_TABLE_NUMS];
uint32_t  tageTablePrediction[TAGE_TABLE_NUMS];
uint32_t  tageTablePredictionIndex[TAGE_TABLE_NUMS];
uint32_t  tageTablePredictionTag[TAGE_TABLE_NUMS];
uint8_t   customPrediction;
uint32_t  tageBimodalTable[TAGE_BIMODAL_TABLE_SIZE];
int pred,altpred,predTableNum, altpredTableNum;
uint8_t bimodal_prediction, bimodal_counter;
uint32_t customTageCounter, predPrediction, altPrediction, msbUsefulnessClear;
uint32_t tageTagLengths[TAGE_TABLE_NUMS] = {9,9,10,10,11,11,12};
uint32_t tageBimodalSize;
uint32_t tagePathHistory,tagePathHistoryMask;
tageTable tageTables[TAGE_TABLE_NUMS];
//
//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void initialize_gshare(){
  int x;
  gshare_global_register_mask = (1 << ghistoryBits) - 1; 
  //1 is subtracetd after shifting To find a mask where upper bits are zero and lower bits are the ghistory
  gshare_page_size = 1 << ghistoryBits;
  //For GShare page size is equal to 2 ** ghistoryBits
  gshare_page_history_table = (uint8_t *) malloc (sizeof(uint8_t) * gshare_page_size);
  for( x = 0; x < gshare_page_size; x++) {
    gshare_page_history_table[x] = 1; //Initialize to weakly not taken
  }
  
}

void initialize_tournament(){
  int x;
  tournament_global_table_size = 1 << ghistoryBits; 
  tournament_global_counter_table = (uint32_t *) malloc (sizeof(uint32_t) * tournament_global_table_size);
  tournament_choice_table = (uint32_t *) malloc (sizeof(uint32_t) * tournament_global_table_size);
  tournament_local_table_size = 1 << lhistoryBits;
  tournament_local_counter_size  = 1 << pcIndexBits;
  tournament_local_counter_table = (uint32_t *) malloc (sizeof(uint32_t) * tournament_local_counter_size);
  tournament_local_history_table = (uint32_t *) malloc (sizeof(uint32_t) * tournament_local_table_size);
  tournament_global_history_mask = tournament_global_table_size -1; //12'hFFF
  tournament_local_address_mask = tournament_local_table_size -1; //10'h3FF
  tournament_global_history     = 0;


  for(x = 0; x < tournament_global_table_size; x++){
    tournament_global_counter_table[x] = 1; //Initalize counter to weak not taken
  }
  for(x = 0; x < tournament_global_table_size; x++){
    tournament_choice_table[x] = 2; //initialize choice to global predictor
  }
  for(x = 0; x < tournament_local_table_size; x++){
    tournament_local_history_table[x] = 0; //Initialize branch history to be not taken
  }
  for(x = 0; x < tournament_local_counter_size; x++){
    tournament_local_counter_table[x] = 1; //initialize counter to be weakly not taken
  }
}
uint32_t createmask(uint32_t maskBits){
  return ((1 << maskBits) -1);
}

void initialize_custom(){
  int x;
  if(customType == CUSTOM_TOURSHARE){
    ghistoryBits = 13;
    lhistoryBits = 11;
    pcIndexBits  = 11;
    custom_global_table_size = 1 << ghistoryBits; 
    custom_global_counter_table = (uint32_t *) malloc (sizeof(uint32_t) * custom_global_table_size);
    custom_choice_table = (uint32_t *) malloc (sizeof(uint32_t) * custom_global_table_size);
    custom_local_table_size = 1 << lhistoryBits;
    custom_local_counter_size  = 1 << pcIndexBits;
    custom_local_counter_table = (uint32_t *) malloc (sizeof(uint32_t) * custom_local_counter_size);
    custom_local_history_table = (uint32_t *) malloc (sizeof(uint32_t) * custom_local_table_size);
    custom_global_history_mask = custom_global_table_size -1; //12'hFFF
    custom_local_address_mask = custom_local_table_size -1; //10'h3FF
    custom_global_history     = 0;

    for(x = 0; x < custom_global_table_size; x++){
      custom_global_counter_table[x] = 1; //Initalize counter to weak not taken
    }
    for(x = 0; x < custom_global_table_size; x++){
      custom_choice_table[x] = 1; //initialize choice to local predictor
    }
    for(x = 0; x < custom_local_table_size; x++){
      custom_local_history_table[x] = 0; //Initialize branch history to be not taken
    }
    for(x = 0; x < custom_local_counter_size; x++){
      custom_local_counter_table[x] = 3; //initialize counter to be weakly not taken 3 bit
    }
  }
  else if (customType == CUSTOM_LOCAL){
    lhistoryBits = 11;
    pcIndexBits  = 11;
    custom_local_table_size = 1 << lhistoryBits;
    custom_local_counter_size  = 1 << pcIndexBits;
    custom_local_counter_table = (uint32_t *) malloc (sizeof(uint32_t) * custom_local_counter_size);
    custom_local_history_table = (uint32_t *) malloc (sizeof(uint32_t) * custom_local_table_size);
    custom_local_address_mask = custom_local_table_size -1; //10'h3FF
    custom_global_history     = 0;
    for(x = 0; x < custom_local_table_size; x++){
      custom_local_history_table[x] = 0; //Initialize branch history to be not taken
    }
    for(x = 0; x < custom_local_counter_size; x++){
      custom_local_counter_table[x] = 1; //initialize counter to be weakly not taken 3 bit
    }
  }
  else if (customType == CUSTOM_PERCEPTRON){
    int x,y;
    pcIndexBits = 10;
    ghistoryBits = 12;
    custom_threshold = 1.93*ghistoryBits + 14;
    custom_perceptron_table_size = 1 << pcIndexBits;
    custom_global_register_mask = (1 << ghistoryBits) - 1; 
    perceptron_size = (ghistoryBits+1);
    custom_perceptron_max_val = 2*2*2*2*2 - 1;
    custom_perceptron_min_val = -custom_perceptron_max_val -1;
    custom_perceptron_table = (uint32_t **) malloc (sizeof(uint32_t *) * custom_perceptron_table_size);
    for(x=0; x < custom_perceptron_table_size; x++) {
      custom_perceptron_table[x] = (uint32_t *)malloc(sizeof(int) * perceptron_size);
    }
    perceptron_selected = (uint32_t *) malloc (sizeof(uint32_t) * perceptron_size); 
    custom_pc_index_mask = custom_perceptron_table_size - 1;
    custom_global_history_register = (uint8_t *) malloc ( sizeof(uint8_t) * ghistoryBits);
    for(x = 0; x < custom_perceptron_table_size; x++){
      for(y=0; y < perceptron_size; y++){
        custom_perceptron_table[x][y] = 0;
      }
    } //initialize perceptron to be not taken completely
    for(x = 0; x <= ghistoryBits; x++){
      custom_global_history_register[x] = 0;
    }
  }
  else if(customType == CUSTOM_TAGE){
    int x;
    //tageglobalhistory = (uint32_t * ) malloc (sizeof(uint8_t) * tagehistoryLength);
    //tageTagMatch = (uint32_t *) malloc (sizeof(uint8_t) * TAGE_TABLE_NUMS);
    //tageTablePrediction = (uint32_t *) malloc (sizeof(uint8_t) * TAGE_TABLE_NUMS);
    //tageTablePredictionIndex = (uint32_t *) malloc (sizeof(uint8_t) * TAGE_TABLE_NUMS);
    //tageTablePredictionTag= (uint32_t *) malloc (sizeof(uint32_t) * TAGE_TABLE_NUMS);
    tagePathHistory = 0;
    tagePathHistoryMask = (1 << 16) - 1;
    for(x = 0; x < tagehistoryLength; x++){
      tageglobalhistory[x] = 0;
    } //Initialize history to NT
    for(x = 0; x < TAGE_TABLE_NUMS; x++){
      for(int y = 0; y < TAGE_TABLE_ENTRIES; y++){
        tageTables[x].tableEntry[y].predCounter       = 3; //weak NT
        tageTables[x].tableEntry[y].tag               = 0; // no tga initially
        tageTables[x].tableEntry[y].usefulnessCounter = 0; //Strongly notuseful
      }
      tageTables[x].historyLength = 1 << (x+1);
      tageTables[x].tagLength     = tageTagLengths[x];
      tageTables[x].tagMask= createmask(tageTagLengths[x]); 
      tageTables[x].csr2Mask= createmask(tageTagLengths[x]-1); 
      tageTables[x].csr1         = 0;
      tageTables[x].csr2         = 0;
      tageTagMatch[x]     = 0;
      tageTablePrediction[x]   = 0;
      tageTablePredictionIndex[x] = 0;
      tageTablePredictionTag[x]   = 0;
    }
    tageBimodalIndexMask = createmask(tageBimodalWidth);
    tageTableIndexMask   = createmask(tageTableIndexLength);
    tageBimodalSize = (1 << TAGE_BIMODAL_SIZE);
    //tageBimodalTable = (uint32_t * )  malloc (sizeof(uint8_t) * tageBimodalSize);
    //printf("Hello World\n");
    for(x = 0; x < tageBimodalSize; x++){
      tageBimodalTable[x] = 1; // 2bit counter initialized to weakly not taken
    }
    //printf("Hello World\n");
  }
}


void init_predictor(){
  if(bpType == GSHARE) {
     initialize_gshare();
  }
  if(bpType == TOURNAMENT) {
     initialize_tournament();
  }
  if(bpType == CUSTOM) {
    initialize_custom();
  //printf("Initializing CUSTOM \n");
  }    
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_gshare_prediction(uint32_t pc_temp){
  uint8_t counter = 0;
  pc_temp = (pc_temp  & gshare_global_register_mask) ^ (gshare_global_register & gshare_global_register_mask); //This will XOR the PC with History
  counter = gshare_page_history_table[pc_temp];
  gshare_prediction = (counter > 1); //Check if Counter is predicting Taken or Not taken
  if(counter > 1){
    return TAKEN;
  }
  else{
    return NOTTAKEN;
  }
}

uint8_t make_tournament_prediction(uint32_t pc_temp){
  tournament_global_index = tournament_global_history & tournament_global_history_mask; //For global we dont use PC, we use the History
  tournament_pc_local  = ((pc_temp) & tournament_local_address_mask);

  tournament_global_counter = tournament_global_counter_table[tournament_global_index] ; //to get global counter
  tournament_choice_pattern = tournament_choice_table[tournament_global_index]; //To get the choice

  tournament_local_pointer =   tournament_local_history_table[tournament_pc_local];
  tournament_local_counter =   (tournament_local_counter_table[tournament_local_pointer]); //To get local counter

  //Check what counter of each type is predicting
  if(tournament_local_counter > 1){
    tournament_local_prediction = 1;
  }
  else{
    tournament_local_prediction = 0;
  }
  if(tournament_global_counter > 1){
    tournament_global_prediction = 1;
  }
  else{
    tournament_global_prediction = 0;
  }
  //Check which choice is better.. saturating counter again...
  if(tournament_choice_pattern > 1){
    tournament_prediction =  tournament_global_prediction;
  }
  else{
    tournament_prediction =  tournament_local_prediction;
  }
  //printf("tournament_local_prediction %x tournament_global_prediction %x tournament_choice_pattern %x tournament_prediction %x\n",
    //tournament_local_prediction,tournament_global_prediction,tournament_choice_pattern,tournament_prediction);
  return tournament_prediction;
}


uint32_t tageIndexComputation(uint32_t pc_temp, uint32_t historyLength, uint8_t tableNum){
//printf("Hello Worls\n");
  int x,y,inner_idx;
  uint32_t finalhistoryXor=0,temphistoryXor =0;
  uint32_t pcXor = 0,temppcXor=0;
  uint8_t xorFolds = 0, xorInnerLoop = 0; 
  uint16_t tageTableTag =0, computedTag =0;
  //Xor of History
  //printf("Table Num : %d\n", tableNum);
  if(tageTableIndexLength > historyLength){
    //printf("historyLength : %d, tageTableIndexLength : %d\n", historyLength,tageTableIndexLength);
    for(x = (historyLength -1); x >= 0; x--){
      finalhistoryXor = (finalhistoryXor << 1) | (tageglobalhistory[x]);
      //printf("finalhistoryXor : 'h%x, TABLE NUM %d\n", finalhistoryXor,tableNum);
    }
  }
  else{
    xorFolds = historyLength/tageTableIndexLength; //how many folds
    //printf("historyLength : %d, tageTableIndexLength : %d, xorFolds : %d\n", historyLength,tageTableIndexLength,xorFolds);
    for(x = 0; x <= xorFolds; x++){
      if(x == xorFolds)
        xorInnerLoop = historyLength - x*tageTableIndexLength;
      else
        xorInnerLoop = tageTableIndexLength;

      temphistoryXor = 0;
      for(y = (xorInnerLoop-1); y >= 0; y--){
        inner_idx = x*tageTableIndexLength+y;
        //printf("inner_idx %d\n", inner_idx);
        temphistoryXor = (temphistoryXor << 1) | (tageglobalhistory[x*tageTableIndexLength+y]);
      }
      //printf("temphistoryXor : %x\n",temphistoryXor);
      if(x != 0){
        finalhistoryXor = finalhistoryXor ^ temphistoryXor;
      }
      else{
        finalhistoryXor = temphistoryXor;
      }
      //printf("finalhistoryXor : 'h%x, TABLE NUM %d, xorInnerLoop : %d, temphistoryXor : %x, finalhistoryXor %x\n", finalhistoryXor,tableNum,xorInnerLoop, temphistoryXor, finalhistoryXor);
    }
  }
  
  //Xor of PC [idx-1:0] & pc[2*idx-1:idx]
  for(y = 0; y < 2; y++){
    if(y == 0){ 
      temppcXor = pc_temp & tageTableIndexMask;
      pcXor = temppcXor;
    }
    else{
      pc_temp = pc_temp >> (tageTableIndexBits - tableNum);
      temppcXor = pc_temp & tageTableIndexMask;
      pcXor = pcXor ^ temppcXor;
    }
  }
  //pcXor = pc_temp & tageTableIndexMask;
  pcXor = pcXor ^ finalhistoryXor ^ (tagePathHistory >> (tageTableIndexBits - tableNum));
  pcXor = pcXor & tageTableIndexMask;
  //printf("pcXor : %x\n", pcXor);
  
  return pcXor;
}
uint32_t tageTagComputation(uint32_t pc_temp, uint32_t csr1, uint32_t csr2, uint32_t tagmask, uint32_t tagLengthBits){
  uint32_t tag, csr1_temp, csr2_temp;
  int x;
//printf("Tag BEFORE:%x, mask %x\n",tag,tagmask);
  tag = (pc_temp ^ csr1_temp ^ (csr2_temp << 1)) & tagmask;
//printf("Tag AFTER:%x\n", tag);
  return tag;
}

uint8_t make_custom_prediction(uint32_t pc_temp){
  if(customType == CUSTOM_TOURSHARE){
    custom_global_index = (pc_temp &  custom_global_history_mask) ^ (custom_global_history & custom_global_history_mask); //For global we dont use PC, we use the History
    custom_pc_local  = ((pc_temp) & custom_local_address_mask);

    custom_global_counter = custom_global_counter_table[custom_global_index] ; //to get global counter
    custom_choice_pattern = custom_choice_table[custom_global_index]; //To get the choice

    custom_local_pointer =   custom_local_history_table[custom_pc_local];
    custom_local_counter =   (custom_local_counter_table[custom_local_pointer]); //To get local counter

    //Check what counter of each type is predicting
    if(custom_local_counter > 3){
      custom_local_prediction = 1;
    }
    else{
      custom_local_prediction = 0;
    }
    if(custom_global_counter > 1){
      custom_global_prediction = 1;
    }
    else{
      custom_global_prediction = 0;
    }
    //Check which choice is better.. saturating counter again...
    if(custom_choice_pattern > 1){
      custom_prediction =  custom_global_prediction;
    }
    else{
      custom_prediction =  custom_local_prediction;
    }
    //printf("custom_local_prediction %x custom_global_prediction %x custom_choice_pattern %x custom_prediction %x\n",
      //custom_local_prediction,custom_global_prediction,custom_choice_pattern,custom_prediction);
    return custom_prediction;
  }
  else if (customType == CUSTOM_LOCAL){
    custom_pc_local  = ((pc_temp) & custom_local_address_mask);
    custom_local_pointer =   custom_local_history_table[custom_pc_local];
    custom_local_counter =   (custom_local_counter_table[custom_local_pointer]); //To get local counter

    //Check what counter of each type is predicting
    if(custom_local_counter > 1){
      custom_prediction = 1;
    }
    else{
      custom_prediction = 0;
    }
    //printf("custom_local_prediction %x custom_global_prediction %x custom_choice_pattern %x custom_prediction %x\n",
      //custom_local_prediction,custom_global_prediction,custom_choice_pattern,custom_prediction);
    return custom_prediction;
  }
  else if (customType == CUSTOM_PERCEPTRON){
    int x,activation;
    yout = 0;
    uint8_t neuron_activation_temp;
    pc_temp = (pc_temp  & custom_global_register_mask) ^ (custom_global_register & custom_global_register_mask);
    pc_index_for_perceptron_table = pc_temp & custom_pc_index_mask;
    perceptron_selected = custom_perceptron_table[pc_index_for_perceptron_table];
    //printf("pc_temp : %x \n", pc_temp);
    for(x = ghistoryBits; x >= 0; x--){ // 
      if(x ==0) {
        yout+= perceptron_selected[x];
      //printf("perceptron_weight : %d\n", perceptron_selected[x]);
      }
      else{
        neuron_activation_temp = custom_global_history_register[x];
      //printf("perceptron_weight : %d, activation : %d\n", perceptron_selected[x],neuron_activation_temp);
        yout = yout + ((neuron_activation_temp == 1) ? perceptron_selected[x] : (-1*perceptron_selected[x]));
      }
    //printf("yout : %d\n", yout);
    }
    if(yout > 0)
      custom_prediction = 1;
    else
      custom_prediction = 0;
    //printf("yout = %x, prediction : %d\n", yout, custom_prediction);

    return custom_prediction;
  }
  else if(customType == CUSTOM_TAGE){
    int y,usefulnessCounter,predictionCounter, pc_local;
    uint32_t idx,tageTableTag,computedTag;
    pc_local = pc_temp & tageBimodalIndexMask;
    bimodal_counter  = tageBimodalTable[pc_local]; 
    bimodal_prediction  = (bimodal_counter > 1);
    pred = 0;
    altpred = 0;
    predTableNum = 0;
    altpredTableNum = 0;
    predPrediction = 0;
    altPrediction = 0;
    //printf("Hello World\n");
    for(y = (TAGE_TABLE_NUMS -1); y >= 0; y--){
      tageTablePrediction[y] = 0;
      tageTablePredictionIndex[y] = 0;
      tageTablePredictionTag[y] = 0;
      idx               = tageIndexComputation(pc_temp, tageTables[y].historyLength, y);
      //printf("Hello World\n");
      tageTablePredictionIndex[y] = idx;
      //printf("idx : %x for table %x\n",tageTablePredictionIndex[y],y);
      tageTableTag      = tageTables[y].tableEntry[idx].tag;
      //printf("Hello World\n");
      predictionCounter = tageTables[y].tableEntry[idx].predCounter;
      usefulnessCounter = tageTables[y].tableEntry[idx].usefulnessCounter;
      computedTag       = tageTagComputation(pc_temp, tageTables[y].csr1, tageTables[y].csr2, tageTables[y].tagMask, tageTagLengths[y]);
      tageTablePredictionTag[y] = computedTag;
      //printf("tageTableTag %x, computedTag %x\n",tageTableTag,computedTag);
      if(tageTableTag == computedTag){
      //printf("HURRAY !!! tageTableTag %x, computedTag %x, tableNum : %x\n",tageTableTag,computedTag,y);
        tageTagMatch[y] = 1;
        //Computer Prediction and Altprediction tables
        if(pred == 0){
          pred = 1;
          predTableNum = y;
          if(predictionCounter > 3){
            tageTablePrediction[y] = 1;
          }
          else{
            tageTablePrediction[y] = 0;
          }
        }
        else if (pred == 1 && altpred == 0){
          altpred = 1;
          altpredTableNum = y;
          if(predictionCounter > 3){
            tageTablePrediction[y] = 1;
          }
          else{
            tageTablePrediction[y] = 0;
          }
          break;
        }
      }
    }
    //printf("Hello World\n");
    if(pred == 1){ //If we get a mathc in tage tables
      customPrediction = tageTablePrediction[predTableNum];
      predPrediction = customPrediction;
      if(altpred == 0){
        altPrediction = bimodal_prediction;
      }
    }
    else{
      customPrediction = bimodal_prediction;
    }
    return customPrediction;
  }
}

uint8_t make_prediction(uint32_t pc) {
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE: return make_gshare_prediction(pc);
    case TOURNAMENT: return make_tournament_prediction(pc);
    case CUSTOM: return make_custom_prediction(pc);
    default:
      break;
  }
  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
uint8_t update_2bit_counter(uint8_t prediction, uint8_t outcome, uint8_t init_counter){
  if(prediction != outcome){
    //go towars NT since prediction was T and outcome was NT
    if((prediction > outcome) && (init_counter != 0))
      init_counter-=1;
    //go towards T since prediction was NT and outcome was T
    else if((prediction < outcome) && (init_counter != 3))
      init_counter+=1;
  }
  else{
    //Go towards T since prediction and Outcome was T
    if((prediction) && (init_counter != 3))
      init_counter+=1;
    //Go towards NT since prediction and outcome was NT
    else if((!prediction) && (init_counter != 0))
      init_counter-=1;
  }
  return init_counter;
}

uint8_t update_3bit_counter(uint8_t prediction, uint8_t outcome, uint8_t init_counter){
  if(prediction != outcome){
    //go towars NT since prediction was T and outcome was NT
    if((prediction > outcome) && (init_counter != 0))
      init_counter-=1;
    //go towards T since prediction was NT and outcome was T
    else if((prediction < outcome) && (init_counter != 7))
      init_counter+=1;
  }
  else{
    //Go towards T since prediction and Outcome was T
    if((prediction) && (init_counter != 7))
      init_counter+=1;
    //Go towards NT since prediction and outcome was NT
    else if((!prediction) && (init_counter != 0))
      init_counter-=1;
  }
  return init_counter;
}

uint8_t update_4bit_counter(uint8_t prediction, uint8_t outcome, uint8_t init_counter){
  if(prediction != outcome){
    //go towars NT since prediction was T and outcome was NT
    if((prediction > outcome) && (init_counter != 0))
      init_counter-=1;
    //go towards T since prediction was NT and outcome was T
    else if((prediction < outcome) && (init_counter != 15))
      init_counter+=1;
  }
  else{
    //Go towards T since prediction and Outcome was T
    if((prediction) && (init_counter != 15))
      init_counter+=1;
    //Go towards NT since prediction and outcome was NT
    else if((!prediction) && (init_counter != 0))
      init_counter-=1;
  }
  return init_counter;
}
void update_gshare_pred_state(uint32_t pc, uint8_t outcome){
  int counter = 0;
  pc = (pc & gshare_global_register_mask) ^ (gshare_global_register & gshare_global_register_mask);
  counter = gshare_page_history_table[pc];
  counter = update_2bit_counter(gshare_prediction, outcome, counter); 
  gshare_page_history_table[pc] = counter;
  gshare_global_register = (gshare_global_register << 1) + outcome; //Shift register one bit left and append the outcome..
}

void update_tournament_pred_state(uint32_t pc_temp, uint8_t outcome){
  
  tournament_global_index = tournament_global_history & tournament_global_history_mask;
  tournament_pc_local  = ((pc_temp) & tournament_local_address_mask);
  tournament_local_pointer =   tournament_local_history_table[tournament_pc_local];

  //printf("(1) pc_temp : %x tournament_local_prediction %x tournament_local_counter %x tournament_global_prediction %x tournament_global_counter %x tournament_choice_pattern %x tournament_prediction %x, outcome %x\n",
    //pc_temp,tournament_local_prediction,tournament_local_counter,tournament_global_prediction,tournament_global_counter,tournament_choice_pattern,tournament_prediction,outcome);
  //Update "tournament_global_counter" and then Write into Memory
  
  tournament_global_counter = update_2bit_counter(tournament_global_prediction, outcome, tournament_global_counter); 
  tournament_local_counter  = update_2bit_counter(tournament_local_prediction, outcome, tournament_local_counter); 
  
  
  tournament_local_counter_table[tournament_local_pointer] = tournament_local_counter; 
  tournament_local_pointer = ((tournament_local_pointer << 1) + outcome) & tournament_local_address_mask; //Update only 10 bits... Forget 11th bit
  //Doing a Read Modify Write to the table and updating the Local history counter
  tournament_local_history_table[tournament_pc_local]      = tournament_local_pointer;

  if(tournament_global_prediction != tournament_local_prediction) {
    //using the same modular function ....
    //If local is crrect and global is incorrect -- arguments are (1,0,counter) analogous to T being predicted but outcome beign NT and hence counter decrements, which is what we want
    //If global is correct and local is incorrect -- arguments are (0,1,counter) analogous to NT being predicted but outcome being T and hence counter increments, which is what we want 
    tournament_choice_pattern = update_2bit_counter((tournament_local_prediction == outcome),(tournament_global_prediction == outcome), tournament_choice_pattern);  
  }
  //printf("(2) pc_temp %x tournament_local_prediction %x tournament_global_prediction %x tournament_choice_pattern %x tournament_prediction %x, outcome %x\n",
    //pc_temp,tournament_local_prediction,tournament_global_prediction,tournament_choice_pattern,tournament_prediction,outcome);
  
  tournament_choice_table[tournament_global_index] =  tournament_choice_pattern; //To get bit[5:4] of the 8 bit value;
  tournament_global_counter_table[tournament_global_index] = tournament_global_counter;
  tournament_global_history = ((tournament_global_history << 1) + outcome) & tournament_global_history_mask;
}

void update_custom_pred_state(uint32_t pc_temp, uint8_t outcome){
  if(customType == CUSTOM_TOURSHARE){
    custom_global_index = (pc_temp &  custom_global_history_mask) ^ (custom_global_history & custom_global_history_mask);
    custom_pc_local  = ((pc_temp) & custom_local_address_mask);
    custom_local_pointer =   custom_local_history_table[custom_pc_local];

    //printf("(1) pc_temp : %x custom_local_prediction %x custom_local_counter %x custom_global_prediction %x custom_global_counter %x custom_choice_pattern %x custom_prediction %x, outcome %x\n",
      //pc_temp,custom_local_prediction,custom_local_counter,custom_global_prediction,custom_global_counter,custom_choice_pattern,custom_prediction,outcome);
    //Update "custom_global_counter" and then Write into Memory
    custom_global_counter = update_2bit_counter(custom_global_prediction, outcome, custom_global_counter); 
    custom_local_counter  = update_3bit_counter(custom_local_prediction, outcome, custom_local_counter); 
    
    
    custom_local_counter_table[custom_local_pointer] = custom_local_counter; 
    custom_local_pointer = ((custom_local_pointer << 1) + outcome) & custom_local_address_mask; //Update only 10 bits... Forget 11th bit
    //Doing a Read Modify Write to the table and updating the Local history counter
    custom_local_history_table[custom_pc_local]      = custom_local_pointer;

    if(custom_global_prediction != custom_local_prediction) {
      //using the same modular function ....
      //If local is crrect and global is incorrect -- arguments are (1,0,counter) analogous to T being predicted but outcome beign NT and hence counter decrements, which is what we want
      //If global is correct and local is incorrect -- arguments are (0,1,counter) analogous to NT being predicted but outcome being T and hence counter increments, which is what we want 
      custom_choice_pattern = update_2bit_counter((custom_local_prediction == outcome),(custom_global_prediction == outcome), custom_choice_pattern);  
    }
    //printf("(2) pc_temp %x custom_local_prediction %x custom_global_prediction %x custom_choice_pattern %x custom_prediction %x, outcome %x\n",
      //pc_temp,custom_local_prediction,custom_global_prediction,custom_choice_pattern,custom_prediction,outcome);
    
    custom_choice_table[custom_global_index] =  custom_choice_pattern; //To get bit[5:4] of the 8 bit value;
    custom_global_counter_table[custom_global_index] = custom_global_counter;
    custom_global_history = ((custom_global_history << 1) + outcome) & custom_global_history_mask;
  }
  else if(customType == CUSTOM_LOCAL){
    custom_pc_local  = ((pc_temp) & custom_local_address_mask);
    custom_local_pointer =   custom_local_history_table[custom_pc_local];

    //printf("(1) pc_temp : %x custom_local_prediction %x custom_local_counter %x custom_global_prediction %x custom_global_counter %x custom_choice_pattern %x custom_prediction %x, outcome %x\n",
      //pc_temp,custom_local_prediction,custom_local_counter,custom_global_prediction,custom_global_counter,custom_choice_pattern,custom_prediction,outcome);
    //Update "custom_global_counter" and then Write into Memory
    custom_local_counter  = update_2bit_counter(custom_local_prediction, outcome, custom_local_counter); 
    
    
    custom_local_counter_table[custom_local_pointer] = custom_local_counter; 
    custom_local_pointer = ((custom_local_pointer << 1) + outcome) & custom_local_address_mask; //Update only 10 bits... Forget 11th bit
    //Doing a Read Modify Write to the table and updating the Local history counter
    custom_local_history_table[custom_pc_local]      = custom_local_pointer;
  }
  else if (customType == CUSTOM_PERCEPTRON){
    int x,bipolar_outcome,activation;
    int yout = 0;
    uint8_t neuron_activation_temp;
    pc_temp = (pc_temp  & custom_global_register_mask) ^ (custom_global_register & custom_global_register_mask);
    bipolar_outcome = (outcome == 1) ? 1 : -1;
    if((custom_prediction != outcome) || (abs(yout)< custom_threshold)){
      pc_index_for_perceptron_table = pc_temp & custom_pc_index_mask;
      perceptron_selected = custom_perceptron_table[pc_index_for_perceptron_table];
      for(x = ghistoryBits; x >= 0; x--){ // 
          if(x ==0) {
          //printf("perceptron_select[x] %d",perceptron_selected[x]);
            perceptron_selected[x] = perceptron_selected[x] +  bipolar_outcome;
          //printf("%d\n",perceptron_selected[x]);
          }
          else{
            neuron_activation_temp = custom_global_history_register[x];
            if(perceptron_selected[x] <= custom_perceptron_max_val && perceptron_selected[x] >= custom_perceptron_min_val){
              perceptron_selected[x] = perceptron_selected[x] + ((neuron_activation_temp == 1)  ? bipolar_outcome : (-1*bipolar_outcome));
            }
          }
      }
      custom_perceptron_table[pc_index_for_perceptron_table] = perceptron_selected;

    }
    for(x = ghistoryBits; x > 0; x--){
      custom_global_history_register[x] = custom_global_history_register[x-1];
    //printf("%d",custom_global_history_register[x]);
    }
    custom_global_history_register[0] = outcome;
    custom_global_register = (custom_global_register << 1) + outcome; //Shift register one bit left and append the outcome..
  //printf("\n custom_prediction %d, outcome %d \n",custom_prediction, outcome);
  }
  else if (customType == CUSTOM_TAGE){
    uint8_t  csr1_msb, csr2_msb,allocateMatch,predictionCounter, globalHistoryMSB = 0;
    int x,allocateStart=0;
    customTageCounter++;
    allocateMatch = 0;
    //printf("ABCD\n");
    pc_temp = pc_temp & tageBimodalIndexMask;
    //Update prediiction counter if u predicted from tage table..... else bimodal
    for(x =0; x < TAGE_TABLE_NUMS; x++){
      //printf("Table %x, idx %x \n", x,tageTablePredictionIndex[x]);
    }

    if(pred == 1){
    //printf("B\n");
      predictionCounter = tageTables[predTableNum].tableEntry[tageTablePredictionIndex[predTableNum]].predCounter;
      predictionCounter = update_3bit_counter(tageTablePrediction[predTableNum], outcome, predictionCounter);
      tageTables[predTableNum].tableEntry[tageTablePredictionIndex[predTableNum]].predCounter = predictionCounter;
    }
    else{
    //printf("C\n");
      bimodal_counter = update_2bit_counter(bimodal_prediction, outcome, bimodal_counter);
      tageBimodalTable[pc_temp] = bimodal_counter;
    } 
    //printf("SPECIAL1 table : 0, idx : %x\n",tageTablePredictionIndex[0]);
    //Update usefulness for TageTable
    if(pred == 1 && (tageTablePrediction[predTableNum] != predPrediction)){
    //printf("D\n");
      if(outcome != predPrediction){
        tageTables[predTableNum].tableEntry[tageTablePredictionIndex[predTableNum]].usefulnessCounter--;
      }
      else{
        tageTables[predTableNum].tableEntry[tageTablePredictionIndex[predTableNum]].usefulnessCounter++;
      }
    }
    //printf("SPECIAL2 table : 0, idx : %x\n",tageTablePredictionIndex[0]);
    if(customTageCounter == 262144){
    //printf("E\n");
      if(msbUsefulnessClear % 2 == 0)
      {
        for(x = 0; x < TAGE_TABLE_NUMS; x++){
          for(int y = 0; y < TAGE_TABLE_ENTRIES; y++){
            tageTables[x].tableEntry[y].usefulnessCounter       = tageTables[x].tableEntry[y].usefulnessCounter & 1; //weak NT
          }
        }
      }
      else{
        for(x = 0; x < TAGE_TABLE_NUMS; x++){
          for(int y = 0; y < TAGE_TABLE_ENTRIES; y++){
            tageTables[x].tableEntry[y].usefulnessCounter       = (int)(tageTables[x].tableEntry[y].usefulnessCounter & 2); //weak NT //CHECK THIS LOGIC.. Dividing by 2 -> typecasting to int and then multiply by 2
          }
        }
      }
      msbUsefulnessClear++;
      customTageCounter = 0;
    }
  //printf("F\n");
    //printf("SPECIAL3 table : 0, idx : %x\n",tageTablePredictionIndex[0]);
    for(x = 0; x < TAGE_TABLE_NUMS; x++){
          csr1_msb = (tageTables[x].csr1 << 1) % tageTables[x].tagMask; // this is like  making 8 bit number  to 9 bit number and then dividing by 8 bit mask
          csr2_msb = (tageTables[x].csr2 << 1) % (tageTables[x].csr2Mask); // this is like making 7 bit number to 8 bit numberand then dividing 8 bit number by 7 bit mask 
          globalHistoryMSB = tageglobalhistory[tageTables[x].historyLength - 1];
          tageTables[x].csr1 = ((tageTables[x].csr1 << 1) + ((outcome ^ csr1_msb ^ globalHistoryMSB) & 1)) & tageTables[x].tagMask;
          tageTables[x].csr2 = ((tageTables[x].csr2 << 1) + ((outcome ^ csr2_msb ^ globalHistoryMSB) & 1)) & (tageTables[x].csr2Mask);
    }
  //printf("G\n");
    //Update Global History
    //printf("SPECIAL5 table : 0, idx : %x\n",tageTablePredictionIndex[0]);
    for(int x = (tagehistoryLength-1); x > 0; x--){
      tageglobalhistory[x] = tageglobalhistory[x-1];
    }
    tageglobalhistory[0] = outcome ? 1 : 0;
    tagePathHistory = ((tagePathHistory << 1) + (pc_temp & 1) & tagePathHistoryMask);
  //printf("H\n");
    //printf("SPECIAL6 table : 0, idx : %x\n",tageTablePredictionIndex[0]);
    //Finding which table to start searching from for the Allocation
    if(pred == 1){
      allocateStart = predTableNum+1;
    }
    //printf("FRESH START\n");
    //printf("AllocateStart : %x\n",allocateStart);
    if(allocateStart < TAGE_TABLE_NUMS) {
      for(x = allocateStart; x < TAGE_TABLE_NUMS; x++){
      //printf("table : %x, idx : %x\n",x,tageTablePredictionIndex[x]);
        if(tageTables[x].tableEntry[tageTablePredictionIndex[x]].usefulnessCounter == 0){ //If usefulness counter is 0, you have found match
          tageTables[x].tableEntry[tageTablePredictionIndex[x]].tag = tageTablePredictionTag[x];
          tageTables[x].tableEntry[tageTablePredictionIndex[x]].predCounter = (outcome ? 4 : 3);
          allocateMatch = 1;
        //printf("WOOHOO Allocated something in table %x with Tag : %x, PredCounter : %x with index : %x\n",x,tageTables[x].tableEntry[tageTablePredictionIndex[x]].tag,tageTables[x].tableEntry[tageTablePredictionIndex[x]].predCounter,tageTablePredictionIndex[x]);
          break;
        }
      }
      if(allocateMatch == 0){
      //printf("COULDNT Allocate\n");
        for(x = allocateStart; x < TAGE_TABLE_NUMS; x++){
          tageTables[x].tableEntry[tageTablePredictionIndex[x]].usefulnessCounter--; //If couldn't allocate, reduce usefulness for each table thereafter
        }
      }
    }
  }
} 


void train_predictor(uint32_t pc, uint8_t outcome) {
  //
  //TODO: Implement Predictor training
  //
  switch(bpType) {
  case STATIC: break;
  case GSHARE:  update_gshare_pred_state(pc, outcome); break;
  case TOURNAMENT: update_tournament_pred_state(pc, outcome); break;
  case CUSTOM: update_custom_pred_state(pc,outcome); break; 
  }
}

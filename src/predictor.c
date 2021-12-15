#include <stdio.h>
#include "predictor.h"
#include <math.h>

#define TAGE_TABLE_ENTRIES  512
#define TAGE_TABLE_NUMS       7
#define TAGE_BIMODAL_SIZE    12
#define TAGE_TABLE_IDX_LENGTH 9
#define TAGE_HISTORY_LENGTH   50


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


uint8_t make_custom_prediction(uint32_t pc_temp){
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

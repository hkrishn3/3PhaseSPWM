#include <Arduino.h>

/***************************************
 * 3-Phase SPWM Motor Controller   
 *
 ***************************************/
 
#include <stdlib.h>
//Beginning of Auto generated function prototypes by Atmel Studio
void clear_string(char read_string);
void get_serial(char read_string);
void setup_pwm();
void setup_dds();
void dds_update(void );
void update_pwm();
void update_spwm();
void pause_spwm();
//End of Auto generated function prototypes by Atmel Studio



#define PIN0 0

/********3 PHASE SPWM START*****************************/

//LUT size
#define TABLE_SIZE 256

//Channel 1-3 on TIMER1
#define PWM_PIN_SPWM1 6
#define PWM_PIN_SPWM2 7
#define PWM_PIN_SPWM3 8

//Timer 1 = PWM
//Timer 3 = Output compare and Phase accumulator update ISR
#define TIMER_NUM1 1
#define TIMER_NUM3 3

//HardwareTimer classes for both TIMERS
HardwareTimer timer_spwm(TIMER_NUM1);
HardwareTimer timer_oc(TIMER_NUM3);

//Initialize the variables
//declare them volatile since they will used in ISR
volatile uint32 phase_accumulator = 0;
volatile uint8 phasor = 0;
volatile uint8 pulse_width1 = 0;
volatile uint8 pulse_width2 = 0;
volatile uint8 pulse_width3 = 0;

volatile uint32 phase_increment = 3067833;

volatile const unsigned char sine_table[]  =  {
  127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
  242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
  221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
  76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
  33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124
};

uint8 offset_120 = 85;
uint8 offset_240 = 170;

uint32 tuning_word = 3067833;
uint8 three_phase_spwm = 0;
uint8 center_align_pwm = 0;

/********3 PHASE SPWM END***********************/

#define STRING_SIZE 21

#define TIMER_NUM2 2

//Channel 1-4 on TIMER2
#define PWM_PIN_CH1 1
#define PWM_PIN_CH2 2
#define PWM_PIN_CH3 3
#define PWM_PIN_CH4 0

//Overflow and compare values for all channels for TIMER2

uint16 arr_val = 65535;
uint16 pre_scaler = 1024;
uint16 compare_val_ch1 = 32767;
uint16 compare_val_ch2 = 32767;
uint16 compare_val_ch3 = 32767;
uint16 compare_val_ch4 = 32767;

//HardwareTimer class for TIMER2
HardwareTimer timer_pwm(TIMER_NUM2);

//intialize the serial buffer string
void clear_string(char * read_string) {
  for(int i = 0; i < STRING_SIZE; i++)
    read_string[i] = 0;
}

/*****************************************
Function to communicate with the LabVIEW interface
Reads each character from a delimited string which is sent from the LabVIEW program
and assigns the values to all the different variables
*****************************************/

void get_serial(char * read_string) {

  char read_char = 0;
  int read_count = 0;
  int i = 0;

  read_char = SerialUSB.read();

  while(read_char != '#') {
    if(read_char == ';') {
      read_count++;
      switch(read_count) {
      case 1:
        arr_val = atoi(read_string);
        clear_string(read_string);
        i = 0;
        break;
      case 2:
        pre_scaler = atoi(read_string);
        clear_string(read_string);
        i = 0;
        break;
      case 3:
        compare_val_ch1 = atoi(read_string);
        clear_string(read_string);
        i = 0;
        break;
      case 4:
        compare_val_ch2 = atoi(read_string);
        clear_string(read_string);
        i = 0;
        break;
      case 5:
        compare_val_ch3 = atoi(read_string);
        clear_string(read_string);
        i = 0;
        break;
      case 6:
        compare_val_ch4 = atoi(read_string);
        clear_string(read_string);
        i = 0;
        break;
      case 7:
        tuning_word = atoi(read_string);
        clear_string(read_string);
        i = 0;
        break;
      case 8:
        three_phase_spwm = atoi(read_string);
        clear_string(read_string);
        i = 0;
        break;
      case 9:
        center_align_pwm = atoi(read_string);
        clear_string(read_string);
        i = 0;
        break;
      }
    } else {
      read_string[i] = read_char;
      i++;
    }
    read_char = SerialUSB.read();
  }
  toggleLED();
}

/*******************************************
Setup TIMER2 and all its channel for individual PWM mode
*******************************************/

void setup_pwm() {

//Pause both timers
  timer_pwm.pause();

//Set TIMER2 for PWM
//Start with a 50% duty cycle

  timer_pwm.setMode(TIMER_CH1, TIMER_PWM);
  timer_pwm.setMode(TIMER_CH2, TIMER_PWM);
  timer_pwm.setMode(TIMER_CH3, TIMER_PWM);
  timer_pwm.setMode(TIMER_CH4, TIMER_PWM);

//Set the prescaler and overflow
  timer_pwm.setPrescaleFactor(pre_scaler);
  timer_pwm.setOverflow(arr_val);
  
//Configure the compare registers for the different channels

  timer_pwm.setCompare(TIMER_CH1, compare_val_ch1);
  timer_pwm.setCompare(TIMER_CH2, compare_val_ch2);
  timer_pwm.setCompare(TIMER_CH3, compare_val_ch3);
  timer_pwm.setCompare(TIMER_CH4, compare_val_ch4);

}

/*****************************************
Setup Direct Digital Synthesis on Timer 1 
Timer 3 updates the phase accumulator register
******************************************/

void setup_dds() {

  //Pause both timers
  timer_spwm.pause();
  timer_oc.pause();

//Set TIMER1 for PWM
//Prescaler = 1
//Overflow = 256 gives a 280 kHz waveform, the number matches with the LUT size
//We will be modifying this value to control the phase of the resultant output wave
//Start with a 50% duty cycle

/********************************************
Center-aligned mode setting on TIMER 1
********************************************/

  TIMER1_BASE->CR1 |= 0x0040; 

  timer_spwm.setMode(TIMER_CH1, TIMER_PWM);

  timer_spwm.setPrescaleFactor(1);
  timer_spwm.setOverflow(TABLE_SIZE);
  timer_spwm.setCompare(TIMER_CH1, (uint16)(TABLE_SIZE/2));

  timer_spwm.setMode(TIMER_CH2, TIMER_PWM);
  timer_spwm.setOverflow(TABLE_SIZE);
  timer_spwm.setCompare(TIMER_CH2, (uint16)(TABLE_SIZE/2));

  timer_spwm.setMode(TIMER_CH3, TIMER_PWM);
  timer_spwm.setOverflow(TABLE_SIZE);
  timer_spwm.setCompare(TIMER_CH3, (uint16)(TABLE_SIZE/2));

//Set TIMER3 for Output Compare
//Prescaler = 1
//Overflow = 512 gives a 140 kHz wave form
//Dutycycle is 50%
  timer_oc.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
  timer_oc.setPrescaleFactor(1);
  timer_oc.setOverflow(TABLE_SIZE*2);
  timer_oc.setCompare(TIMER_CH1, TABLE_SIZE);

//Phase accumulator update ISR
  timer_oc.attachInterrupt(TIMER_CH1, dds_update);

//Update the registers for both timers with new values
  timer_spwm.refresh();
  timer_oc.refresh();

}

/*The heart of the DDS implementation*/

void dds_update(void) {

   // digitalWrite(PIN0, HIGH);
  /*Set the duty cycle for TIMER1 and thereby increment the phase of the desired sinusoidal wave */

  timer_spwm.setCompare(TIMER_CH1, pulse_width1);
  timer_spwm.setCompare(TIMER_CH2, pulse_width2);
  timer_spwm.setCompare(TIMER_CH3, pulse_width3);

  /*Calculate the phase for the next time ISR is executed */

//Increment phase with the Tuning word

  phase_accumulator += tuning_word;

//Set the index for the LUT to the high byte (top 8 bits) of the accumulator

  phasor = (uint8)(phase_accumulator >> 24);

//Finally set the pulse width for the next iteration

  pulse_width1 = sine_table[255-phasor];
  pulse_width2 = sine_table[255-uint8(phasor + offset_120)];
  pulse_width3 = sine_table[255-uint8(phasor + offset_240)];

//Refresh the timer with the updated values

  timer_spwm.refresh();
 // digitalWrite(PIN0, LOW);
}

//Update PWM with new settings

void update_pwm() {
  timer_pwm.setPrescaleFactor(pre_scaler);
  timer_pwm.setOverflow(arr_val);
  timer_pwm.setCompare(TIMER_CH1, compare_val_ch1);
  timer_pwm.setCompare(TIMER_CH2, compare_val_ch2);
  timer_pwm.setCompare(TIMER_CH3, compare_val_ch3);
  timer_pwm.setCompare(TIMER_CH4, compare_val_ch4);

//set center-aligned PWM if user has set it 

  if(center_align_pwm == 1) {
    TIMER2_BASE->CR1 |= 0x0040;
  }
  if(center_align_pwm == 0) {
    TIMER2_BASE->CR1 &= 0xFFBF;
  }

  timer_pwm.refresh();
  timer_pwm.resume();
}

//Update SPWM with new settings
void update_spwm() {
  timer_spwm.resume();
  timer_oc.resume();
}

//Pause SPWM on command
void pause_spwm() {
  timer_spwm.pause();
  timer_oc.pause();
}

//Setup the output pins for PWM and the on-board LED
//Setup DDS for SPWM and initialize PWM

void setup () {

//Setup the output pin for PWM
//Setup the onboard LED

  pinMode(PWM_PIN_CH1, PWM);
  pinMode(PWM_PIN_CH2, PWM);
  pinMode(PWM_PIN_CH3, PWM);
 // pinMode(PWM_PIN_CH4, PWM);
  pinMode(PIN0, OUTPUT);
  digitalWrite(PIN0, LOW);
  
  pinMode(PWM_PIN_SPWM1, PWM);
  pinMode(PWM_PIN_SPWM2, PWM);
  pinMode(PWM_PIN_SPWM3, PWM);

  pinMode(BOARD_LED_PIN, OUTPUT);

  setup_pwm();
  setup_dds();

}

//Main loop polls the serial interface for commands from the GUI and processes them
void loop () {

  char read_string[21] = {0};

  int bytes_available = SerialUSB.available();
  if(bytes_available > 0) {
    digitalWrite(0, HIGH);
    get_serial(read_string);
    update_pwm();
    digitalWrite(0, LOW);
    if(three_phase_spwm == 1)
      update_spwm();
    else if(three_phase_spwm == 0)
      pause_spwm();
  }
}

//=========================================================
// src/EFM8SB10F8A-A-QFN24_main.c: generated by Hardware Configurator
//
// This file will be updated when saving a document.
// leave the sections inside the "$[...]" comment tags alone
// or they will be overwritten!!
//=========================================================

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <si_toolchain.h>
#include <SI_EFM8SB1_Register_Enums.h>                  // SFR declarations
#include "InitDevice.h"
#include "EFM8SB1_SMBus_Master_Multibyte.h"
#include "power.h"
#include "SmaRTClock.h"
#include "adc_0.h"

// $[Generated Includes]
// [Generated Includes]$

//-----------------------------------------------------------------------------
// Application-component specific constants and variables
//-----------------------------------------------------------------------------

// Stimulation
// P0.7 - IREF
uint8_t T_on_HB;
uint8_t T_on_LB;
uint8_t T_on;
uint8_t F_hz_HB;
uint8_t F_hz_LB;
uint8_t F_hz;
uint8_t Iset;
bool stimDelivering;
bool isStim = 0;
//extern volatile uint16_t i_50us;
//extern volatile uint8_t i_50us;
//extern volatile float timer2;
//const float cycles_large = 2e6/50;
uint8_t set = 1;
uint8_t j;
float cycles;
float half_T_on;
// uint32_t chunks_30 = 30e6 / 50; // holds how many chunks of 50 us fit in 30 seconds pulse train/pulse off period
// volatile uint8_t isstim = 1;
// Stimulation function prototypes
void Polarity(uint8_t);
void T0_Waitus (uint8_t);
void Pulse_On(void);
void Pulse_Off(void);
//void Monophasic(void);
void Biphasic(void);
void Biphasic_pulm(void);
uint8_t getByte (uint16_t, uint8_t);
void RTC_Fhz_set (uint16_t);


// I2C
// P0.0 - SMBus SDA
// P0.1 - SMBus SCL
#define MEMA  0x01 // NT3H memory address
uint8_t SMB_DATA_OUT[NUM_BYTES_WR] = {0};
uint8_t SMB_DATA_IN[NUM_BYTES_RD] = {0};
uint8_t SAVE[16];
uint8_t TARGET;                             // Target SMBus slave address
volatile bool SMB_BUSY;
volatile bool SMB_RW;
uint16_t NUM_ERRORS;
SI_SBIT (SDA, SFR_P0, 0);                   // SMBus on P0.0
SI_SBIT (SCL, SFR_P0, 1);                   // and P0.1

// I2C function prototypes
void SMB_Write (void);
void SMB_Read (void);
void SDA_Reset(void);

// LT8410, MUX36 shutdown pin
SI_SBIT (P05, SFR_P0, 5);                   // Pin 0.5 for SHDN enable/disable

// MUX36S16 - H-bridge multiplexer, pins 1.4 - 1.7
SI_SBIT (P17, SFR_P1, 7);                   // Pin 1.7 for MUX36S16 A0
SI_SBIT (P16, SFR_P1, 6);                   // Pin 1.6 for MUX36S16 A1
SI_SBIT (P15, SFR_P1, 5);                   // Pin 1.5 for MUX36S16 A2
SI_SBIT (P14, SFR_P1, 4);                   // Pin 1.4 for MUX36S16 A3
uint8_t mux36s16_state = 0;                     // MUX36S16 state byte
// MUX36S16 state function
void MUX36S16_output(uint8_t);

// MUX36D08 - multiplexer for output channels, pins 0.2 - 0.4
SI_SBIT (P02, SFR_P0, 2);                   // Pin 0.2 for MUX36D08 A0
SI_SBIT (P03, SFR_P0, 3);                   // Pin 0.3 for MUX36D08 A1
SI_SBIT (P04, SFR_P0, 4);                   // Pin 0.4 for MUX36D08 A2
uint8_t mux36d08_state;                     // MUX36D08 state byte
// MUX36D08 state function
void MUX36D08_output(uint8_t);

// ADC
void sampleADC(void);
#define VREF_MV         (1650UL)
#define ADC_MAX_RESULT  ((1 << 10)-1) // 10 bit ADC
uint16_t ADC0_convertSampleToMillivolts(uint16_t);

//-----------------------------------------------------------------------------
// SiLabs_Startup() Routine
// ----------------------------------------------------------------------------
// This function is called immediately after reset, before the initialization
// code is run in SILABS_STARTUP.A51 (which runs before main() ). This is a
// useful place to disable the watchdog timer, which is enable by default
// and may trigger before main() in some instances.
//-----------------------------------------------------------------------------
void
SiLabs_Startup (void)
{
// $[SiLabs Startup]
// [SiLabs Startup]$
}

//-----------------------------------------------------------------------------
// main() Routine
// ----------------------------------------------------------------------------
void
main (void)
{
  T0_Waitus(1);
  // SMBus reset
  enter_smbus_reset_from_RESET ();
  while(!SDA){
      SDA_Reset();
  }
  // Initialize normal operation
  enter_DefaultMode_from_smbus_reset ();
  // Initialize RTC and power management.
  //SMB_Read();
  RTC0CN0_Local = 0xC0;                // Initialize Local Copy of RTC0CN0
  RTC0CN0_SetBits(RTC0TR+RTC0AEN+ALRM);// Enable Counter, Alarm, and Auto-Reset

  LPM_Init();                         // Initialize Power Management
  LPM_Enable_Wakeup(RTC);

  RTC_Alarm = 1;                      // Set the RTC Alarm Flag on startup

  // SMBus comms
  // Read data from NT3H
  SMB_DATA_OUT[0] = 0x4C;      // NT3H Address to Read
  TARGET = SLAVE_ADDR;         // I2C slave address for NT3H is 0xAA
  SMB_Write();                 // Write sequence with the MEMA as per NT3H data sheet
  TARGET = SLAVE_ADDR;
  SMB_Read();                  // Read MEMA address
  // I2C data save
  for(j=0; j<16; j++){
      SAVE[j] = SMB_DATA_IN[j];   // Save read data
  }
  T_on_HB = SAVE[0]; // Pulse Width in chunks of 50 us high byte
  T_on_LB = SAVE[1]; // Pulse Width in chunks of 50 us low byte
  F_hz_HB = SAVE[2]; // Pulse frequency high byte
  F_hz_LB = SAVE[3]; // Pulse frequency low byte
  T_on = (T_on_HB<<8)|(T_on_LB); // Combine PW into single hex
  F_hz = (F_hz_HB<<8)|(F_hz_LB); // Combine IPW into single hex
  // mux36d08_state = SAVE[4]; // save MUX36 switch state
  Iset = SAVE[8];

//  cycles = 20000 / F_hz; // number of cycles for 50 us/20 kHz timer for a given pulse frequency

  P05 = 1;              // Enable LT8410, enable MUX36D08 and 2x MUX36S16
//  half_T_on = (float) T_on / 2.0;
  TMR3CN0 |= TMR3CN0_TR3__RUN; // start timer 3 for stimulation
  while(1){
      Biphasic_pulm();
  };
}

// Function declarations

void Polarity(char polar) {
  switch (polar) {
  case 1: // Forward polarity
    MUX36D08_output(0x01);
    break;
  case 2: // Reversed polarity
    MUX36D08_output(0x02);
    break;
  case 0: // Shunted
    MUX36D08_output(0x00);
    break;
  }
}

/*
 * Function: Pulse_On
 * --------------------
 * Activates current reference at P0.7.
 * Reads amplitude bit from SMBus as Iset.
 */
void Pulse_On(void){
  IREF0CN0 = IREF0CN0_SINK__DISABLED | IREF0CN0_MDSEL__HIGH_CURRENT
                        | (Iset << IREF0CN0_IREF0DAT__SHIFT);
}

void Pulse_Off(void){
  IREF0CN0 = IREF0CN0_SINK__DISABLED | IREF0CN0_MDSEL__HIGH_CURRENT
                            | (0x00 << IREF0CN0_IREF0DAT__SHIFT); // Current 0 mA
}

/* Function: Biphasic
 * --------------------
 * Uninterrupted Biphasic stimulation with set parameters
 */
void Biphasic(void){
  // handle T_on division by 2 in integers
  }

void T0_Waitus (uint8_t us)
{
   TCON &= ~0x30;                      // Stop Timer0; Clear TCON_TF0
   TMOD &= ~0x0f;                      // 16-bit free run mode
   TMOD |=  0x01;

   CKCON0 |= 0x04;                      // Timer0 counts SYSCLKs

   while (us) {
      TCON_TR0 = 0;                         // Stop Timer0
      //TH0 = ((-SYSCLK/1000) >> 8);     // Overflow in 1ms
      // Overflow in 0xFC18 (64536) cycles, which for the 20 MHz is 50 us.
      TH0 = 0xFE;
      //TL0 = ((-SYSCLK/1000) & 0xFF);
      TL0 = 0x0C;
      TCON_TF0 = 0;                         // Clear overflow indicator
      TCON_TR0 = 1;                         // Start Timer0
      while (!TCON_TF0);                    // Wait for overflow
      us--;                            // Update us counter
   }

   TCON_TR0 = 0;                            // Stop Timer0
}

// Function: Biphasic_pulm
// * --------------------
// * Biphasic stimulation with the pulmonary protocol
//
void Biphasic_pulm(void){
  // start shunted
  Polarity(0);
  MUX36S16_output(0);
  //RTC_Fhz_set(20);
  while(1){
      // Handle RTC failure
      if(RTC_Failure)
      {
       RTC_Failure = 0;              // Reset RTC Failure Flag to indicate
                       // that we have detected an RTC failure
                       // and are handling the event
       // Do something...RTC Has Stopped Oscillating
       while(1);                     // <Insert Handler Code Here>
      }
      // Handle RTC Alarm
      if(RTC_Alarm)
          {

           RTC_Alarm = 0;                // Reset RTC Alarm Flag to indicate
                           // that we have detected an alarm
                           // and are handling the alarm event

           //
           isStim = !isStim;       // Change stim state

          }
      // Stimulation state. Stay awake.
      if (isStim) {



        MUX36S16_output(mux36s16_state);
        while((PMU0CF & RTCAWK) == 0); // Wait for next alarm or clock failure, then clear flags
        // Initiate interrupts. Interrupts in process until the next RTC alarm
        if(PMU0CF & RTCAWK) RTC_Alarm = 1;
        if(PMU0CF & RTCFWK) RTC_Failure = 1;
        PMU0CF = 0x20;
        if ((mux36s16_state < 16)){
            mux36s16_state ++;
            if (mux36s16_state == 13){
                mux36s16_state = 14;
            }
        }
        else {
            mux36s16_state = 0;
        }
      }
      else {
          // Interburst state. Place the device into the sleep mode
          LPM(SLEEP);  // Enter Sleep Until Next Alarm
      }
    }
}

void SDA_Reset(void)
{
    uint8_t j;                    // Dummy variable counters
    // Provide clock pulses to allow the slave to advance out
    // of its current state. This will allow it to release SDA.
    XBR1 = 0x40;                     // Enable Crossbar
    SCL = 0;                         // Drive the clock low
    for(j = 0; j < 255; j++);        // Hold the clock low
    SCL = 1;                         // Release the clock
    while(!SCL);                     // Wait for open-drain
                     // clock output to rise
    for(j = 0; j < 10; j++);         // Hold the clock high
    XBR1 = 0x00;                     // Disable Crossbar
}

void SMB_Write (void)
{
  SMB0CF &= ~0x80;                 // Reset communication
  SMB0CF |= SMB0CN0_MASTER__MASTER;
  SMB0CN0_MASTER = 0x1;                 // Force SMB0 into Master mode (preventing error)
  SMB0CN0_TXMODE = 0x1;                 // Force to transmit
  SMB0CN0_STA = 0;
  SMB0CN0_STO = 0;
  SMB0CN0_ACK = 0;
  SMB_BUSY = 0;// Free SMBus

  while (SMB_BUSY);                   // Wait for SMBus to be free.
   SMB_BUSY = 1;                       // Claim SMBus (set to busy)
   SMB_RW = 0;                         // Mark this transfer as a WRITE
   SMB0CN0_STA = 1;                            // Start transfer

   while (SMB_BUSY);
}

void SMB_Read (void)
{
  SMB0CF &= ~0x80;                 // Reset communication
  SMB0CF |= SMB0CN0_MASTER__MASTER;
  SMB0CN0_MASTER = 0x1;           // Force SMB0 into Master mode (preventing error)
  SMB0CN0_TXMODE = 0x1;           // Force to transmit
  SMB0CN0_STA = 0;
  SMB0CN0_STO = 0;
  SMB0CN0_ACK = 0;
  SMB_BUSY = 0;// Free SMBus

  while (SMB_BUSY);               // Wait for bus to be free.
   SMB_BUSY = 1;                       // Claim SMBus (set to busy)
   SMB_RW = 1;                         // Mark this transfer as a READ
   SMB0CN0_STA = 1;                            // Start transfer

   while (SMB_BUSY);               // Wait for transfer to complete
}

/*
 * Function: MUX36S16_output
 * -------------------------------
 * MUX36D08 output selection function
 * Selects output channel of the stimulation.
 *
 * mux36s16_state:switch state read from NT3H.
 * Switch state converted to corresponding bits that are set as control pins for MUX36
 * as per MUX36 truth table.
 * MUX36S16 truth table:
 * EN A3 A2 A1 A0 ON-CHANNEL
 * 0  X  X  X  X  All channels are off
 * 1 0 0 0 0 Channel 1
 * 1 0 0 0 1 Channel 2
 * 1 0 0 1 0 Channel 3
 * 1 0 0 1 1 Channel 4
 * 1 0 1 0 0 Channel 5
 * 1 0 1 0 1 Channel 6
 * 1 0 1 1 0 Channel 7
 * 1 0 1 1 1 Channel 8
 * 1 1 0 0 0 Channel 9
 * 1 1 0 0 1 Channel 10
 * 1 1 0 1 0 Channel 11
 * 1 1 0 1 1 Channel 12
 * 1 1 1 0 0 Channel 13
 * 1 1 1 0 1 Channel 14
 * 1 1 1 1 0 Channel 15
 * 1 1 1 1 1 Channel 16
 */
void MUX36S16_output(uint8_t mux36s16_state){
  P17 = (mux36s16_state & (1 << (1-1))) ? 1 : 0; // Get 1st bit of MUX36S16 state byte
  P16 = (mux36s16_state & (1 << (2-1))) ? 1 : 0; // Get 2nd bit of the state byte
  P15 = (mux36s16_state & (1 << (3-1))) ? 1 : 0; // Get 3rd bit of the state byte
  P14 = (mux36s16_state & (1 << (4-1))) ? 1 : 0; // Get 4th bit of the state byte
}

/*
 * Function: MUX36D08_output
 * -------------------------------
 * MUX36D08 output selection function
 * Selects output channel of the stimulation.
 *
 * mux36d08_state:switch state read from NT3H.
 * Switch state converted to corresponding bits that are set as control pins for MUX36
 * as per MUX36 truth table.
 * MUX36D08 truth table:
 * EN A2  A1  A0  ON-CHANNEL
 * 0  X   X   X   All channels are off
 * 1  0   0   0   Channels 1A and 1B
 * 1  0   0   1   Channels 2A and 2B
 * 1  0   1   0   Channels 3A and 3B
 * 1  0   1   1   Channels 4A and 4B
 * 1  1   0   0   Channels 5A and 5B
 * 1  1   0   1   Channels 6A and 6B
 * 1  1   1   0   Channels 7A and 7B
 * 1  1   1   1   Channels 8A and 8B
 */
void MUX36D08_output(uint8_t mux36d08_state){
  P02 = (mux36d08_state & (1 << (1-1))) ? 1 : 0; // Get 1st bit of MUX36D08 state byte
  P03 = (mux36d08_state & (1 << (2-1))) ? 1 : 0; // Get 2nd bit of the state byte
  P04 = (mux36d08_state & (1 << (3-1))) ? 1 : 0; // Get 3rd bit of the state byte
}

/*
 * Function: getByte
 * Takes the integer number and the byte number of interest
 * Returns the N-th byte of the integer. First byte starts at 0, i.e.
 * For the LSB byte (LB) N = 0, for the second byte (MSB) N =1.
 */
uint8_t
getByte (uint16_t number, uint8_t N)
{
  uint8_t x = (number >> (8 * N)) & 0xff;
  return x;
}

/*
 * Function: RTC_Fhz_set
 * Sets pulse frequency as RTC alarm to wake up the MCU at pulse generation time
 * freq - low byte of the frequency hexadecimal value
 */
void
RTC_Fhz_set (uint16_t freq) // make freq float instead of uiunt16??
{
  uint8_t alarm0; // RTC set alarm LB
  uint8_t alarm1; // RTC set alarm HB
  // Conversion to the RTC alarm value
  uint16_t F_alarm_value;
//  F_alarm_value = 16384 / freq; // 16384 is the RTC low freq oscillator. However, the RTC LFosc behaves like 32768 counter. (see RTC reference manual)
  F_alarm_value = 32768 / freq;
  // decToHexa(F_alarm_value); // convert to Hex
  // Get high and low bytes of F_alarm_value:
  alarm0 = getByte (F_alarm_value, 0);
  alarm1 = getByte (F_alarm_value, 1);
  // Re-set the RTC from the start
  // RTC oscillator control

  RTC0ADR = RTC0XCN0;
  RTC0DAT = RTC0XCN0_XMODE__SELF_OSCILLATE | RTC0XCN0_AGCEN__DISABLED
      | RTC0XCN0_BIASX2__DISABLED | RTC0XCN0_LFOEN__ENABLED;
  while ((RTC0ADR & RTC0ADR_BUSY__BMASK) == RTC0ADR_BUSY__SET)
    ;
  // Program RTC alarm value 0
  RTC0ADR = ALARM0;
  RTC0DAT = (alarm0 << ALARM0_ALARM0__SHIFT);
  while ((RTC0ADR & RTC0ADR_BUSY__BMASK) == RTC0ADR_BUSY__SET)
    ;
  // Program RTC alarm value 1
  RTC0ADR = ALARM1;
  RTC0DAT = (alarm1 << ALARM1_ALARM1__SHIFT);
  while ((RTC0ADR & RTC0ADR_BUSY__BMASK) == RTC0ADR_BUSY__SET)
    ;
  // RTC control. Prepare to be started, do not start yet.
  RTC0ADR = RTC0CN0;
  RTC0DAT = RTC0CN0_RTC0EN__ENABLED | RTC0CN0_RTC0TR__STOP
      | RTC0CN0_MCLKEN__ENABLED | RTC0CN0_RTC0AEN__DISABLED
      | RTC0CN0_ALRM__NOT_SET | RTC0CN0_RTC0CAP__NOT_SET
      | RTC0CN0_RTC0SET__NOT_SET;
  while ((RTC0ADR & RTC0ADR_BUSY__BMASK) == RTC0ADR_BUSY__SET)
    ;
}

void sampleADC(void)
{
  uint16_t mV; uint8_t mV1; uint8_t mV2;
  //uint8_t SMB_DATA_OUT[3];
  ADC0_startConversion();
  while(!ADC0_isConversionComplete()); // Wait for conversion to complete
  mV = ADC0_convertSampleToMillivolts(ADC0_getResult());
  mV1 = getByte(mV,0);
  mV2 = getByte(mV,1);
  SMB_DATA_OUT[0] = 0x00;
  SMB_DATA_OUT[1] = mV1;
  SMB_DATA_OUT[2] = mV2;
  TARGET = SLAVE_ADDR;         // I2C slave address for NT3H is 0xAA
  SMB_Write();                     // Initiate SMBus write
}

uint16_t ADC0_convertSampleToMillivolts(uint16_t sample)
{
  // The measured voltage applied to P1.7 is given by:
  //
  //                           Vref (mV)
  //   measurement (mV) =   --------------- * result (bits)
  //                       (2^10)-1 (bits)
  return ((uint32_t)sample * VREF_MV) / ADC_MAX_RESULT;
}

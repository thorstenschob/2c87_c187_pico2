/*
 * Copyright (c) 2025-2026 Thorsten Schob
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include <tusb.h>

#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"

#include "NDP_hw_io.h"


uint16_t convertOut [0x10000];
uint16_t convertIn  [0x10000];


// Value at standard clock speed (150 MHz) and Bit width: 24 bits
// Counting range: 0 to 16,777,215
// Minimum resolution:                     6.67 nanoseconds
// Maximum measurable continuous time:  ~111.8 milliseconds

void sysTickStart() {
    systick_hw->rvr = 0x00FFFFFF; 
    systick_hw->cvr = 0; 
    systick_hw->csr = 0x5; 
}

void delaySysTicks(uint32_t n) {
    volatile int32_t start, current, diff, reload;
    if (n == 0) return;
    if (n > 15000) n = 15000; 
    start  = (int32_t)systick_hw->cvr;
    reload = (int32_t)systick_hw->rvr;
    do {
        current = (int32_t)systick_hw->cvr;
        diff = start - current;
        if (diff < 0) {
            diff += reload;
        }
    } while (diff < (int32_t)n);
}


// check static inline in .h as an alternative
// __attribute__((always_inline)) static inline void ndpPeReqAckOff(void) {} // If  C187, force it to vanish instantly with zero overhead

#if (NDP_IS_C187)

void ndpFWAIT() { 
  addCycles(4);
  while( gpio_get( NDPBUSY )) { ; }                     // c187 BUSY active high
}

void ndpInitWaitPeReq(void) {
  // addCycles(4);                                      // 'hint'
  // gpio_put(HW_PEACK,0);                              // used as HW_Trigger_n_ACK 
                                                        // need for C187
  ndpWRbus(NDP_EXEPTION_MASK, EXCEPTIONPOINTERVALUE);   // TSc must be, related to DS 80C187 and so far checked with tests

  ndpWaitPeReq();

}

void ndpPeReqAckOff(void) {
  // gpio_put(HW_PEACK,1); 
}

#else

void ndpFWAIT() { 
  addCycles(4);
  while(!(gpio_get( NDPBUSY ))) {;}                     //  .. x87 BUSY pin is active low! nBUSY
}

void ndpInitWaitPeReq(void) {
  // addCycles(4);                                      // 'hint'
  // gpio_put(HW_TRIGGER, 0); addCycles(6); gpio_put(HW_TRIGGER, 1); // HW debug

  ndpWaitPeReq();
  ndpPeReqAck();
}

void ndpPeReqAck()    { gpio_put(NDPnPEACK,0); } 

void ndpPeReqAckOff() { gpio_put(NDPnPEACK,1); } 

#endif


void ndpWaitPeReq() { while(!(gpio_get( NDPPEREQ ))) { ; } }  // wait while inaktive, until active one time, than feed the fifo

bool ndpError() { return gpio_get(NDPnERROR) ? false : true ; }

void ndpRESET() {
  gpio_put(NDPRESET, 1); 
  sleep_ms(20);
  gpio_put(NDPRESET, 0);  
  sleep_ms(20);
  ndpFWAIT();
}

// The connections for the piggyback version are hard- and soft-coded as follows:
// You must definitely adapt pin definition in NDP_hw_io.h for documentation.
void init_PiggybackMap(){
    for (uint32_t word = 0; word < 0x10000; word++) {
      uint16_t wordmask = 0;
                                                                    // this ~160ns
      wordmask = (word & 0x3803u)                                   // use bits 0 1  11  12 13
              | ((word & 0x8478u) >> 1)                             // shift    3  4 5 6 10 15
              | ((word & 0x0004u) << 6)     // ?  ndpD2m : 0)       // 0x0100 // 2  7 8 9  14  bits to set
              | ((word & 0x0080u) << 3)     // ?  ndpD7m : 0)       // 0x0400
              | ((word & 0x0300u) >> 2)     // ?  ndpD8m : 0)       // 0x0080 0x0040 D9 & D8
              | ((word & 0x4000u) << 1);    // ? ndpD14m : 0)       // 0x8000 

      convertOut[word] = wordmask;
    }

    for (uint32_t wordmask = 0; wordmask < 0x10000; wordmask++) {
      uint16_t word = 0;
                                                                    // this ~160ns  ~ version in putData with shift only
      word = (wordmask & 0x3803)                                    // use bits 0 1  11  12 13
          | ((wordmask & 0x423C) << 1)                              // >> 0x8478 // shift    3  4 5 6 10 15
          | ((wordmask & 0x0100) >> 6)      // 0004 ?  ndpD2m : 0)  // 0x0100 // 2  7 8 9  14  bits to set
          | ((wordmask & 0x0400) >> 3)      // 0080 ?  ndpD7m : 0)  // 0x0400
          | ((wordmask & 0x00C0) << 2)      // 0300 ?  ndpD8m : 0)  // 0x0080 0x0040 D9 & D8
          | ((wordmask & 0x8000) >> 1);     // 4000 ? ndpD14m : 0)  // 0x8000 

      convertIn[wordmask] = word;
    }
}

void init_ControlBus() {
    gpio_set_function(NDPRESET , GPIO_FUNC_SIO); gpio_set_dir(NDPRESET,  GPIO_OUT);  gpio_put(NDPRESET,  1);    // Reset
    gpio_set_function(NDPS2    , GPIO_FUNC_SIO); gpio_set_dir(NDPS2,     GPIO_OUT);  gpio_put(NDPS2,     0);    // CS2 high active
    gpio_set_function(NDPnRD   , GPIO_FUNC_SIO); gpio_set_dir(NDPnRD,    GPIO_OUT);  gpio_put(NDPnRD,    1);    // /RD low active
    gpio_set_function(NDPnWR   , GPIO_FUNC_SIO); gpio_set_dir(NDPnWR,    GPIO_OUT);  gpio_put(NDPnWR,    1);    // /WR low active
    gpio_set_function(NDPCMD0  , GPIO_FUNC_SIO); gpio_set_dir(NDPCMD0,   GPIO_OUT);  gpio_put(NDPCMD0,   0);    // CMD0      
    gpio_set_function(NDPCMD1  , GPIO_FUNC_SIO); gpio_set_dir(NDPCMD1,   GPIO_OUT);  gpio_put(NDPCMD1,   0);    // CMD1

    #if (NDP_IS_C187)
      #if (NDP_HW_TRIGGER_OPT)              // option: second trigger signal ||, == NDPnPEACK output
        gpio_set_function(NDPnPEACK, GPIO_FUNC_SIO); gpio_set_dir(NDPnPEACK, GPIO_OUT);  gpio_put(NDPnPEACK, 1);  // used as HW_n_ACK     
      #else
        gpio_set_function(NDPnPEACK, GPIO_FUNC_SIO); gpio_set_dir(NDPnPEACK, GPIO_IN);                            // Pin 17 n.c., input
      #endif
    #else
      gpio_set_function(NDPnPEACK, GPIO_FUNC_SIO); gpio_set_dir(NDPnPEACK, GPIO_OUT);  gpio_put(NDPnPEACK, 1);    // /PEACK
      // gpio_set_slew_rate(NDPnPEACK,  GPIO_SLEW_RATE_FAST);
    #endif

    /*
    gpio_set_slew_rate(NDPRESET ,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(NDPS2    ,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(NDPnRD   ,  GPIO_SLEW_RATE_FAST);  
    gpio_set_slew_rate(NDPnWR   ,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(NDPCMD0  ,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(NDPCMD1  ,  GPIO_SLEW_RATE_FAST);
    */
    gpio_set_function(NDPBUSY ,  GPIO_FUNC_SIO); gpio_set_dir(NDPBUSY,   GPIO_IN); gpio_pull_up(NDPBUSY);
    gpio_set_function(NDPPEREQ,  GPIO_FUNC_SIO); gpio_set_dir(NDPPEREQ,  GPIO_IN); gpio_pull_up(NDPPEREQ);
    gpio_set_function(NDPnERROR, GPIO_FUNC_SIO); gpio_set_dir(NDPnERROR, GPIO_IN); gpio_pull_up(NDPnERROR);

    gpio_set_input_hysteresis_enabled(NDPBUSY, true);
    gpio_set_input_hysteresis_enabled(NDPPEREQ, true);
    gpio_set_input_hysteresis_enabled(NDPnERROR, true);
}

void init_DataBus() {
    gpio_set_function(ndpD0,  GPIO_FUNC_SIO);
    gpio_set_function(ndpD1,  GPIO_FUNC_SIO);
    gpio_set_function(ndpD2,  GPIO_FUNC_SIO); 
    gpio_set_function(ndpD3,  GPIO_FUNC_SIO);
    gpio_set_function(ndpD4,  GPIO_FUNC_SIO);
    gpio_set_function(ndpD5,  GPIO_FUNC_SIO);
    gpio_set_function(ndpD6,  GPIO_FUNC_SIO);
    gpio_set_function(ndpD7,  GPIO_FUNC_SIO);
    gpio_set_function(ndpD8,  GPIO_FUNC_SIO);
    gpio_set_function(ndpD9,  GPIO_FUNC_SIO);
    gpio_set_function(ndpD10, GPIO_FUNC_SIO);
    gpio_set_function(ndpD11, GPIO_FUNC_SIO);
    gpio_set_function(ndpD12, GPIO_FUNC_SIO);
    gpio_set_function(ndpD13, GPIO_FUNC_SIO);
    gpio_set_function(ndpD14, GPIO_FUNC_SIO);
    gpio_set_function(ndpD15, GPIO_FUNC_SIO);
    
    /*
    gpio_set_slew_rate(ndpD0,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD1,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD2,  GPIO_SLEW_RATE_FAST);  
    gpio_set_slew_rate(ndpD3,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD4,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD5,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD6,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD7,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD8,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD9,  GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD10, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD11, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD12, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD13, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD14, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(ndpD15, GPIO_SLEW_RATE_FAST);
    */
    gpio_set_input_hysteresis_enabled(ndpD0,  true);
    gpio_set_input_hysteresis_enabled(ndpD1,  true);
    gpio_set_input_hysteresis_enabled(ndpD2,  true);
    gpio_set_input_hysteresis_enabled(ndpD3,  true);
    gpio_set_input_hysteresis_enabled(ndpD4,  true);
    gpio_set_input_hysteresis_enabled(ndpD5,  true);
    gpio_set_input_hysteresis_enabled(ndpD6,  true);
    gpio_set_input_hysteresis_enabled(ndpD7,  true);
    gpio_set_input_hysteresis_enabled(ndpD8,  true);
    gpio_set_input_hysteresis_enabled(ndpD9,  true);
    gpio_set_input_hysteresis_enabled(ndpD10, true);
    gpio_set_input_hysteresis_enabled(ndpD11, true);
    gpio_set_input_hysteresis_enabled(ndpD12, true);
    gpio_set_input_hysteresis_enabled(ndpD13, true);
    gpio_set_input_hysteresis_enabled(ndpD14, true);
    gpio_set_input_hysteresis_enabled(ndpD15, true);
}

void pull_up_DataBus() {
    
    gpio_pull_up(ndpD0);
    gpio_pull_up(ndpD1); 
    gpio_pull_up(ndpD2);   
    gpio_pull_up(ndpD3);
    gpio_pull_up(ndpD4); 
    gpio_pull_up(ndpD5);
    gpio_pull_up(ndpD6);
    gpio_pull_up(ndpD7);
    gpio_pull_up(ndpD8);
    gpio_pull_up(ndpD9);
    gpio_pull_up(ndpD10);
    gpio_pull_up(ndpD11);
    gpio_pull_up(ndpD12); 
    gpio_pull_up(ndpD13);
    gpio_pull_up(ndpD14); 
    gpio_pull_up(ndpD15);
    // or
    /*
    gpio_pull_down(ndpD0);
    gpio_pull_down(ndpD1); 
    gpio_pull_down(ndpD2);   
    gpio_pull_down(ndpD3);
    gpio_pull_down(ndpD4); 
    gpio_pull_down(ndpD5);
    gpio_pull_down(ndpD6);
    gpio_pull_down(ndpD7);
    gpio_pull_down(ndpD8);
    gpio_pull_down(ndpD9);
    gpio_pull_down(ndpD10);
    gpio_pull_down(ndpD11);
    gpio_pull_down(ndpD12); 
    gpio_pull_down(ndpD13);
    gpio_pull_down(ndpD14); 
    gpio_pull_down(ndpD15);
  */
}

void disable_pulls_temp_DataBus() {
    gpio_disable_pulls(ndpD0);
    gpio_disable_pulls(ndpD1); 
    gpio_disable_pulls(ndpD2);   
    gpio_disable_pulls(ndpD3);
    gpio_disable_pulls(ndpD4); 
    gpio_disable_pulls(ndpD5);
    gpio_disable_pulls(ndpD6);
    gpio_disable_pulls(ndpD7);
    gpio_disable_pulls(ndpD8);
    gpio_disable_pulls(ndpD9);
    gpio_disable_pulls(ndpD10);
    gpio_disable_pulls(ndpD11);
    gpio_disable_pulls(ndpD12); 
    gpio_disable_pulls(ndpD13);
    gpio_disable_pulls(ndpD14); 
    gpio_disable_pulls(ndpD15);
}


uint16_t getDataBus() {
// gpio_put(HW_TRIGGER, 0); // HW debug
  uint32_t wordmask = gpio_get_all();                     // !!! short ~>0ns!
  uint16_t word = convertIn[(uint16_t)wordmask];          // <> = (uint16_t)wordmask;
    /*
                                                          // this ~160ns  ~ version in putData with shift only
    word =  (wordmask & 0x3803)                           // use bits 0 1  11  12 13
         | ((wordmask & 0x423C) << 1)                     // >> 0x8478 // shift    3  4 5 6 10 15
         | ((wordmask & 0x0100) >> 6)        // 0004 ?  ndpD2m : 0)  // 0x0100 // 2  7 8 9  14  bits to set
         | ((wordmask & 0x0400) >> 3)        // 0080 ?  ndpD7m : 0)  // 0x0400
         | ((wordmask & 0x00C0) << 2)        // 0300 ?  ndpD8m : 0)  // 0x0080 0x0040 D9 & D8
         | ((wordmask & 0x8000) >> 1);       // 4000 ? ndpD14m : 0)  // 0x8000 
    */
//  gpio_put(HW_TRIGGER, 1); // HW debug
    return word;
}

void putDataBus(uint16_t word) { 
  uint32_t wordmask = convertOut[word];                   // <> = word
  gpio_put_masked (ndpDBUSMASK, wordmask);                //
  /*
  uint32_t wordmask = 0;                                   // this ~160ns
  wordmask =  (word & 0x3803u)                             // use bits 0 1  11  12 13
           | ((word & 0x8478u) >> 1)                       // shift    3  4 5 6 10 15
           | ((word & 0x0004u) << 6)    // ?  ndpD2m : 0)  // 0x0100 // 2  7 8 9  14  bits to set
           | ((word & 0x0080u) << 3)    // ?  ndpD7m : 0)  // 0x0400
           | ((word & 0x0300u) >> 2)    // ?  ndpD8m : 0)  // 0x0080 0x0040 D9 & D8
           | ((word & 0x4000u) << 1);   // ? ndpD14m : 0)  // 0x8000 
  */ 
}

//---------------------------------------------------------------------------------------------------
// The next NDP basic I/O functions refer to Table 'Bus Cycles Definition' and 'I/O Address Decoding'


// ndpWRcode - 0x00F8 Opcode Write to 80C187 (instruction code)
// 
void ndpWRcode(uint16_t code) {                         // adr 00b; Code in Maschinenprogramm c't 1987 Heft 3 Seite 88 
                                                        // wird sequentiel gelistet, D9/D8/.. 'ESC001 Byte 0' zuerst, dann 2tr Teil 'Byte 1' an höherer Speicherstelle, ist aber ansonsten little endian,   
                                                        // daher tausch der Byte-Reihenfolge hier, Maschienenkode wird immer so gelistet, auch in Wikipedia und Disassemblern.
                                                        // or uint16_t __builtin_bswap16(uint16_t x) { return (x << 8) | (x >> 8);
                                                        // Most Debuggers display memory dumps in big-endian format.
// gpio_put(HW_TRIGGER, 1); // HW debug
  gpio_put_masked (ndpABUSMASK, 0);                     // adr 00b assume
  gpio_put(NDPS2, 1);
  addCycles(1); 
  
  gpio_put(NDPnWR, 0);

  gpio_put_masked (ndpDBUSMASK, convertOut[(uint16_t)(code << 8) | (code >> 8)]);  // <> gpio_put_masked (ndpDBUSMASK, (uint16_t)(code << 8) | (code >> 8));
  addCycles(5);                                         // 5 ~175ns, 6 ~195ns

  gpio_put(NDPnWR, 1);
  addCycles(1);

  gpio_put(NDPS2, 0);
  gpio_put_masked (ndpDBUSMASK, ndpDBUSMASK);           // putDataBus( 0xFFFF);
  gpio_put_masked (ndpABUSMASK, 0);                     // putAdrBus( 0x00)

// gpio_put(HW_TRIGGER, 0); // HW debug
}


void ndpWRbus(uint32_t adrmsk, uint16_t code) {         // adr 00b; Code in Maschinenprogramm c't 1987 Heft 3 Seite 88 
// gpio_put(HW_TRIGGER, 0); // HW debug

  gpio_put_masked (ndpABUSMASK, adrmsk);
  gpio_put(NDPS2, 1);

  addCycles(1); 
                                                  
  gpio_put(NDPnWR, 0);
  putDataBus(code);

  addCycles(5);                                         // NDPWR Active Time // 5 ~175ns, 6 ~195ns

  gpio_put(NDPnWR, 1); 
  addCycles(1);

  gpio_put(NDPS2, 0);
  gpio_put_masked (ndpDBUSMASK, ndpDBUSMASK);           // putDataBus( 0xFFFF);
  gpio_put_masked (ndpABUSMASK, 0);                     // putAdrBus( 0x00)

// gpio_put(HW_TRIGGER, 1); // HW debug
}

uint16_t  ndpRDbus(uint32_t adrmsk) { 
// gpio_put(HW_TRIGGER, 0); // HW debug
  
  gpio_put_masked (ndpABUSMASK, adrmsk);

  gpio_set_dir_in_masked (ndpDBUSMASK); 
  gpio_put(NDPS2, 1);    

  addCycles(1); 

  gpio_put(NDPnRD, 0);
  addCycles(8);                                         // /NDPRD Active to Data Valid; 8 ~170..175 ns, 7 ~155ns  5 was with 'mistake'~135ns

  uint32_t wordmask = gpio_get_all();
  gpio_put(NDPnRD, 1); 

  volatile uint16_t data  = convertIn[(((uint16_t)wordmask) & ndpDBUSMASK)];  // <> = (((uint16_t)wordmask) & ndpDBUSMASK);
   
  gpio_put(NDPS2, 0);
  addCycles(1); 

  gpio_set_dir_out_masked (ndpDBUSMASK);
  gpio_put_masked (ndpDBUSMASK, ndpDBUSMASK);           // putDataBus( 0xFFFF);
  gpio_put_masked (ndpABUSMASK, 0);                     // putAdrBus( 0x00)

// gpio_put(HW_TRIGGER, 1); // HW debug
  return data;
}

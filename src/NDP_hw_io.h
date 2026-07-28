/*
 * Copyright (c) 2025-2026 Thorsten Schob
 * SPDX-License-Identifier: MIT
 */

#ifndef _NDP_HW_IO_H
#define _NDP_HW_IO_H

#include <stdint.h>           // Required for uint8_t, uint16_t, etc.
#include <stdbool.h>

#ifndef NDP_IS_C187
#define NDP_IS_C187 1         //_AN           
#endif

#define EXCEPTIONPOINTERVALUE 0x0000        // ExceptionPointers = 0 'hint', just for C187 


// The following is simply a practical suggestion for the development phase.
// #define NDPRESET   18      // , alternatively, only this line is required for the NDP

#define NDP_HW_TRIGGER_OPT 0  // HW test  option, optimized for use with logic analyzer (stacked on a Pivo 2 for prototypes)
                              //        gpio_put(HW_TRIGGER, 0); // HW debug - parallel to TP5' gpio_put(PICO_DEFAULT_LED_PIN, 0);
                              // also:  gpio_put(HW_PEACK, 0); delayCycles(1); gpio_put(HW_PEACK, 1); // HW debug
#if (NDP_HW_TRIGGER_OPT)      
                              // optimized option for use with logic analyzer
  #define HW_TRIGGER  18      // for Logicanalyzer stacked || on this Pico 2
  #define NDPRESET    25      // is wired to GPIO 25 at TP5' - modification of: "TP5 GPIO25/LED (not recommended to be used)"
  //#define HW_PEACK  17      // option: second trigger signal ||, == NDPnPEACK output
#else
                              // original piggy back of prototype
  #define HW_TRIGGER  25      // is wired to GPIO 25 at TP5 - modification of: "TP5 GPIO25/LED (not recommended to be used)"
  #define NDPRESET    18      // connection x87 'piggyback' socket to the GPIO 18
#endif

// &
#define NDPnPEACK     17      // no function for C187, then output for HW debug '2' use; for x87 necessary!

//      NDPnS1                // can left on low      - is tied to GND
#define NDPS2         19      // can assigned to high 
#define NDPCMD0       22
#define NDPCMD1       21
#define NDPnRD        26
#define NDPnWR        16

#define NDPBUSY       20      // c187 BUSY active high  ... x87 nBUSY active low!
#define NDPPEREQ      28
#define NDPnERROR     27

// The connections for the piggyback version are hard wired- and soft-coded as follows:
//                            
#define ndpD0      0          // You must definitely adapt: init_PiggybackMap(){ ... in NDP_hw_io.c
#define ndpD1      1
#define ndpD2      8
#define ndpD3      2
#define ndpD4      3
#define ndpD5      4
#define ndpD6      5 
#define ndpD7     10
#define ndpD8      6 
#define ndpD9      7
#define ndpD10     9 
#define ndpD11    11
#define ndpD12    12
#define ndpD13    13
#define ndpD14    15
#define ndpD15    14          // You must definitely adapt: init_PiggybackMap(){ ... in NDP_hw_io.c

#define ndpDBUSMASK   0x0FFFF
// #define ndpDBUSMASK2  1 << ndpD15 | 1 << ndpD14 | 1 << ndpD13 | 1 << ndpD12 | 1 << ndpD11 | 1 << ndpD10 | 1 << ndpD9 | 1 << ndpD8 | 1 << ndpD7 | 1 << ndpD6 | 1 << ndpD5 | 1 << ndpD4 | 1 << ndpD3 | 1 << ndpD2 | 1 << ndpD1 | 1 << ndpD0 
// copy:                             |             |             |             |             |             |            |            |            |            |            |            |            |            |            |         ^^
// OT draft at the beginning of the project

#define ndpABUSMASK       (1 << NDPCMD1 | 1 << NDPCMD0)
                                                        //  Table 11. I/O Address Decoding (DS 80C187)
#define NDP_OPCODE_MASK    0                            //  '00F8' Opcode   Write to x87     adr. 00b
#define NDP_CWSW_MASK      0                            //  '00F8' CW or SW Read from x87    adr. 00b
#define NDP_DATA_MASK     (1 << NDPCMD0)                //  '00FA' Data, Read/Write x87      adr. 01b
#define NDP_EXEPTION_MASK (1 << NDPCMD1)                //  '00FC' Write Exception Pointers  adr. 10b
#define NDP_OPSTATUS_MASK   ndpABUSMASK                 //  '00FE' Read Opcode Status        adr. 11b

// 
// ndpWRcode       - 0x00F8 Opcode Write to 80C187 (instruction code) ; is byte swaped
// void ndpWRcode(uint16_t code); see in _io.c !        // adr. 00b   ; like most Listings, see in _io.c !
// 
// ndpRDcwsw       - 0x00F8  CW or SW Read from 80C187  // adr. 00b ndpRDbus(0, 0);
// ndpRDdata       - 0x00FA  Read Data from 80C187      // adr. 01b ndpRDbus(0, 1);
// ndpRDstatus     - 0x00FE  Read Data from 80C187      // adr. 11b ndpRDbus(1, 1);
// ndpWRdata()     - 0x00FA  Write Data to 80C187       // adr. 01b ndpWRbus(data, 0, 1);
// ndpWRexception  - 0x00FC  Write Exception Pointers   // adr. 10b ndpWRbus(data, 1, 0);


// Micro-gap delays (1 to 4 exact cycles)
__attribute__((always_inline)) static inline void addCycl_1t() { __asm__ volatile ("nop"); }
__attribute__((always_inline)) static inline void addCycl_2t() { __asm__ volatile ("nop\n\t" "nop"); }
__attribute__((always_inline)) static inline void addCycl_3t() { __asm__ volatile ("nop\n\t" "nop\n\t" "nop"); }
__attribute__((always_inline)) static inline void addCycl_4t() { __asm__ volatile ("nop\n\t" "nop\n\t" "nop\n\t" "nop"); }

// Loop delay (3n + 1 cycles)
__attribute__((always_inline)) static inline void addCycles(uint32_t n) {   // Loop delay (3n + 1 cycles)
    if (n == 0) return;
    __asm__ volatile (
        "1:\n\t"
        "subs %0, %0, #1\n\t"   // (1 cycle)
        "bne 1b\n\t"            // (1-3 cycles)*
        : "+r" (n)
        :
        : "cc"                  // Tell the compiler: Condition codes (flags) are changing
    );
}

void sysTickStart();

void ndpFWAIT();

// void ndpInitExecution(void); // ... at the end of this file

void ndpInitWaitPeReq(void);

void ndpPeReqAck(); 
void ndpPeReqAckOff(void);

void ndpWaitPeReq();
bool ndpError();

void ndpRESET();

void init_PiggybackMap();
void init_ControlBus();

void init_DataBus();

void pull_up_DataBus();
// void disable_pulls_temp_DataBus();
uint16_t getDataBus();

void putDataBus(uint16_t word);


void     ndpWRcode(uint16_t code) ; // Opcode Write to NDP x87 (instruction code); byte is swaped
                                    // like most Listings, see in NDP_hw_io.c !
void     ndpWRbus(uint32_t adrmsk, uint16_t code);
uint16_t ndpRDbus(uint32_t adrmsk);

// void ndpInitExecution(void);
#if (NDP_IS_C187)               // TSc must be related to Datasheet C187 and so far checked with tests
  __attribute__((always_inline)) static inline void ndpInitExecution(void) { ndpWRbus(NDP_EXEPTION_MASK, EXCEPTIONPOINTERVALUE); }
#else
  __attribute__((always_inline)) static inline void ndpInitExecution(void) {}
#endif

#endif

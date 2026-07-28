/*
 * Copyright (c) 2025-2026 Thorsten Schob
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "pico/stdlib.h"

#include "NDP_hw_io.h"
#include "NDP_x87.h"


const uint32_t L_10p[] = {
  0x1,                    // 10^0  = 1 
  0xA,                    // 10^1  = 10
  0x64,                   // 10^2  = 100
  0x3E8,                  // 10^3  = 1000
  0x2710,                 // 10^4  = 10000
  0x186A0,                // 10^5  = 100.000
  0xF4240,                // 10^6  = 1.000.000
  0x989680,               // 10^7  = 10.000.000
  0x5F5E100,              // 10^8  = 100.000.000
  0x3B9ACA00              // 10^9  = 1.000.000.000
};

const uint64_t LL_10p[] = {
  0x1ULL,                 // 10^0  = 1
  0xAULL,                 // 10^1  = 10
  0x64ULL,                // 10^2  = 100
  0x3E8ULL,               // 10^3  = 1k [kilo]  = 1 Thousand      [DE: 1 Tausend]
  0x2710ULL,              // 10^4  = 10k
  0x186A0ULL,             // 10^5  = 100k
  0xF4240ULL,             // 10^6  = 1M [mega]  = 1 Million       [DE: 1 Million]
  0x989680ULL,            // 10^7  = 10M
  0x5F5E100ULL,           // 10^8  = 100M
  0x3B9ACA00ULL,          // 10^9  = 1G [giga]  = 1 Billion       [DE: 1 Milliarde]
  0x2540BE400ULL,         // 10^10 = 10G
  0x174876E800ULL,        // 10^11 = 100G
  0xE8D4A51000ULL,        // 10^12 = 1T [tera]  = 1 Trillion      [DE: 1 Billion]
  0x9184E72A000ULL,       // 10^13 = 10T
  0x5AF3107A4000ULL,      // 10^14 = 100T
  0x38D7EA4C68000ULL,     // 10^15 = 1P [peta]  = 1 Quadrillion   [DE: 1 Billiarde]
  0x2386F26FC10000ULL,    // 10^16 = 10P
  0x16345785D8A0000ULL,   // 10^17 = 100P
  0xDE0B6B3A7640000ULL,   // 10^18 = 1E [exa]   = 1 Quintillion   [DE: 1 Trillion]
  0x8AC7230489E80000ULL   // 10^19 = 10E
};

// Do not use this for decimal shifting. - just for tests.
// --- FP32 (Single Precision) Sequential Table ...
const uint32_t Sm32fp_10p[] = {
    0x3F800000UL,  // 10^0  = 1
    0x41200000UL,  // 10^1  = 10
    0x42C80000UL,  // 10^2  = 100
    0x44480000UL,  // 10^3  = 1000
    0x461C4000UL,  // 10^4  = 10000
    0x47C35000UL,  // 10^5  = 100000
    0x49742400UL,  // 10^6  = 1000000
    0x4B189680UL,  // 10^7  = 10.000.000
    0x4CBEBC20UL,  // 10^8  ...
    0x4E6E6B28UL,  // 10^9  
    0x501502F9UL,  // 10^10 
    0x51A43CF0UL,  // 10^11 
    0x534D4940UL,  // 10^12 
    0x55007E30UL,  // 10^13 
    0x56A10C24UL,  // 10^14 
    0x584E72A0UL,  // 10^15 
    0x5A0E1BCAUL,  // 10^16 
    0x5A8D4A51UL,  // 10^17 
    0x5B18E441UL   // 10^18 
};


// Drive of the 80C187 as a peripheral device, similar to the application note
//___________________________________________________________________________________________________

// uses ndpWRcode: Opcode Write to 80C187 (instruction code); is byte swaped
// void ndpWRcode(uint16_t code); see in NDP_hw_io.c        ; like most listings, see in _io.c !
//
// ndpInitExecution solves the needs of C187 with write to the exception pointer:
//     "For most instructions, the NPX does not start executing the previously transferred opcode
//      until the CPU  first writes exception pointer information to port 00FCH of the NDP. "

void ndp_WrCmd(uint16_t cmd) {
  ndpFWAIT();
  ndpWRcode(cmd);

  ndpInitExecution();                                   // need for C187, or no code (less than 'nop')
}

void ndp_WrCmdReg(uint16_t cmd, uint8_t reg) {
  ndpFWAIT();
  ndpWRcode(cmd | (reg & 0x07));

  ndpInitExecution();                                   // need for C187, or no code (less than 'nop')
}

uint16_t ndp_WRcodeRDcwsw(uint16_t cmd) {
  uint16_t d;
  ndpFWAIT();
  ndpWRcode(cmd);

  ndpInitExecution();                                   // need for C187, or no code (less than 'nop')
  d = ndpRDbus(NDP_CWSW_MASK);
  return d;
}


__attribute__((always_inline)) inline void ndp(uint16_t opc) {ndp_WrCmd(opc);}
__attribute__((always_inline)) inline void ndp_(uint16_t opc, uint8_t reg) { ndp_WrCmdReg(opc, reg); } 

// ndp_WRcodeRDcwsw(FSTSWR) 
__attribute__((always_inline)) inline uint16_t ndp_get(uint16_t opc) { return ndp_WRcodeRDcwsw(opc); }

void ndp_init(void) {                                   // WIP, F-Restore or set 'nan'
  ndp(FINIT);                                       
  for (int r =0; r < 8; r++) {ndp(FLDZ);  }             // set all to zero  
  ndp(FINIT);                                           // mark FPU stack empty
}


void ndp_WrWm16(uint16_t opc, const uint16_t *ptr) {    // 'W'  Word Integer in memory to FP register
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, *ptr);
  ndpPeReqAckOff();
}

void ndp_WrLm32(uint16_t opc, const uint32_t *ptr) {    // 'L'  Long Word Integer in memory to FP register 
  const uint16_t *w = (const uint16_t *)ptr;
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, w[0]);                        // low word
  ndpPeReqAckOff();
  ndpWRbus(NDP_DATA_MASK, w[1]);                        // high word
}

void ndp_WrSm32(uint16_t opc, const union m32fp *ptr) { // 'S'  Single Precision Real in memory to FP register 
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, ptr->word0);                  // ndpWRbus(NDP_DATA_MASK, (uint16_t)((*ptr).word0));
  ndpPeReqAckOff();
  ndpWRbus(NDP_DATA_MASK, ptr->word1);                  // ndpWRbus(NDP_DATA_MASK, (uint16_t)((*ptr).word1));
}

void ndp_WrLLm64(uint16_t opc, const uint64_t *ptr) {   // 'LL' _m64, int64_t in memory to FP register 
  const uint16_t *w = (const uint16_t *)ptr;
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, w[0]);
  ndpPeReqAckOff();
  ndpWRbus(NDP_DATA_MASK, w[1]);
  ndpWRbus(NDP_DATA_MASK, w[2]);
  ndpWRbus(NDP_DATA_MASK, w[3]);
}

void ndp_WrDm64(uint16_t opc, const union m64fp *ptr) { // 'D'  Double Precision Real in memory to FP register 
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, ptr->word0);
  ndpPeReqAckOff();
  ndpWRbus(NDP_DATA_MASK, ptr->word1);
  ndpWRbus(NDP_DATA_MASK, ptr->word2);
  ndpWRbus(NDP_DATA_MASK, ptr->word3);   
}

void ndp_WrXm80(uint16_t opc, const union m80 *ptr) {   // 'X'  Extended Precision Real in memory to FP register  
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, ptr->word0);
  ndpPeReqAckOff();
  ndpWRbus(NDP_DATA_MASK, ptr->word1);
  ndpWRbus(NDP_DATA_MASK, ptr->word2);
  ndpWRbus(NDP_DATA_MASK, ptr->word3);
  ndpWRbus(NDP_DATA_MASK, ptr->word4);
}

// Motorola Packed BCD 
void ndp_WrPm80(uint16_t opc, const union m80pBCD *ptr) { // Motorola pBCD  Packed BCD 'mantisse integer' in memory to FP register  
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, ptr->word4);        // MS
  ndpPeReqAckOff();
  ndpWRbus(NDP_DATA_MASK, ptr->word3);
  ndpWRbus(NDP_DATA_MASK, ptr->word2);
  ndpWRbus(NDP_DATA_MASK, ptr->word1);
  ndpWRbus(NDP_DATA_MASK, ptr->word0);        // LS
}


void ndp_RdWm16(uint16_t opc, uint16_t *ptr) {          // 'W'  Word Integer FP register value to memory operation
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ptr[0] = ndpRDbus(NDP_DATA_MASK);                     // ptr[0] =,  *ptr =  are equivalent
  ndpPeReqAckOff();
}

void ndp_RdLm32(uint16_t opc, uint32_t *ptr) {          // 'L'  Long Word Integer FP register value to memory operation
  uint16_t *w = (uint16_t *)ptr;
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  w[0] = ndpRDbus(NDP_DATA_MASK);
  ndpPeReqAckOff();
  w[1] = ndpRDbus(NDP_DATA_MASK);
}

void ndp_RdSm32(uint16_t opc, union m32fp *ptr) {       // 'S'  Single Precision Real FP register value to memory operation      
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ptr->word0 = ndpRDbus(NDP_DATA_MASK);
  ndpPeReqAckOff();
  ptr->word1 = ndpRDbus(NDP_DATA_MASK);
}

void ndp_RdLLm64(uint16_t opc, uint64_t *ptr) {         // 'LL' _m64, int64_t  FP register value to memory operation
  uint16_t *w = (uint16_t *)ptr;
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  w[0] = ndpRDbus(NDP_DATA_MASK);
  ndpPeReqAckOff();
  w[1] = ndpRDbus(NDP_DATA_MASK);
  w[2] = ndpRDbus(NDP_DATA_MASK);
  w[3] = ndpRDbus(NDP_DATA_MASK);
}

void ndp_RdDm64(uint16_t opc, union m64fp *ptr) {       // 'D'  Double Precision Real FP register value to memory operation
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ptr->word0 = ndpRDbus(NDP_DATA_MASK);
  ndpPeReqAckOff();
  ptr->word1 = ndpRDbus(NDP_DATA_MASK);
  ptr->word2 = ndpRDbus(NDP_DATA_MASK);
  ptr->word3 = ndpRDbus(NDP_DATA_MASK);
}

void ndp_RdXm80(uint16_t opc, union m80 *ptr) {         // 'X'  Extended Precision Real FP register value to memory operation
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ptr->word0 = ndpRDbus(NDP_DATA_MASK);
  ndpPeReqAckOff();
  ptr->word1 = ndpRDbus(NDP_DATA_MASK);
  ptr->word2 = ndpRDbus(NDP_DATA_MASK);
  ptr->word3 = ndpRDbus(NDP_DATA_MASK);
  ptr->word4 = ndpRDbus(NDP_DATA_MASK);
}

// Motorola Packed BCD 
void ndp_RdPm80(uint16_t opc, union m80pBCD *ptr) {     //  Motorola Packed BCD - FP register value to memory operation
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ptr->word4 = ndpRDbus(NDP_DATA_MASK);                 // MS
  ndpPeReqAckOff();
  ptr->word3 = ndpRDbus(NDP_DATA_MASK);
  ptr->word2 = ndpRDbus(NDP_DATA_MASK);
  ptr->word1 = ndpRDbus(NDP_DATA_MASK);
  ptr->word0 = ndpRDbus(NDP_DATA_MASK);                 // LS
}


void ndp_fildW_m16(const uint16_t *ptr)       { ndp_WrWm16(FILDW_m16, ptr); }    // 'W'  Word Integer in memory to FP register

void ndp_fildL_m32(const uint32_t *ptr)       { ndp_WrLm32(FILDL_m32, ptr); }    // 'L'  Long Word Integer in memory to FP register 

void ndp_fldS_m32(const union m32fp *ptr)     { ndp_WrSm32(FLDS_m32fp, ptr); }   // 'S'  Single Precision Real in memory to FP register 

void ndp_fildLL_m64(const uint64_t *ptr)      { ndp_WrLLm64(FILDLL_m64, ptr); }  // 'LL' _m64, int64_t in memory to FP register 

void ndp_fldD_m64fp(const union m64fp *ptr)   { ndp_WrDm64(FLDD_m64fp, ptr); }   // 'D'  Double Precision Real in memory to FP register 

void ndp_fldX_m80(const union m80 *ptr)       { ndp_WrXm80(FLDX_m80, ptr); }     // 'X'  Extended Precision Real in memory to FP register  

void ndp_fld_m80pBCD(const union m80pBCD *ptr){ ndp_WrPm80 (FBLD_m80BCD, ptr); } //   Motorola Packed BCD in memory to FP register


void ndp_fistW_m16(uint16_t *ptr)             { ndp_RdWm16(FISTW_m16, ptr); }    // 'W'  Word Integer FP register value to memory operation

void ndp_fistpW_m16(uint16_t *ptr)            { ndp_RdWm16(FISTPW_m16, ptr); }  

void ndp_fistL_m32(uint32_t *ptr)             { ndp_RdLm32(FISTL_m32, ptr); }    // 'L'  Long Word Integer FP register value to memory operation

void ndp_fistpL_m32(uint32_t *ptr)            { ndp_RdLm32(FISTPL_m32, ptr); }

void ndp_fstS_m32fp(union m32fp *ptr)         { ndp_RdSm32(FSTS_m32fp, ptr); }   // 'S'  Single Precision Real FP register value to memory operation

void ndp_fstpS_m32fp(union m32fp *ptr)        { ndp_RdSm32(FSTPS_m32fp, ptr); }

void ndp_fistpLL_m64(uint64_t *ptr)           { ndp_RdLLm64(FISTPLL_m64, ptr); } // 'LL' _m64, int64_t  FP register value to memory operation

void ndp_fstD__m64fp(union m64fp *ptr)        { ndp_RdDm64(FSTD_m64fp, ptr); }   // 'D'  Double Precision Real FP register value to memory operation

void ndp_fstpD__m64fp(union m64fp *ptr)       { ndp_RdDm64(FSTPD_m64fp, ptr); } 

void ndp_fstPX_m80fp(union m80 *ptr)          { ndp_RdXm80(FSTPX_m80, ptr); }    // 'X'  Extended Precision Real FP register value to memory operation

void ndp_fstp_m80pBCD(union m80pBCD *ptr) { ndp_RdPm80 (FBSTP_m80BCD , ptr); }   //  Motorola Packed BCD - FP register value to memory operation

                                                                                 // x87  instruction wrappers Comparison (memory operand vs. ST(0))
void ndp_ficomW_m16(const uint16_t *ptr)      { ndp_WrWm16(FICOMW_m16,  ptr); }  // FCOM  'W'  compare, no pop
void ndp_ficomL_m32(const uint32_t *ptr)      { ndp_WrLm32(FICOML_m32,  ptr); }  // FCOM  'L'
void ndp_fcomS_m32fp(const union m32fp *ptr)  { ndp_WrSm32(FCOMS_m32fp, ptr); }  // FCOM  'S'
void ndp_fcomD_m64fp(const union m64fp *ptr)  { ndp_WrDm64(FCOMD_m64fp, ptr); }  // FCOM  'D'

void ndp_ficompW_m16(const uint16_t *ptr)     { ndp_WrWm16(FICOMPW_m16,  ptr); } // FCOMP 'W'  compare & pop
void ndp_ficompL_m32(const uint32_t *ptr)     { ndp_WrLm32(FICOMPL_m32,  ptr); } // FCOMP 'L'
void ndp_fcompS_m32fp(const union m32fp *ptr) { ndp_WrSm32(FCOMPS_m32fp, ptr); } // FCOMP 'S'
void ndp_fcompD_m64fp(const union m64fp *ptr) { ndp_WrDm64(FCOMPD_m64fp, ptr); } // FCOMP 'D'

                                                                                 // x87  instruction wrappers Arithmetic (memory operand -> ST(0))
void ndp_fiaddW_m16(const uint16_t *ptr)      { ndp_WrWm16(FIADDW_m16,  ptr); }  // FADD  'W'  Word Integer
void ndp_fiaddL_m32(const uint32_t *ptr)      { ndp_WrLm32(FIADDL_m32,  ptr); }  // FADD  'L'  Long Word Integer
void ndp_faddS_m32fp(const union m32fp *ptr)  { ndp_WrSm32(FADDS_m32fp, ptr); }  // FADD  'S'  Single Precision Real
void ndp_faddD_m64fp(const union m64fp *ptr)  { ndp_WrDm64(FADDD_m64fp, ptr); }  // FADD  'D'  Double Precision Real

void ndp_fisubW_m16(const uint16_t *ptr)      { ndp_WrWm16(FISUBW_m16,  ptr); }  // FSUB  'W'  ST(0) = ST(0) - m
void ndp_fisubL_m32(const uint32_t *ptr)      { ndp_WrLm32(FISUBL_m32,  ptr); }  // FSUB  'L'
void ndp_fsubS_m32fp(const union m32fp *ptr)  { ndp_WrSm32(FSUBS_m32fp, ptr); }  // FSUB  'S'
void ndp_fsubD_m64fp(const union m64fp *ptr)  { ndp_WrDm64(FSUBD_m64fp, ptr); }  // FSUB  'D'

void ndp_fisubrW_m16(const uint16_t *ptr)     { ndp_WrWm16(FISUBRW_m16,  ptr); } // FSUBR 'W'  ST(0) = m - ST(0)
void ndp_fisubrL_m32(const uint32_t *ptr)     { ndp_WrLm32(FISUBRL_m32,  ptr); } // FSUBR 'L'
void ndp_fsubrS_m32fp(const union m32fp *ptr) { ndp_WrSm32(FSUBRS_m32fp, ptr); } // FSUBR 'S'
void ndp_fsubrD_m64fp(const union m64fp *ptr) { ndp_WrDm64(FSUBRD_m64fp, ptr); } // FSUBR 'D'

void ndp_fimulW_m16(const uint16_t *ptr)      { ndp_WrWm16(FIMULW_m16,  ptr); }  // FMUL  'W'
void ndp_fimulL_m32(const uint32_t *ptr)      { ndp_WrLm32(FIMULL_m32,  ptr); }  // FMUL  'L'
void ndp_fmulS_m32fp(const union m32fp *ptr)  { ndp_WrSm32(FMULS_m32fp, ptr); }  // FMUL  'S'
void ndp_fmulD_m64fp(const union m64fp *ptr)  { ndp_WrDm64(FMULD_m64fp, ptr); }  // FMUL  'D'

void ndp_fidivW_m16(const uint16_t *ptr)      { ndp_WrWm16(FIDIVW_m16,  ptr); }  // FDIV  'W'  ST(0) = ST(0) / m
void ndp_fidivL_m32(const uint32_t *ptr)      { ndp_WrLm32(FIDIVL_m32,  ptr); }  // FDIV  'L'
void ndp_fdivS_m32fp(const union m32fp *ptr)  { ndp_WrSm32(FDIVS_m32fp, ptr); }  // FDIV  'S'
void ndp_fdivD_m64fp(const union m64fp *ptr)  { ndp_WrDm64(FDIVD_m64fp, ptr); }  // FDIV  'D'

void ndp_fidivrW_m16(const uint16_t *ptr)     { ndp_WrWm16(FIDIVRW_m16,  ptr); } // FDIVR 'W'  ST(0) = m / ST(0)
void ndp_fidivrL_m32(const uint32_t *ptr)     { ndp_WrLm32(FIDIVRL_m32,  ptr); } // FDIVR 'L'
void ndp_fdivrS_m32fp(const union m32fp *ptr) { ndp_WrSm32(FDIVRS_m32fp, ptr); } // FDIVR 'S'
void ndp_fdivrD_m64fp(const union m64fp *ptr) { ndp_WrDm64(FDIVRD_m64fp, ptr); } // FDIVR 'D'

//___________________________________________________________________________________________________

void ndp_WRm16(uint16_t opc, uint16_t d) {              // call by value, int /uint is raw for struct with ndp devinition
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, d);
  ndpPeReqAckOff();
}

void ndp_WRm32(uint16_t opc, uint32_t d)
{
  const uint16_t *w = (const uint16_t *)&d;
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, w[0]);
  ndpPeReqAckOff();
  ndpWRbus(NDP_DATA_MASK, w[1]);
}

void ndp_WRm64(uint16_t opc, uint64_t d)
{
  const uint16_t *w = (const uint16_t *)&d;
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  ndpWRbus(NDP_DATA_MASK, w[0]);
  ndpPeReqAckOff();
  ndpWRbus(NDP_DATA_MASK, w[1]);
  ndpWRbus(NDP_DATA_MASK, w[2]);
  ndpWRbus(NDP_DATA_MASK, w[3]);
}


uint16_t ndp_RDm16(uint16_t opc) {                      // return value is int / uint  raw for sturct with ndp definitio
  uint16_t d;
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  d = ndpRDbus(NDP_DATA_MASK);
  ndpPeReqAckOff();
  return d;
}

uint32_t ndp_RDm32(uint16_t opc) {
  uint32_t d;
  uint16_t *w = (uint16_t *)&d;
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  w[0] = ndpRDbus(NDP_DATA_MASK);
  ndpPeReqAckOff();
  w[1] = ndpRDbus(NDP_DATA_MASK);
  return d;
}

uint64_t ndp_RDm64(uint16_t opc)
{
  uint64_t d;
  uint16_t *w = (uint16_t *)&d;
  ndpFWAIT();
  ndpWRcode(opc);
  ndpInitWaitPeReq();

  w[0] = ndpRDbus(NDP_DATA_MASK);
  ndpPeReqAckOff();
  w[1] = ndpRDbus(NDP_DATA_MASK);
  w[2] = ndpRDbus(NDP_DATA_MASK);
  w[3] = ndpRDbus(NDP_DATA_MASK);
  return d;
}


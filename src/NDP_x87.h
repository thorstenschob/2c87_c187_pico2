/*
 * Copyright (c) 2025-2026 Thorsten Schob
 * SPDX-License-Identifier: MIT
 */

#ifndef _NDP_x87_H
#define _NDP_x87_H

#include <stdint.h>             // Required for uint8_t, uint16_t, etc.

//===================================================================================================
// The definition is taken from the 80C187 PDF document, inserted into a spreadsheet and then reformatted.
// The structure and order are derived from the original data sheet.
//
// Naming convention: function and macro names mirror the real Intel mnemonic (including the 'FI'/'fi'
// integer prefix) 1:1 with the opcode #define above them - this is a deliberate convention, not an
// oversight. The _m16/_m32/_m64/... suffix (a C-side clarity aid introduced during the assembler-to-C
// conversion) only indicates the memory operand's width/shape; it does not replace the mnemonic prefix.
//
//===================================================================================================
                                    // FLD = Load
#define FLDS_m32fp      0xD900      // 
#define FILDL_m32       0xDB00      // 
#define FLDD_m64fp      0xDD00      // 
#define FILDW_m16       0xDF00      // 
#define FILDLL_m64      0xDF28      // 
#define FLDX_m80        0xDB28      // 
#define FBLD_m80BCD     0xDF20      // is not 'packed'
#define FLDn            0xD9C0      // 
                                    // FST = Store
#define FSTS_m32fp      0xD910      // 
#define FISTL_m32       0xDB10      // 
#define FSTD_m64fp      0xDD10      // 
#define FISTW_m16       0xDF10      // 
#define FSTn            0xDDD0      // 
                                    // FSTP = Store and Pop
#define FSTPS_m32fp     0xD918      // 
#define FISTPL_m32      0xDB18      // 
#define FSTPD_m64fp     0xDD18      // 
#define FISTPW_m16      0xDF18      // 
#define FISTPLL_m64     0xDF38      // & pop
#define FSTPX_m80       0xDB38      // & pop
#define FBSTP_m80BCD    0xDF30      // & pop, is not 'packed'
#define FSTPn           0xDDD8      // 
                                    // FXCH = Exchange
#define FXCHn           0xD9C8      // 
                                    // COMPARISON
                                    // FCOM = Compare
#define FCOMS_m32fp     0xD810      // 
#define FICOML_m32      0xDA10      // 
#define FCOMD_m64fp     0xDC10      // 
#define FICOMW_m16      0xDE10      // 
#define FCOMn           0xD8D0      // 
                                    // FCOMP = Compare and pop
#define FCOMPS_m32fp    0xD818      // 
#define FICOMPL_m32     0xDA18      // 
#define FCOMPD_m64fp    0xDC18      // 
#define FICOMPW_m16     0xDE18      // 
#define FCOMPn          0xD8D8      // 
                                    // FCOMPP = Compare and pop twice
#define FCOMPP          0xDED9      // 
#define FTST            0xD9E4      // 
#define FUCOMn          0xDDE0      // do not exist on the 8087 or original 80287
                                    // FUCOMP = Unordered compare
#define FUCOMPn         0xDDE8      // do not exist on the 8087 or original 80287
                                    // FUCOMPP = Unordered compareand pop twice
#define FUCOMPP         0xDAE9      // do not exist on the 8087 or original 80287
#define FXAM            0xD9E5      // 
                                    // CONSTANTS
#define FLDZ            0xD9EE      //    Load a0.0  
#define FLD1            0xD9E8      //    Load a1.0 
#define FLDPI           0xD9EB      //    Load pi 
#define FLDL2T          0xD9E9      //    Load log2(10) 
#define FLDL2E          0xD9EA      //    Load log2(e)  
#define FLDLG2          0xD9EC      //    Load log10(2)  
#define FLDLN2          0xD9ED      //    Load loge(2) 
                                    // ARITHMETIC
                                    // FADD = Add
#define FADDS_m32fp     0xD800      // 
#define FIADDL_m32      0xDA00      // 
#define FADDD_m64fp     0xDC00      // 
#define FIADDW_m16      0xDE00      // 
#define FADDn           0xD8C0      // 
#define FADDdn          0xDCC0      // 
#define FADDPdn         0xDEC0      // 
                                    // FSUB = Subtract
#define FSUBS_m32fp     0xD820      // 
#define FISUBL_m32      0xDA20      // 
#define FSUBD_m64fp     0xDC20      // 
#define FISUBW_m16      0xDE20      // 
#define FSUBn           0xD8E0      // ST(0) = ST(0) - ST(i)
#define FSUBdn          0xDCE0      // ST(i) = ST(0) - ST(i)
#define FSUBPdn         0xDEE0      // ST(i) = ST(0) - ST(i),  & POP
                                    // FSUBR = Subtract Reverse
#define FSUBRS_m32fp    0xD828      // 
#define FISUBRL_m32     0xDA28      // 
#define FSUBRD_m64fp    0xDC28      // 
#define FISUBRW_m16     0xDE28      // 
#define FSUBRn          0xD8E8      // ST(0) = ST(i) - ST(0)
#define FSUBRdn         0xDCE8      // ST(i) = ST(i) - ST(0)
#define FSUBRPdn        0xDEE8      // ST(i) = ST(i) - ST(0),  & POP
                                    // FMUL = Multiply
#define FMULS_m32fp     0xD808      // 
#define FIMULL_m32      0xDA08      // 
#define FMULD_m64fp     0xDC08      // 
#define FIMULW_m16      0xDE08      // 
#define FMULn           0xD8C8      // 
#define FMULdn          0xDCC8      // 
#define FMULPdn         0xDEC8      // 
                                    // FDIV = Divide
#define FDIVS_m32fp     0xD830      // 
#define FIDIVL_m32      0xDA30      // 
#define FDIVD_m64fp     0xDC30      // 
#define FIDIVW_m16      0xDE30      // 
#define FDIVn           0xD8F0      // ST(0) = ST(0) / ST(i)
#define FDIVdn          0xDCF0      // ST(i) = ST(0) / ST(i)
#define FDIVPdn         0xDEF0      // ST(i) = ST(0) / ST(i), & POP
                                    // FDIVR = Divide Reverse
#define FDIVRS_m32fp    0xD838      // 
#define FIDIVRL_m32     0xDA38      // 
#define FDIVRD_m64fp    0xDC38      // 
#define FIDIVRW_m16     0xDE38      // 
#define FDIVRn          0xD8F8      // ST(0) = ST(i) / ST(0
#define FDIVRdn         0xDCF8      // ST(i) = ST(i) / ST(0)
#define FDIVRPdn        0xDEF8      // ST(i) = ST(i) / ST(0), & POP
                                    // 
#define FSQRT           0xD9FA      // 
#define FSCALE          0xD9FD      // 
                                    // FPREM = Partial remainder of
#define FPREM           0xD9F8      // 
                                    // FPREM1 = Partial remainder
#define FPREM1_DoNotUse 0xD9F5      // not available in 8087.  ? is notoriously unstable or missing depending on the chip stepping of Intels 287. ?
                                    // FRNDINT = Round ST(0) [& x287] to integer
#define FRNDINT         0xD9FC      // at 80287  fully supported and available
                                    // FXTRACT = Extract components of ST(0)
#define FXTRACT         0xD9F4      // 
#define FABS            0xD9E1      // 
#define FCHS            0xD9E0      // 
                                    // TRANSCENDENTAL
#define FCOS            0xD9FF      // do not exist on the 8087 or 80287 
#define FPTAN           0xD9F2      // 
#define FPATAN          0xD9F3      // 
#define FSIN            0xD9FE      // do not exist on the 8087 or 80287
#define FSINCOS         0xD9FB      // do not exist on the 8087 or 80287
#define F2XM1           0xD9F0      // 
#define FYL2X           0xD9F1      // 
#define FYL2XP1         0xD9F9      // 
                                    // PROCESSOR CONTROL
#define FINIT           0xDBE3      // 
#define FSTSW_AX        0xDFE0      // Word and register AX is _m16
#define FLDCW_m16       0xD928      // 
#define FSTCW_m16       0xD938      // 
#define FSTSW_m16       0xDD38      // Supported for backwards compatibility; has the same value as FSTSW AX (0xDFE0).
#define FCLEX           0xDBE2      // 
#define FSTENV          0xD930      // 
#define FLDENV          0xD920      // 
#define FSAVE           0xDD30      // 
#define FRSTOR          0xDD20      // 
#define FINCSTP         0xD9F7      // 
#define FDECSTP         0xD9F6      // 
#define FFREEn          0xDDC0      // 
#define FNOP            0xD9D0      //

//===================================================================================================

#define NDP_X87_C0      0x0100      // Bit 8  
#define NDP_X87_C2      0x0400      // Bit 10 
#define NDP_X87_C3      0x4000      // Bit 14 

#define NDP_X87_COMP_MASK (NDP_X87_C0 | NDP_X87_C2 | NDP_X87_C3)  // Combined flag mask for clean checking 

static inline bool ndp_is_gt(uint16_t sw) {                   // Evaluates if ST(0) > Operand (Greater Than)
    return (sw & NDP_X87_COMP_MASK) == 0x0000;                // True only if C3, C2, and C0 are all 0.
}

//===================================================================================================
// NDP_* convenience macros - optional wrapper layer around the ndp_*() functions/opcodes above.
// Not required by this codebase (every NDP_xxx expands 1:1 to a plain function call), kept as an
// alternative offer for developers used to older compilers in  8-16 bit systems, where a directly
// visible opcode-macro style was common. Currently a DRAFT: coverage is incomplete (only a subset
// of the opcodes above have a NDP_* counterpart) - extend on demand, no need to complete all of them.
//===================================================================================================
                                                              // FLD = Load
#define NDP_FILDW_m16(d)  ndp_WRm16(FILDW_m16, (d))           //
#define NDP_FLD(n)        ndp_WrCmd(FLDn   | ((n) & 0x07))    //
                                                              // FST = Store
#define NDP_FSTS_m32fp    ndp_RDm32(FSTS_m32fp)               //
#define NDP_FST(n)        ndp_WrCmd(FSTn   | ((n) & 0x07))    //    
                                                              // COMPARISON
#define NDP_FCOMP(n)      ndp_WrCmd(FCOMPn | ((n) & 0x07))    // FCOM = Compare
                                                              // CONSTANTS
#define NDP_FLDZ          ndp_WrCmd(FLDZ)                     //    Load a0.0 
#define NDP_FLD1          ndp_WrCmd(FLD1)                     //    Load a1.0
#define NDP_FLDPI         ndp_WrCmd(FLDPI)                    //    Load pi
#define NDP_FLDL2T        ndp_WrCmd(FLDL2T)                   //    Load log2(10)
#define NDP_FLDL2E        ndp_WrCmd(FLDL2E)                   //    Load log2(e) 
#define NDP_FLDLG2        ndp_WrCmd(FLDLG2)                   //    Load log10(2) 
#define NDP_FLDLN2        ndp_WrCmd(FLDLN2)                   //    Load loge(2)
                                                              // ARITHMETIC
#define NDP_FADD(n)       ndp_WrCmd(FADDn  | ((n) & 0x07))    // FADD = Add
#define NDP_FSUB(n)       ndp_WrCmd(FSUBn  | ((n) & 0x07))    // FSUB = Subtract
#define NDP_FMULS_m32fp(d)ndp_WRm32(FMULS_m32fp, (d))         // FMUL = Multiply
#define NDP_FMUL(n)       ndp_WrCmd(FMULn  | ((n) & 0x07))    //
//      NDP_FMULwW(n)     ndp_WRcode(FMULn | ((n) & 0x07))    // ... without wait for example detail from c't 3/87
#define NDP_FMULd(n)      ndp_WrCmd(FMULdn | ((n) & 0x07))    // 
//      NDP_FMULwWd(n)    ndp_WRcode(FMULdn| ((n) & 0x07))    // ... without wait for example detail from c't 3/87
#define NDP_FDIV(n)       ndp_WrCmd(FDIVn  | ((n) & 0x07))    // FDIV = Divide
                                                              // PROCESSOR CONTROL
#define NDP_FINIT         ndp_WrCmd(FINIT)                    //
#define NDP_FSTSW_AX      ndp_WRcodeRDcwsw(FSTSW_AX)          // Word and register AX is _m16
#define NDP_FLDCW(d)      ndp_WRm16(FLDCW_m16, (d))           // 
#define NDP_FSTCW         ndp_WRcodeRDcwsw(FSTCW_m16)         //
#define NDP_FSTSW         ndp_WRcodeRDcwsw(FSTSW_m16)         // Supported for backwards compatibility; has the same value as FSTSW AX (0xDFE0).
#define NDP_FINCSTP       ndp_WrCmd(FINCSTP)                  //
#define NDP_FDECSTP       ndp_WrCmd(FDECSTP)                  //
#define NDP_FFREE(n)      ndp_WrCmd(FFREEn | ((n) & 0x07))    //
#define NDP_FNOP          ndp_WrCmd(FNOP)                     //
                                                              //
//===================================================================================================


extern const uint32_t L_10p[10];
extern const uint64_t LL_10p[20];


// B_m8     B  Byte Integer 	         int8_t  ('m8int‘)
// W_m16    W  Word Integer		         int16_t ( m16int)
// L_m32    L  Long Word Integer	     int32_t,  m32int
// LL_m64                              int64_t,  m64
// S_m32fp  S  Single Precision Real 	m32fp	Single-precision floating-point format
// D_m64fp  D  Double Precision Real 	m64fp	Double-precision floating-point format
// X_m80    X  Extended Precision Real	m96fp Extended Precision Binary Real Format (Intel m80fp)
//            with 'High Order Long Word', 'Mid Order Long Word', 'Low Order Long Word'
// P_m80BCD P  Packed Decimal Real   	m96BCD  …see DS x87 (p. 3-9. Packed Decimal Real Data Format)


union m32fp { 
    uint32_t  L_int;            // Long Word Integer
    struct { // LSB                  MSB
      uint8_t byte0, byte1, byte2, byte3;
      };
    uint8_t   b[4U];
    struct { //   LS     MS
      uint16_t word0, word1;
      };
    uint16_t  w[2U];

    float     S_real;             // Single-precision floating-point format
};

union m64fp { 
    uint64_t  LL_int;             // Long Word Integer
    struct { // LSB                                              MSB
      uint8_t byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7;
      };
    uint8_t   b[8U];
    struct { //   LS                   MS
      uint16_t word0, word1, word2, word3;
      };
    uint16_t  w[4U];
                               
    double    D_real;           // Double-precision floating-point format,
};

union m80 {                     // not m96fp
    struct { // LSB                                                            MSB
      uint8_t byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9;
      };
    uint8_t   b[10U];
    struct { //   LS                          MS
      uint16_t word0, word1, word2, word3, word4;
      };
    uint16_t  w[5U];
                                // Compatibility with other systems is not guaranteed.
    // long double    X_real;   // extended-precision floating-point format. ?
};


union m80pBCD {   // motorola pBCD  Packed BCD 'mantisse integer', MS first
    struct { // MSB                                                            LSB
      uint8_t byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9;
      };
    uint8_t   b[10U];
    struct { //   MS                          LS
      uint16_t word0, word1, word2, word3, word4;
      };
    uint16_t  w[5U];
};

// P  Packed Decimal Real   	m96BCD  …see DS MC68881 (p. 3-9.? Packed Decimal Real Data Format)
union m96pBCD {                
    struct { // MSB                                                                            LSB
      uint8_t byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9, byte10, byte11;
      };
    uint8_t   b[12U];
    struct { //   MS                                 LS
      uint16_t word0, word1, word2, word3, word4, word5;
      };
    uint16_t  w[6U];
};


extern uint16_t fpcpPrimitiveResponse;  // storage temporary Primitive Response
extern uint16_t fpcpPrimitiveResponse2; // storage temporary Primitive Response 2 for debug


                                                        // needed for the macro example
void     ndp_WrCmd(uint16_t cmd);                       // for the macro
// void  ndp_WrCmdReg(uint16_t cmd, uint8_t reg)        // obsoled here, is made by the macro
uint16_t ndp_WRcodeRDcwsw(uint16_t opc);                // for the macro

void ndp_init(void);

void     ndp(uint16_t opc);                             // == ndp_WrCmd(opc);
void     ndp_(uint16_t opc, uint8_t reg);               // == ndp_WrCmdReg(opc, reg);
uint16_t ndp_get(uint16_t opc);                         // == ndp_WRcodeRDcwsw(opc);

                                                        // Write memory transfer (pointer interface)
void ndp_WrWm16 (uint16_t opc, const uint16_t *ptr);
void ndp_WrLm32(uint16_t opc, const uint32_t *ptr);
void ndp_WrSm32(uint16_t opc, const union m32fp *ptr);
void ndp_WrLLm64(uint16_t opc, const uint64_t *ptr);
void ndp_WrDm64(uint16_t opc, const union m64fp *ptr);
void ndp_WrXm80(uint16_t opc, const union m80 *ptr);
void ndp_WrPm80(uint16_t opc, const union m80pBCD *ptr);
                                                        // Read memory transfer (pointer interface)
void ndp_RdWm16 (uint16_t opc, uint16_t *ptr);
void ndp_RdLm32(uint16_t opc, uint32_t *ptr);
void ndp_RdSm32(uint16_t opc, union m32fp *ptr);
void ndp_RdLLm64(uint16_t opc, uint64_t *ptr);
void ndp_RdDm64(uint16_t opc, union m64fp *ptr);
void ndp_RdXm80(uint16_t opc, union m80 *ptr);
void ndp_RdPm80(uint16_t opc, union m80pBCD *ptr);

                                                        // x87  instruction wrappers Load
void ndp_fildW_m16(const uint16_t *ptr);                // 'W'  Word Integer in memory to FP register
void ndp_fildL_m32(const uint32_t *ptr);                // 'L'  Long Word Integer in memory to FP register 
void ndp_fldS_m32(const union m32fp *ptr);              // 'S'  Single Precision Real in memory to FP register 
void ndp_fildLL_m64(const uint64_t *ptr);               // 'LL' _m64, int64_t in memory to FP register 
void ndp_fldD_m64fp(const union m64fp *ptr);            // 'D'  Double Precision Real in memory to FP register 
void ndp_fldX_m80(const union m80 *ptr);                // 'X'  Extended Precision Real in memory to FP register  
void ndp_fld_m80pBCD(const union m80pBCD *ptr);         //  Motorola Packed BCD in memory to FP register

                                                        // x87  instruction wrappers Store
void ndp_fistW_m16(uint16_t *ptr);                      // 'W'  Word Integer FP register value to memory operation
void ndp_fistpW_m16(uint16_t *ptr);                     // 'W'  & pop
void ndp_fistL_m32(uint32_t *ptr);                      // 'L'  Long Word Integer FP register value to memory operation
void ndp_fistpL_m32(uint32_t *ptr);                     // 'L'  & pop
void ndp_fstS_m32fp(union m32fp *ptr);                  // 'S'  Single Precision Real FP register value to memory operation
void ndp_fstpS_m32fp(union m32fp *ptr);                 // 'S'  & pop
void ndp_fistpLL_m64(uint64_t *ptr);                    // 'LL' _m64, int64_t  FP register value to memory operation
void ndp_fstD__m64fp(union m64fp *ptr);                 // 'D'  Double Precision Real FP register value to memory operation
void ndp_fstpD__m64fp(union m64fp *ptr);                // 'D'  & pop
void ndp_fstPX_m80fp(union m80 *ptr);                   // 'X'  Extended Precision Real FP register value to memory operation
void ndp_fstp_m80pBCD(union m80pBCD *ptr);              //  Motorola Packed BCD - FP register value to memory operation

                                                        // x87  instruction wrappers Comparison (memory operand vs. ST(0))
void ndp_ficomW_m16(const uint16_t *ptr);               // FCOM  'W'  compare, no pop
void ndp_ficomL_m32(const uint32_t *ptr);               // FCOM  'L'
void ndp_fcomS_m32fp(const union m32fp *ptr);           // FCOM  'S'
void ndp_fcomD_m64fp(const union m64fp *ptr);           // FCOM  'D'

void ndp_ficompW_m16(const uint16_t *ptr);              // FCOMP 'W'  compare & pop
void ndp_ficompL_m32(const uint32_t *ptr);              // FCOMP 'L'
void ndp_fcompS_m32fp(const union m32fp *ptr);          // FCOMP 'S'
void ndp_fcompD_m64fp(const union m64fp *ptr);          // FCOMP 'D'

                                                        // x87  instruction wrappers Arithmetic (memory operand -> ST(0))
void ndp_fiaddW_m16(const uint16_t *ptr);               // FADD  'W'  Word Integer
void ndp_fiaddL_m32(const uint32_t *ptr);               // FADD  'L'  Long Word Integer
void ndp_faddS_m32fp(const union m32fp *ptr);           // FADD  'S'  Single Precision Real
void ndp_faddD_m64fp(const union m64fp *ptr);           // FADD  'D'  Double Precision Real

void ndp_fisubW_m16(const uint16_t *ptr);               // FSUB  'W'  ST(0) = ST(0) - m
void ndp_fisubL_m32(const uint32_t *ptr);               // FSUB  'L'
void ndp_fsubS_m32fp(const union m32fp *ptr);           // FSUB  'S'
void ndp_fsubD_m64fp(const union m64fp *ptr);           // FSUB  'D'

void ndp_fisubrW_m16(const uint16_t *ptr);              // FSUBR 'W'  ST(0) = m - ST(0)
void ndp_fisubrL_m32(const uint32_t *ptr);              // FSUBR 'L'
void ndp_fsubrS_m32fp(const union m32fp *ptr);          // FSUBR 'S'
void ndp_fsubrD_m64fp(const union m64fp *ptr);          // FSUBR 'D'

void ndp_fimulW_m16(const uint16_t *ptr);               // FMUL  'W'
void ndp_fimulL_m32(const uint32_t *ptr);               // FMUL  'L'
void ndp_fmulS_m32fp(const union m32fp *ptr);           // FMUL  'S'
void ndp_fmulD_m64fp(const union m64fp *ptr);           // FMUL  'D'

void ndp_fidivW_m16(const uint16_t *ptr);               // FDIV  'W'  ST(0) = ST(0) / m
void ndp_fidivL_m32(const uint32_t *ptr);               // FDIV  'L'
void ndp_fdivS_m32fp(const union m32fp *ptr);           // FDIV  'S'
void ndp_fdivD_m64fp(const union m64fp *ptr);           // FDIV  'D'

void ndp_fidivrW_m16(const uint16_t *ptr);              // FDIVR 'W'  ST(0) = m / ST(0)
void ndp_fidivrL_m32(const uint32_t *ptr);              // FDIVR 'L'
void ndp_fdivrS_m32fp(const union m32fp *ptr);          // FDIVR 'S'
void ndp_fdivrD_m64fp(const union m64fp *ptr);          // FDIVR 'D'


void     ndp_WRm16(uint16_t opc, uint16_t d);           // Write memory transfer (by value)
void     ndp_WRm32(uint16_t opc, uint32_t d);
void     ndp_WRm64(uint16_t opc, uint64_t d);
uint16_t ndp_RDm16(uint16_t opc);                       // Read memory transfer (by value)
uint32_t ndp_RDm32(uint16_t opc);
uint64_t ndp_RDm64(uint16_t opc);

#endif

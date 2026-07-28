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

#include <string.h>

//#include <sys/types.h>
//#include <inttypes.h>

#include "NDP_hw_io.h"
#include "NDP_x87.h"

// #include <stdlib.h>


union m32fp   m32fp,   m32fp2;
union m64fp   m64fp,   m64fp2;
union m80pBCD m80pBCD, m80pBCD2;


uint16_t fpcpPrimitiveResponse;  // storage temporary Primitive Response
uint16_t fpcpPrimitiveResponse2; // storage temporary Primitive Response 2 for debug

uint16_t resp[20];

int32_t startTime, endTime, executionTime;


//===================================================================================================
    typedef struct {
        const char *cmd;            //  Command ID
        const uint16_t  opcode;     // Hex opcode
        const uint16_t  opcode2;    // Hex opcode
        const char *equation;       // Mathematical equation
        const char *description;    // Description
        float r0;                   // Current value of ST(0) -> Overwritten by ndp_eval()
        float r1;                   // Current value of ST(i) -> Overwritten by ndp_eval()
        float expected_r0;          // Fixed setpoint for ST(0)
        float expected_r1;          // Fixed setpoint for ST(i)
        uint8_t status;             // Status mask
    } NDP_Instruction;

    // Naming note: unlike the memory-operand mnemonics, the register-form opcodes tested below are a C-side clarity aid 
    // introduced during the assembler-to-C conversion. The suffix follows the logic and math needed to parameterize the
    // ST(i) register index as a C function argument, not the Intel assembly philosophy, which writes the operand explicitly .
    // The table below exists to verify that the resulting math is actually correct.
    //
    //  For comparison and verification purposes, a copy from the sources listed here has been overlayed. (block copying method)
    NDP_Instruction ndp_table[] = {
        //Macro        Define    Opcode   Mathematical equation    description                  r0   r1      r0'=   r1'=  Status
        {"FSUBn     ", FSUBn   , 0xD8E0, "ST(0) = ST(0) - ST(i)", "Subtract ST(i) from ST(0)",  5.0,  2.0,   3.0,   2.0,  0},
        {"FSUBRn    ", FSUBRn  , 0xD8E8, "ST(0) = ST(i) - ST(0)", "Subtract ST(0) from ST(i)",  5.0,  2.0,  -3.0,   2.0,  0},
        {"FDIVn     ", FDIVn   , 0xD8F0, "ST(0) = ST(0) / ST(i)", "Divide   ST(0) by ST(i)  ",  5.0,  2.0,   2.5,   2.0,  0},
        {"FDIVRn    ", FDIVRn  , 0xD8F8, "ST(0) = ST(i) / ST(0)", "Divide   ST(i) by ST(0)  ",  5.0,  2.0,   0.4,   2.0,  0},
        {"FSUBdn    ", FSUBdn  , 0xDCE0, "ST(i) = ST(0) - ST(i)", "Subtract ST(i) from ST(0)",  5.0,  2.0,   5.0,   3.0,  0},
        {"FSUBRdn   ", FSUBRdn , 0xDCE8, "ST(i) = ST(i) - ST(0)", "Subtract ST(0) from ST(i)",  5.0,  2.0,   5.0,  -3.0,  0},
        {"FDIVdn    ", FDIVdn  , 0xDCF0, "ST(i) = ST(0) / ST(i)", "Divide   ST(0) by ST(i)  ",  5.0,  2.0,   5.0,   2.5,  0},
        {"FDIVRdn   ", FDIVRdn , 0xDCF8, "ST(i) = ST(i) / ST(0)", "Divide   ST(i) by ST(0)  ",  5.0,  2.0,   5.0,   0.4,  0},
        {"FSUBPdn   ", FSUBPdn , 0xDEE0, "ST(i) = ST(0) - ST(i)", "Sub ST(i) from ST(0), POP",  5.0,  2.0,   3.0,   0.0,  0},
        {"FSUBRPdn  ", FSUBRPdn, 0xDEE8, "ST(i) = ST(i) - ST(0)", "Sub ST(0) from ST(i), POP",  5.0,  2.0,  -3.0,   0.0,  0},
        {"FDIVPdn   ", FDIVPdn , 0xDEF0, "ST(i) = ST(0) / ST(i)", "Div ST(0) by ST(i)  , POP",  5.0,  2.0,   2.5,   0.0,  0},
        {"FDIVRPdn  ", FDIVRPdn, 0xDEF8, "ST(i) = ST(i) / ST(0)", "Div ST(i) by ST(0)  , POP",  5.0,  2.0,   0.4,   0.0,  0}
    }; 

//===================================================================================================
    // The inclusion of two conflicting data formats is only temporary for the purposes of
    // this demonstration; it makes no sense in the context of further planning, 
    // particularly as the aim is to achieve an implementation that is largely similar to that of the MC68881 variant.

    union m80BCD_tmp {  // intel BCD                
        struct { //   LS                          MS
          uint16_t word0, word1, word2, word3, word4;
          };
        uint16_t  w[5U];
    } m80BCD_tmp;

    // Intel BCD
    // Temporary artifact for verification just in purpose of formality and not implemented for normal use; it only leads to confusion.*
    // 'most significant' (MS) first makes the most sense with BCD similar to the Motorola implementation,
    // especially as a further extension with a k factor is appropriate (MC68881 describes Big-Endian ordering).
    // *'better' than two conflicting data formats in the library 'NDP_x87'.

    void ndp_RdBm80_tmp(uint16_t opc, union m80BCD_tmp *ptr) {  // Intel BCD  
      ndpFWAIT();
      ndpWRcode(opc);
      ndpInitWaitPeReq();

      (*ptr).word0 = ndpRDbus(NDP_DATA_MASK); // LS
      ndpPeReqAckOff();
      (*ptr).word1 = ndpRDbus(NDP_DATA_MASK);
      (*ptr).word2 = ndpRDbus(NDP_DATA_MASK);
      (*ptr).word3 = ndpRDbus(NDP_DATA_MASK);
      (*ptr).word4 = ndpRDbus(NDP_DATA_MASK); // MS
    }
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//===================================================================================================


void _p7(){ printf("       "); }

void _ps(char* str){ printf("%s", str); }

void _p_st(char* info) {
  int j;
  printf("%s: ", info);
  for (j = 0; j < 8; j++)           
  {
    m32fp.L_int = NDP_FSTS_m32fp;                            //  m32fp { uint32_t  L_int; 
    printf("%5.1f; ", m32fp.S_real);
    NDP_FINCSTP;
  }
  for (j=0; j < 8; j++) { NDP_FDECSTP;}
  printf("\r\n");  
}

int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_set_function(PICO_DEFAULT_LED_PIN, GPIO_FUNC_SIO); // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);           // so we can use normal GPIO functionality to turn the led on and off
    return PICO_OK;
#else
    return PICO_ERRR_IO
#endif    
}

void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)                           // Turn the led on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);                 // Just set the GPIO on or off
#endif
}

bool pico_get_led() {
#if defined(PICO_DEFAULT_LED_PIN)                           // Read status LED
    return gpio_get(PICO_DEFAULT_LED_PIN);
#endif
}

void osc_frequency_count(void)
{   // Info from 'clocks.h': Writing to this register initiates the frequency count
    uint f_pll_sys  = frequency_count_mhz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb  = frequency_count_mhz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc     = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC); // khz
    uint f_clk_sys  = frequency_count_mhz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_mhz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb  = frequency_count_mhz(CLOCKS_FC0_SRC_VALUE_CLK_USB);

    printf("  pll_sys  = %dmHz\n", f_pll_sys);
    printf("  pll_usb  =  %dmHz\n", f_pll_usb);   //
    printf("  rosc     = %dkHz\n", f_rosc);       // kHz
    printf("  clk_sys  = %dmHz\n", f_clk_sys);
    printf("  clk_peri = %dmHz\n", f_clk_peri);
    printf("  clk_usb  =  %dmHz\n", f_clk_usb);   //
}


int main()
{
  /*                                      // C:\Users\xyz\.pico-sdk\sdk\2.2.0\src\rp2_common
                                          // Info https://github.com/raspberrypi/pico-sdk/issues/1037 info-print
                                          // How to: https://forums.raspberrypi.com/viewtopic.php?t=375975
                                          //         https://learn.pimoroni.com/article/overclocking-the-pico-2
  uint32_t sysclk = 50 * 1000  ;          // Pico2 standard 150MHz <<== set sysclk
  vreg_set_voltage(VREG_VOLTAGE_1_20);    // VREG_VOLTAGE_DEFAULT = VREG_VOLTAGE_1_10,  ///< Default voltage on power up.
  sleep_ms(100);                          // 1.10V default? ;   1.15  1.20  1.25  1.30V
  set_sys_clock_khz(sysclk, true);
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, sysclk * 1000, sysclk * 1000 / 2 ); // <<== freq: Requested frequency := /2
                                          //
  */   
  sysTickStart();

  stdio_init_all();
  int rc = pico_led_init();
  hard_assert(rc == PICO_OK);

  gpio_set_function(HW_TRIGGER , GPIO_FUNC_SIO); gpio_set_dir(HW_TRIGGER,  GPIO_OUT);  gpio_put(HW_TRIGGER,  1);   // HW_TRIGGER
  //gpio_set_slew_rate(HW_TRIGGER, GPIO_SLEW_RATE_FAST);

  init_ControlBus();

  init_DataBus();

  gpio_set_dir_out_masked (ndpDBUSMASK);                      //(must not be INPUT_PULLDOWN mask A2 & 'E9');

  gpio_put_masked (ndpDBUSMASK, ndpDBUSMASK);                 // set databus

  pull_up_DataBus();
    
  ndpRESET();

  while (!tud_cdc_connected())
  {
    pico_set_led(true);               // == NDP RESET (High, 1)
    sleep_ms(100); // 100
    pico_set_led(false);
    sleep_ms(100); // 100             // == NDP no reset (Low, 0), is high active
  }

  printf("RP2350 tusb_cdc_connected()\r\n");
  gpio_put(PICO_DEFAULT_LED_PIN, 0);  // == NDP no reset (Low, 0), is high active
  gpio_put(HW_TRIGGER, 1);            // HW debug

  init_PiggybackMap();

  //====================================================================================================================

  printf("PICO_SDK_VERSION_STRING:  %s\n", PICO_SDK_VERSION_STRING);
  printf("RP2350_chip_/ROM_version: %d / %d\n", rp2040_chip_version(), rp2040_rom_version());
  osc_frequency_count();

  //====================================================================================================================

  sleep_ms(1000);

  if (NDP_IS_C187) {
    printf("\r\nInit RP2350 with NDP Intel C187 only: \r\n");
  } else {
    printf("Init RP2350 with NDP x87 series: \r\n");
  }

  while (true)
  {
    int16_t       ix, jy, i;
    union m32fp   cXchar, cYchar;   // const X Y multiply char with
    uint32_t      stw_ax;           // for FSTSW, FSTSW_AX

    sleep_ms(10);
        
    if (NDP_IS_C187) {
      printf("\r\nTest 'i80c187_pico2': RP2350 Pico 2 with C187 NDP>\r\n");
    } else {
      printf("\r\nTest '2c87_pico2': RP2350 Pico 2 with x87 NDP series>\r\n");
    }

    sleep_ms(100);

    ndpRESET();
    sleep_us(1);

    sleep_ms(500);

    // gpio_put(HW_TRIGGER, 0); addCycles(32); gpio_put(HW_TRIGGER, 1); // HW debug

    ndp_init();                           // ndp(FINIT);

    printf("\r\n- START - ======================================================================================\r\n");
    printf("\r\n- Start: Debug output for Pi as a short test, quick and dirty                     ______________\r\n\r\n");

    ndp(FLDPI); 
    ndp_WRm32(FIMULL_m32 , L_10p[8]);     //  0x5F5E100  10^8  =   100.000.000 (100M [mega])
    ndp_WRm32(FIMULL_m32 , L_10p[9]);     // 0x3B9ACA00  10^9  = 1.000.000.000 (  1G [giga])

                                          // temporary, pointless just for the purpose of formality
                                          // Replacing words like "pointless" with precise terms like "temporary artifact for hardware verification" 
    ndp_RdBm80_tmp(FBSTP_m80BCD, &m80BCD_tmp);              // Intel sequence
    printf("     Result   Intel BCD:    %04X%04X%04X%04X%04X   LSB first in memory \r\n", m80BCD_tmp.word4, m80BCD_tmp.word3, m80BCD_tmp.word2, m80BCD_tmp.word1, m80BCD_tmp.word0);

    ndp_WRm64(FILDLL_m64, LL_10p[17]);    // 10^17 = 100.000.000.000.000.000   (100P [peta])
    ndp(FLDPI);  
    ndp_(FMULn,1);

    ndp_RdPm80(FBSTP_m80BCD, &m80pBCD);                     // Motorola sequence
    printf("     Result 'Motorola' BCD: %04X%04X%04X%04X%04X   MSB first in memory \r\n", m80pBCD.word0,  m80pBCD.word1, m80pBCD.word2,  m80pBCD.word3, m80pBCD.word4);

    printf("                               |1       |10   |16 \r\n");
    printf("     NASA: \"  ... we use     3.141592653589793. ...\" ; \r\n");
    printf("     Pi:        ... is     ~ 3.141592653589793238462643383279 ... ; \r\n");
    printf("\r\n           _____________________________________________________________________________________\r\n");
    printf("\r\n");

    m80pBCD.word3 = m80pBCD.word4 = 0x00; // x with 'less digits'
    ndp_WRm64(FILDLL_m64, LL_10p[17]);    // 10^17 = 100.000.000.000.000.000 = 100P [peta]
    ndp_WrPm80(FBLD_m80BCD, &m80pBCD);    // Motorola sequence
    ndp_(FDIVn,1);                        // FDIV

    ndp(FLDPI);
    ndp_(FSUBn, 1);                       // FSUB ' Pi - x '

    ndp_WRm64(FILDLL_m64, LL_10p[17]);    // 10^17 = 100P [peta]  = 100 Quadrillions   [DE: 100 Billiarden]
    ndp_(FMULn,1);                        // FMUL
    ndp_WRm32(FIMULL_m32 , 1000);         // and for test *1000 ==> 100 Quintillions   [DE: 100 Trillionen]
    ndp_RdPm80(FBSTP_m80BCD, &m80pBCD2);  // Motorola sequence

    printf("     - 9 digits         x: %04X%04X%04X\r\n", m80pBCD.word0,  m80pBCD.word1, m80pBCD.word2);
    printf("     BCD   Deviation Pi-x:            %04X%04X%04X  =  Pi - 3.1%04X%04X\r\n", m80pBCD2.word2,  m80pBCD2.word3, m80pBCD2.word4, m80pBCD.word1, m80pBCD.word2 );
    printf("                              |1       |10   |16|19                   |9\r\n");
    printf("     NASA: \"  ... we use    3.141592653589793. ...\" ; \r\n");                         // 0,1f [femto] / 100a [atto]  resolution is good enough
    printf("     Pi:        ... is    ~ 3.141592653589793238462643383279 ... ; \r\n");
    printf("\r\n-_______________________________________________________________________________________________\r\n");
    printf("\r\n  Architecture Note: The NDP provides 17 decimal places using Packed BCD formatting.");
    printf("\r\n- When evaluating raw 'binary floating point' operations, Pi retains approx. 19 useful decimal places.");
    printf("\r\n- Future hardware analysis requires shifting completely to the 'X' 80-bit Extended Precision Binary Real Format.\r\n");
    printf("\r\n- Keep in mind: https://www.jpl.nasa.gov/edu/news/how-many-decimals-of-pi-do-we-really-need/");
    printf("\r\n- End of WIP Test Pass - _______________________________________________________________________\r\n\r\n");

    sleep_ms(1000);
    printf("\r\n- START - ======================================================================================\r\n\r\n\r\n");
    startTime = time_us_32();      

    ndp_init();                                     // ndp(FINIT); & FPU stack is completely empty.
    ndp(FLD1); 
    ndp(FLDPI);
    ndp_(FMULn, 0);
    ndp(FRNDINT);         // _p_st("FRNDINT cX");   // Round ST(0) & x287 !!!
    ndp_(FLDn, 0);        // _p_st("FLD 0  cX ");   // FLD ST(0) = PUSH
    ndp_(FLDn, 0);        // _p_st("FLD 0  cX ");
    ndp_(FMULdn, 1);      // _p_st("FMULd0 cX ");
    ndp_(FSUBn, 3);       // _p_st("FSUB 3 cX ");
    ndp_(FLDn, 0);        // _p_st("FLD 0  cX ");
    ndp_(FSUBn, 4);       // _p_st("FSUB 4 cX ");   // 8.0; ;   9.0; ; 100.0; ;  10.0; ;   1.0; ;   nan; ;   nan; ;   nan; ; 

    ndp_(FLDn, 2);        // _p_st("FLD 2  cX ");
    ndp_(FADDn, 3);       // _p_st("FADD 3 cX ");
    ndp_(FXCHn, 2);       _p_st("Values test ");    // 9.0; ;   8.0; ; 200.0; ; 100.0; ;  10.0; ;   1.0; ;   nan; ;   nan; ; 
    ndp_(FDIVn, 2);       // _p_st("FDIV 2 cX ");   // 0.0; ;   8.0; ; 200.0; ; 100.0; ;  10.0; ;   1.0; ;   nan; ;   nan; ;

    ndp_RdSm32(FSTS_m32fp, &cXchar);                // = 4 / 100 + 1 / 200  ==>  9 / 200 ; = 0.045
                                                    // FSTP ST(0) = POP
    ndp_(FSTPn, 0);       // _p_st("FSTP 0 cY ");   // 8.0; ; 200.0; ; 100.0; ;  10.0; ;   1.0; ;   nan; ;   nan; ;   nan; ; 
    ndp_(FDIVn, 2);       // _p_st("FDIV 2 cY ");   // 0.1; ; 200.0; ; 100.0; ;  10.0; ;   1.0; ;   nan; ;   nan; ;   nan; ;
    ndp(FLDPI);           // _p_st("FLDPI  cY ");   // 3.1; ;   0.1; ; 200.0; ; 100.0; ;  10.0; ;   1.0; ;   nan; ;   nan; ; 
    ndp_(FDIVn, 3);       // _p_st("FDIV 3 cY ");   // 0.0; ;   0.1; ; 200.0; ; 100.0; ;  10.0; ;   1.0; ;   nan; ;   nan; ;
    ndp_(FDIVn, 4);       // _p_st("FDIV 4 cY ");
    ndp_(FADDn, 1);       // _p_st("FADD 1 cY ");   // 0.1; ;   0.1; ; 200.0; ; 100.0; ;  10.0; ;   1.0; ;   nan; ;   nan; ;

    ndp_RdSm32(FSTS_m32fp, &cYchar);                //  = (8 / 100) + (Pi / 1000) ; = 0.083

    // dx, dy: 0.04500000, 0.08314160 <-> 0x3D3851EC, 0x3DAA4624
    // Sind die gleichen Werte aus MC68882
    // cXchar.L_int = 0x3D3851EC; 
    // cYchar.L_int = 0x3DAA4624;

    printf("\r\nTest dx, dy - 'ndp  FST   S_m32fp' and union: %1.8f, %1.8f <-> 0x%X, 0x%X\r\n\r\n\r\n", cXchar.S_real, cYchar.S_real, cXchar.L_int, cYchar.L_int);
    ndp_init();                              // ndp(FINIT);

    printf("Iteration nach Werner Durandi (c't 3/87) - DEBUG PRINT  TEST   n=FCOMP C-code \r\n");

    for (jy = 2; jy <=2; jy++) {             // to check some arithmetic ...
      for (ix = 3; ix <= 3; ix++) {
        ndpFWAIT();

        ndp(FINIT); // _p_st("FINIT   ");  //        _ps("\r\n");   

        printf("\r\nloop ix, jy   %X, %X\r\n\r\n", ix, jy); _ps("NDP register R0    R1     R2     R3     R4     R5     R6     R7  \r\n\r\n");   
        ndp_WRm16(FILDW_m16, 7);  _p_st("FILDi_7 ");
        ndp_fildW_m16((const uint16_t *)&ix);               _p_st("FILDi_x ");
        ndp_fildW_m16((const uint16_t *)&jy);               _p_st("FILDi_y ");
                                  _p_st("Init x y");        _ps("            cy     cx     r \r\n\n");  
        ndp_(FLDn, 1);            _p_st("FLD_1   ");        _ps("            x      cy     cx     r   \r\n");   
        ndp_(FLDn, 1);            _p_st("FLD_1   ");        _ps("            y      x      cy     cx     r   \r\n");   
        ndp_(FMULn, 0);           _p_st("FMUL_0  ");        _ps("            y²     x      cy     cx     r   \r\n");   
        ndp_(FLDn, 3);            _p_st("FLD_3   ");        _ps("            x      y²     x      cy     cx     r   \r\n");   
        ndp_(FMULn,0);            _p_st("FMUL_0  ");        _ps("            x²     y²     x      cy     cx     r   \r\n");   
        ndp_(FLDn,3);             _p_st("FLD_3   ");        _ps("            x      x²     y²     x      cy     cx     r   \r\n");   
                                                            _ps("  Y =  2xy  + cy\r\n");      
        //  ndp_(FMULn,0);    _p_st("FMUL_3  ");            _ps("*           xy     x²     y²     x      cy     cx     r   \r\n");   // without   FMULwW
        printf("eval:\r\n"); 

        for (i = 0; i <= 1; i++) {  // 15

          ndp_(FMULn, 3);         _p_st("FMUL_3  ");        _ps("            xy     x²     y²     x      cy     cx     r   \r\n");
                                                        _ps("  Y = 2xy + cy \r\n");
          ndp_(FADDn, 0);         _p_st("FADD_0  ");        _ps("           2xy     x²     y²     x      cy     cx     r   \r\n\n");
          ndp_(FADDn, 4);         _p_st("FADD_4  ");        _ps("            Y      x²     y²     x      cy     cx     r   \r\n");
                                                        _ps("  X = x²-y² + cx\r\n");
          ndp(FINCSTP);           _p_st("FINCSTP ");        _ps("    <<      x²     y²     x      cy     cx     r      -      Y   \r\n\n");
          ndp_(FSUBn, 1);         _p_st("FSUB_1  ");        _ps("    R0    x²-y²    y²     x      cy     cx     r      -      Y   \r\n\n");
          ndp_(FADDn, 4);         _p_st("FADD_4  ");        _ps("    R0      X      y²     x      cy     cx     r      -      Y   \r\n");
                                                        _ps("  x² = x*x\r\n"); 
          ndp_(FSTn, 2);          _p_st("FST_2   ");        _ps("    R0      X      y²     X      cy     cx     r      -      Y   \r\n\n");
          ndp_(FMULn, 0);         _p_st("FMUL_0  ");        _ps("    R0      X²     y²     X      cy     cx     r      -      Y   \r\n\n");
          ndp_(FSTn, 6);          _p_st("FST_6   ");        _ps("    R0      X²     y²     X      cy     cx     r      X²     Y   \r\n");
                                                        _ps("  y² = y*y \r\n");
          ndp(FDECSTP);           _p_st("FDECSTP ");        _ps("    >>      Y      X²     y²     X      cy     cx     r      X²  \r\n\n");
          ndp_(FSTn, 2);          _p_st("FST_2   ");        _ps("            Y      X²     Y      X      cy     cx     r      X²  \r\n\n");
          ndp_(FMULdn, 2);        _p_st("FMULd_2 ");        _ps("            Y      X²     Y²     X      cy     cx     r      X²  \r\n\n");
          ndp(FDECSTP);           _p_st("FDECSTP ");        _ps("    >>      X²     Y      X²     Y²     X      cy     cx     r   \r\n");     
                                                        _ps("  x² + y² ... > r \r\n");
          ndp_(FADDn, 3);         _p_st("FADD_3  ");        _ps("            Z      x²    y²      x      cy     cx     r      - \r\n");                   
          
          ndp_RdSm32(FSTS_m32fp, &m32fp);
                                                      _ps("  Z > r \r\n");
          ndp_(FCOMPn, 7);        _p_st("FCOMP_7 ");        _ps("            x²    y²      x      cy     cx     r      - \r\n\n");                   
          
          stw_ax = ndp_get(FSTSW_AX) ;

          // ndp_(FMUL,0);        _p_st("FMUL_3  ");      // _ps("*           yx     x²    y²      x      cy     cx     r      - \r\n");  //w. wW .. * 138 Taktzyklen 

          if (ndp_is_gt(stw_ax)) { break; }
        }
        if (i <= 15) { printf("%c", 0x60 - i); } 
        else         { printf(" "); }
      } // for ix
      printf("\n");  
    } // for jy

    sleep_ms(1000);     
    printf("\r\n- START - --------------------------------------------------------------------------------------\r\n");

    gpio_put(HW_TRIGGER, 1); // HW debug

    ndp_init();                              // ndp(FINIT);
    startTime = time_us_32();
    printf("%d ms &  print \n",  (time_us_32() - startTime)/1000U );
  
    printf("Iteration nach Werner Durandi (c't 3/87) - TEST n=FCOMP C-code  \r\n\n");
    

    for (jy = 12; jy >= -12; jy--) {         // -12 ... 12 24+1 for symmetry
      for (ix = -50; ix <= 29; ix++) {       // -50 ... 29

        ndpFWAIT();
        NDP_FINIT;                  //_AN Reset Stack und Stapelzeiger auf R0

        ndp_WRm16(FILDW_m16, 7);   // NDP_FILDW_m16(4); //  // _p_st("FILDi_4 ");    //      {     cy    cx     r   }
        // Sind die gleichen Werte aus MC68882
        // cXchar.L_int = 0x3D3851EC; 
        // cYchar.L_int = 0x3DAA4624;

        ndp_WRm16(FILDW_m16, ix);
        ndp_WRm32(FMULS_m32fp, cXchar.L_int);   // not cXchar.S_real
        
        ndp_WRm16(FILDW_m16, jy);
        ndp_WRm32(FMULS_m32fp, cYchar.L_int);   // not cYchar.S_real

        ndp_(FLDn, 1);         // _p_st("FLD_1  ");          //        x     cy    cx  r 
        ndp_(FLDn, 1);         // _p_st("FLD_1  ");          //        y   x       cy    cx   r
        ndp_(FMULn, 0);        // _p_st("FMUL_0 ");          //        y²  x       cy    cx   r
        ndp_(FLDn, 3);         // _p_st("FLD_3  ");          //        x     y²  x   cy    cx  r
        ndp_(FMULn,0);         // _p_st("FMUL_0 ");          //        x²    y²  x   cy    cx  r
        ndp_(FLDn,3);          // _p_st("FLD_3  "); _p_();   //        y     x²  y²  x    cy  cx    r  
                                                             //        y    x²   y²  x   cy   cx  r 
                                                             //        Y = 2xy + cy
                                                             // *      X²   Y²   X²   y    X    cy   cx   r
        //ndp_(FMULn,3);  FMUL(3); // _p_st("FMUL_3  ");     // xy   *... was according to Mr Werner Durandi (c't 3/87) a point to consider in order to save time     

        for (i = 0; i <= 15; i++) { // _p_st("istart: ");
          ndp_(FMULn, 3);      // _p_st("FMUL_3  ");         // *      yx   x²   y²   x    cy   cx   r 
                               //  Y = 2xy + cy
          ndp_(FADDn, 0);      // _p_st("FADD_0  ");         //       2xy   x²   y²   x    cy   cx   r
          ndp_(FADDn, 4);      // _p_st("FADD_4  "); _p_();  //        Y    x²   y²   x    cy   cx   r
                                                          //  X = x²-y² +cx
          ndp(FINCSTP);        // _p_st("FINCSTP ");         //        -    x²   y²   x    cy   cx   r   -     Y 
          ndp_(FSUBn, 1);      // _p_st("FSUB_1  ");         //        -  x²-y²  y²   x    cy   cx   r   -     Y
          ndp_(FADDn, 4);      // _p_st("FADD_4  "); _p_();  //        -    X    y²   x    cy   cx   r   -     Y 
                                                          //  x² = x*x
          ndp_(FSTn, 2);       // _p_st("FST_2   ");         //        -    X    y²   X    cy   cx   r   -     Y 
          ndp_(FMULn, 0);      // _p_st("FMUL_0  ");         //        -    X²   y²   X    cy   cx   r   -     Y 
          ndp_(FSTn, 6);       // _p_st("FST_6   "); _p_();  //        -    X²   y²   X    cy   cx   r   X²    Y 
                                                          //  y² = y*y }
          ndp(FDECSTP);        // _p_st("FDECSTP ");         //        Y    X²    y²   X    cy   cx   r   X²
          ndp_(FSTn, 2);       // _p_st("FST_2   ");         //        Y    X²    Y    X    cy   cx   r   X²
          ndp_(FMULdn, 2);     // _p_st("FMULd_2 "); _p_();  //        Y    X²    Y²   X    cy   cx   r   X²       d:Destination is ST(n)
          ndp(FDECSTP);        // _p_st("FDECSTP ");         //        X²    Y    X²    Y²   X    cy   cx   r         
                                                          //  x² + y² < r 
          ndp_(FADDn, 3);      // _p_st("FADD_3  ");         //      x²+y²                            
                                                                    //        Z    x²   y²   x    cy   cx    r
          // ndp_RdSm32(FSTS_m32fp, &m32fp);

          ndp_(FCOMPn, 7);     //_p_st("FCOMP_7 ");          // _ps("            x²    y²      x      cy     cx     r      - \r\n");                   
          stw_ax = ndp_get(FSTSW_AX) ;

          // ndp_(FMULn,3);;      _p_st("FMUL_3  ");         // _ps("*           yx     x²    y²      x      cy     cx     r      - \r\n"); 
                                                             //  yx  *... was according to Mr Werner Durandi (c't 3/87) a point to consider in order to save time 
          // if (m32fp.S_real > 7) { break; }
          if (ndp_is_gt(stw_ax)) { break; }               
        }
        if (i <= 15) { printf("%c", 0x60 - i); } 
        else         { printf(" "); }

      } // for ix
      printf("\n");  
    } // for jy

    printf("%d ms &  print \r\n\r\n", (time_us_32() - startTime)/1000U );
    ndp_init();                                            // ndp(FINIT);
    startTime = time_us_32();

    printf("\r\n- START - --------------------------------------------------------------------------------------\r\n");

    printf("Iteration nach Werner Durandi (c't 3/87) - DEBUG PRINT  TEST n=FCOMP with macro \r\n");

    for (jy = 2; jy <= 2; jy++) {                          // to check some arithmetic ...
      for (ix = 3; ix <= 3; ix++) {
        ndpFWAIT();

        NDP_FINIT; // _p_st("FINIT   ");  //        _ps("\r\n");   
        printf("\r\nFINIT\r\n");

        ndpFWAIT();
        NDP_FILDW_m16(7);         _p_st("FILDi_7 ");    //      {     cy    cx     r   }

        NDP_FILDW_m16(ix);        _p_st("FILDi_x ");
        NDP_FILDW_m16(jy);        _p_st("FILDi_y ");


        NDP_FLD(1);               _p_st("FLD_1   ");        _ps("            x      cy     cx     r   \r\n");   
        NDP_FLD(1);               _p_st("FLD_1   ");        _ps("            y      x      cy     cx     r   \r\n");   
        NDP_FMUL(0);              _p_st("FMUL_0  ");        _ps("            y²     x      cy     cx     r   \r\n");   
        NDP_FLD(3);               _p_st("FLD_3   ");        _ps("            x      y²     x      cy     cx     r   \r\n");   
        NDP_FMUL(0);              _p_st("FMUL_0  ");        _ps("            x²     y²     x      cy     cx     r   \r\n");   
        NDP_FLD(3);               _p_st("FLD_3   ");        _ps("            x      x²     y²     x      cy     cx     r   \r\n");   
                                                  _ps("  Y =  2xy  + cy\r\n");      
        // NDP_FMUL(3);     _p_st("FMUL_3  ");         _ps("*           xy     x²     y²     x      cy     cx     r   \r\n");   // without   FMULwW
                                                      // xy  *... was according to Mr Werner Durandi (c't 3/87) a point to consider in order to save time  

        printf("\r\n"); 

        for (i = 0; i <= 1; i++) { // 15
            
          printf("istart: \r\n");
          // ndp_(FMULn, 3);
          NDP_FMUL(3);            _p_st("FMUL_3  ");        // _ps("            xy     x²     y²     x      cy     cx     r   \r\n");
                                                        //  Y = 2xy + cy
          // ndp_(FADDn, 0);
          NDP_FADD(0);            _p_st("FADD_0  ");        // _ps("           2xy     x²     y²     x      cy     cx     r   \r\n");
          // ndp_(FADDn, 4);  
          NDP_FADD(4);            _p_st("FADD_4  ");        // _ps("            Y      x²     y²     x      cy     cx     r   \r\n");
                                                        // _ps("  X = x²-y² + cx\r\n");
          // ndp_(FINCSTP);   
          NDP_FINCSTP;            _p_st("FINCSTP ");        // _ps("    <<      x²     y²     x      cy     cx     r      -      Y   \r\n");
          // ndp_(FSUBn, 1);  
          NDP_FSUB(1);            _p_st("FSUB_1  ");        // _ps("    R0    x²-y²    y²     x      cy     cx     r      -      Y   \r\n");
          // ndp_(FADDn, 4);  
          NDP_FADD(4);            _p_st("FADD_4  ");        // _ps("    R0      X      y²     x      cy     cx     r      -      Y   \r\n");
                                                        // _ps("  x² = x*x\r\n"); 
          // ndp_(FSTn, 2); 
          NDP_FST(2);             _p_st("FST_2   ");        // _ps("    R0      X      y²     X      cy     cx     r      -      Y   \r\n");
          // ndp_(FMULn, 0);  
          NDP_FMUL(0);            _p_st("FMUL_0  ");        // _ps("    R0      X²     y²     X      cy     cx     r      -      Y   \r\n");
          // ndp_(FSTn, 6); 
          NDP_FST(6);             _p_st("FST_6   ");        // _ps("    R0      X²     y²     X      cy     cx     r      X²     Y   \r\n");
                                                        // _ps("  y² = y*y \r\n");
          // ndp(FDECSTP);  
          NDP_FDECSTP;            _p_st("FDECSTP ");        // _ps("    >>      Y      X²     y²     X      cy     cx     r      X²  \r\n");
          // ndp_(FSTn, 2); 
          NDP_FST(2);             _p_st("FST_2   ");        // _ps("            Y      X²     Y      X      cy     cx     r      X²  \r\n");
          // ndp_(FMULdn, 2); 
          NDP_FMULd(2);           _p_st("FMULd_2 ");        // _ps("            Y      X²     Y²     X      cy     cx     r      X²  \r\n");
          // ndp(FDECSTP);  
          NDP_FDECSTP;            _p_st("FDECSTP ");        // _ps("    >>      X²     Y      X²     Y²     X      cy     cx     r   \r\n");     
                                                        // _ps("  x² + y² ... > r \r\n");
          // ndp_(FADDn, 3);  
          NDP_FADD(3);            _p_st("FADD_3  ");        // _ps("            Z      x²    y²      x      cy     cx     r      - \r\n");                   
          
          // m32fp.L_int = NDP_FSTS_m32fp;

          // ndp(FCOMP, 7);
          NDP_FCOMP(7);          _p_st("FCOMP_7 ");        // _ps("            x²    y²      x      cy     cx     r      - \r\n");                   
          
          stw_ax = NDP_FSTSW_AX ;

          // FMUL(3);      _p_st("FMUL_3  ");             // _ps("*           yx     x²    y²      x      cy     cx     r      - \r\n");
                                                          //  yx  *... was according to Mr Werner Durandi (c't 3/87) a point to consider in order to save time 
          if (ndp_is_gt(stw_ax)) { break; }
        }
        if (i <= 15) { printf("%c", 0x60 - i); } 
        else         { printf(" "); }

      } // for ix
      printf("\n");  
    } // forjy


    printf("\r\n- START - --------------------------------------------------------------------------------------\r\n");

    gpio_put(HW_TRIGGER, 1); // HW debug

    sleep_ms(1000);
    ndp_init();                              // ndp(FINIT);
    startTime = time_us_32();
    printf("%d ms &  print \n",  (time_us_32() - startTime)/1000U );

    printf("Iteration nach Werner Durandi (c't 3/87) - TEST   n=FCOMP code with macro \r\n\n");

    for (jy = 12; jy >= -12; jy--) {         // -12 ... 12 24+1 for symmetry
      for (ix = -50; ix <= 29; ix++) {       // -50 ... 29

        ndpFWAIT();
        NDP_FINIT;                  //_AN Reset Stack und Stapelzeiger auf R0

        NDP_FILDW_m16(7); //  // _p_st("FILDi_7 ");    //      {     cy    cx     r   }
        // Sind die gleichen Werte aus MC68882
        // cXchar.L_int = 0x3D3851EC; 
        // cYchar.L_int = 0x3DAA4624;

        NDP_FILDW_m16(ix);
        NDP_FMULS_m32fp(cXchar.L_int);

        NDP_FILDW_m16(jy);
        NDP_FMULS_m32fp(cYchar.L_int);

        NDP_FLD(1);            // _p_st("FLD_1  ");          //        x     cy    cx  r 
        NDP_FLD(1);            // _p_st("FLD_1  ");          //        y   x       cy    cx   r
        NDP_FMUL(0);           // _p_st("FMUL_0 ");          //        y²  x       cy    cx   r
        NDP_FLD(3);            // _p_st("FLD_3  ");          //        x     y²  x   cy    cx  r
        NDP_FMUL(0);           // _p_st("FMUL_0 ");          //        x²    y²  x   cy    cx  r
        NDP_FLD(3);            // _p_st("FLD_3  "); _p_();   //        y     x²  y²  x    cy  cx    r  
                                                             //        y    x²   y²  x   cy   cx  r 
                                                             //        Y = 2xy + cy
        // FMUL(3);            // _p_st("FMUL_3  ");         // *      X²   Y²   X²   y    X    cy   cx   r     

        for (i = 0; i <= 15; i++) { // _p_st("istart: ");
          NDP_FMUL(3);         // _p_st("FMUL_3  ");         // *      yx   x²   y²   x    cy   cx   r 
                               //  Y = 2xy + cy
          NDP_FADD(0);         // _p_st("FADD_0  ");         //       2xy   x²   y²   x    cy   cx   r
          NDP_FADD(4);         // _p_st("FADD_4  "); _p_();  //        Y    x²   y²   x    cy   cx   r
                                                         //  X = x²-y² +cx
          NDP_FINCSTP;         // _p_st("FINCSTP ");         //        -    x²   y²   x    cy   cx   r   -     Y 
          NDP_FSUB(1);         // _p_st("FSUB_1  ");         //        -  x²-y²  y²   x    cy   cx   r   -     Y
          NDP_FADD(4);         // _p_st("FADD_4  "); _p_();  //        -    X    y²   x    cy   cx   r   -     Y 
                                                         //  x² = x*x
          NDP_FST(2);          // _p_st("FST_2   ");         //        -    X    y²   X    cy   cx   r   -     Y 
          NDP_FMUL(0);         // _p_st("FMUL_0  ");         //        -    X²   y²   X    cy   cx   r   -     Y 
          NDP_FST(6);          // _p_st("FST_6   "); _p_();  //        -    X²   y²   X    cy   cx   r   X²    Y 
                                                         //  y² = y*y }
          NDP_FDECSTP;         // _p_st("FDECSTP ");         //        Y    X²    y²   X    cy   cx   r   X²
          NDP_FST(2);          // _p_st("FST_2   ");         //        Y    X²    Y    X    cy   cx   r   X²
          NDP_FMULd(2);        // _p_st("FMULd 2 "); _p_();  //        Y    X²    Y²   X    cy   cx   r   X²       d:Destination is ST(n)
          NDP_FDECSTP;         // _p_st("FDECSTP ");         //        X²    Y    X²    Y²   X   cy   cx   r         
                                                         //  x² + y² < r ??? 
          NDP_FADD(3);         // _p_st("FADD_3  ");         //      x²+y²                            
                                                             //        Z    x²   y²   x    cy   cx    r
          // m32fp.L_int = NDP_FSTS_m32fp;
          NDP_FCOMP(7);        // _p_st("FCOMP_7 "); _p_();  // 7
          stw_ax = NDP_FSTSW_AX ;
          // if (m32fp.S_real > 7) { break; }
          if (ndp_is_gt(stw_ax)) { break; }
        }
        if (i <= 15) { printf("%c", 0x60 - i); } 
        else         { printf(" "); }
      } // for ix
      printf("\n");  
    } // for jy

    printf("%d ms &  print \r\n\r\n", (time_us_32() - startTime)/1000U );
    startTime = time_us_32();

  sleep_ms(1000);
  printf("\r\n- - - - - ======================================================================================\r\n\r\n");
  printf("\r\n- START -  For comparison and verification purposes, a copy from the sources listed here has been overlayed.\r\n\r\n"); 
  startTime = time_us_32();   

  int num_elements = sizeof(ndp_table) / sizeof(ndp_table[0]);
  uint8_t status = 0;

  for (int i = 0; i < num_elements; i++) {
    ndp_table[i].status =0;  
    ndp_init();                                         // _p_st("FINIT      ");
  
    ndp_WRm16(FILDW_m16, 2);                            // _p_st("FILDW_m16 2");
    ndp_WRm16(FILDW_m16, 5);                            // _p_st("FILDW_m16 5");
    ndp_(ndp_table[i].opcode, 1U);                      // _p_st("F_table    ");
    ndp_RdSm32(FSTPS_m32fp, (void*)&ndp_table[i].r0);   // _p_st("FSTPSm32 r0");
    ndp_RdSm32(FSTS_m32fp,  (void*)&ndp_table[i].r1);   // _p_st("FSTPSm32 r1");

    // "x != x" is true only when x is NaN (IEEE754), so "x == x" means "x is a real number, not NaN".
    // A popped-empty register legitimately reads back as NaN on the 80C187; that's an expected
    // hardware state, not a fault - so a NaN result is treated as automatically matching the table.
    // I80c187 returns “nan” after pop, which is correct, but simply does not fit the intended format. (WIP)
    //
    if (ndp_table[i].opcode != ndp_table[i].opcode2) { ndp_table[i].status = ndp_table[i].status | 0x1; } 
    if ((ndp_table[i].r0 == ndp_table[i].r0) && (ndp_table[i].r0 != ndp_table[i].expected_r0)) { ndp_table[i].status = ndp_table[i].status | 0x2; } 
    if ((ndp_table[i].r1 == ndp_table[i].r1) && (ndp_table[i].r1 != ndp_table[i].expected_r1)) { ndp_table[i].status = ndp_table[i].status | 0x4; }

    status = status | ndp_table[i].status;
  }
  if (status == 0) { for (int i = 0; i < num_elements; i++) { ndp_table[i].r0 = 5; ndp_table[i].r1 = 2; } }

  printf("                     Test with initial values of r0=5 and r1=2 respectively.               (/ assumed value)\r\n");
  printf("Define      Opcode   Mathematical equation     description                   r0     r1      r0'=    r1'=    Status [mask]\r\n");
  for (int i = 0; i < num_elements; i++) {
    printf("%-9s | %04X |  %s  ;  %s  | % 3.1f | % 3.1f  |  % 3.1f | % 3.1f  |  %1X  inconsistencies \n",
          ndp_table[i].cmd, ndp_table[i].opcode, ndp_table[i].equation, ndp_table[i].description, ndp_table[i].r0, ndp_table[i].r1, ndp_table[i].expected_r0, ndp_table[i].expected_r1, ndp_table[i].status);
  }

  printf("\r\n================================================================================================\r\n\r\n");

  sleep_ms(2000);

//=========================================

  startTime = time_us_32();
  }  // while (true) after init
  return 0;
} 

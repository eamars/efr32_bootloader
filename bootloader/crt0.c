/**
 * @file bootloader/crt0.c
 * @brief C runtime initilization for Silicon Labs EFM32 (based on Cortex-M4 with FPU) series
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date May, 2017
 */

#include <stdlib.h>
#include <stdint.h>
#include "em_device.h"
#include "em_core.h"
#include "irq.h"

/** Symbols defined by linker script.  These are all VMAs except those
    with a _load__ suffix which are LMAs.  */
extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __copy_table_start__;
extern uint32_t __copy_table_end__;
extern uint32_t __zero_table_start__;
extern uint32_t __zero_table_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __StackTop;

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
void Default_Handler(void);                          /* Default empty handler */
void Reset_Handler(void);                            /* Reset Handler */
void HardFault_Handler(void);
int main (void);
void __libc_init_array (void);

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Cortex-M Processor Exceptions */
void NMI_Handler         (void) __attribute__ ((weak, alias("Default_Handler")));
// void HardFault_Handler   (void) __attribute__ ((weak, alias("Default_Handler")));
void MemManage_Handler   (void) __attribute__ ((weak, alias("Default_Handler")));
void BusFault_Handler    (void) __attribute__ ((weak, alias("Default_Handler")));
void UsageFault_Handler  (void) __attribute__ ((weak, alias("Default_Handler")));
void DebugMon_Handler    (void) __attribute__ ((weak, alias("Default_Handler")));
void SVC_Handler         (void) __attribute__ ((weak, alias("Default_Handler")));
void PendSV_Handler      (void) __attribute__ ((weak, alias("Default_Handler")));
void SysTick_Handler     (void) __attribute__ ((weak, alias("Default_Handler")));

/* Part Specific Interrupts */
void EMU_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void FRC_PRI_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void WDOG0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void WDOG1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void FRC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void MODEM_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void RAC_SEQ_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void RAC_RSM_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void BUFC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void LDMA_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void GPIO_EVEN_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void TIMER0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART0_RX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART0_TX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void ACMP0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void ADC0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void IDAC0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void I2C0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void GPIO_ODD_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void TIMER1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART1_RX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART1_TX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void LEUART0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void PCNT0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CMU_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void MSC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CRYPTO0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void LETIMER0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void AGC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void PROTIMER_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void RTCC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void SYNTH_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CRYOTIMER_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void RFSENSE_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void FPUEH_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void SMU_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void WTIMER0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void WTIMER1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void PCNT1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void PCNT2_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART2_RX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART2_TX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void I2C1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART3_RX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART3_TX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void VDAC0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CSEN_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void LESENSE_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CRYPTO1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void TRNG0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));


/* Exception table this needs to be mapped into flash.  This table
   does not contain the interrupt vectors.  These are allocated dynamically
   in the array exception_table.  */
__attribute__ ((section(".vectors")))
irq_handler_t static_vector_table[] =
{
		// cortex m4 interrupt vectors
		(irq_handler_t)&__StackTop,               /*      Initial Stack Pointer     */
		Reset_Handler,                            /*      Reset Handler             */
		NMI_Handler,                              /*      NMI Handler               */
		HardFault_Handler,                        /*      Hard Fault Handler        */
		MemManage_Handler,                        /*      MPU Fault Handler         */
		BusFault_Handler,                         /*      Bus Fault Handler         */
		UsageFault_Handler,                       /*      Usage Fault Handler       */
		Default_Handler,                          /*      Reserved                  */
		Default_Handler,                          /*      Reserved                  */
		Default_Handler,                          /*      Reserved                  */
		Default_Handler,                          /*      Reserved                  */
		SVC_Handler,                              /*      SVCall Handler            */
		DebugMon_Handler,                         /*      Debug Monitor Handler     */
		Default_Handler,                          /*      Reserved                  */
		PendSV_Handler,                           /*      PendSV Handler            */
		SysTick_Handler,                          /*      SysTick Handler           */
};


/* This needs to be carefully aligned.   */
__attribute__ ((section(".dynamic_vectors")))
irq_handler_t dynamic_vector_table[] =
{
		// cortex m4 interrupt vectors
		(irq_handler_t)&__StackTop,               /*      Initial Stack Pointer     */
		Reset_Handler,                            /*      Reset Handler             */
		NMI_Handler,                              /*      NMI Handler               */
		HardFault_Handler,                        /*      Hard Fault Handler        */
		MemManage_Handler,                        /*      MPU Fault Handler         */
		BusFault_Handler,                         /*      Bus Fault Handler         */
		UsageFault_Handler,                       /*      Usage Fault Handler       */
		Default_Handler,                          /*      Reserved                  */
		Default_Handler,                          /*      Reserved                  */
		Default_Handler,                          /*      Reserved                  */
		Default_Handler,                          /*      Reserved                  */
		SVC_Handler,                              /*      SVCall Handler            */
		DebugMon_Handler,                         /*      Debug Monitor Handler     */
		Default_Handler,                          /*      Reserved                  */
		PendSV_Handler,                           /*      PendSV Handler            */
		SysTick_Handler,                          /*      SysTick Handler           */

		// external interrupts
		EMU_IRQHandler,                       /*  0 - EMU       */
		FRC_PRI_IRQHandler,                       /*  1 - FRC_PRI       */
		WDOG0_IRQHandler,                       /*  2 - WDOG0       */
		WDOG1_IRQHandler,                       /*  3 - WDOG1       */
		FRC_IRQHandler,                       /*  4 - FRC       */
		MODEM_IRQHandler,                       /*  5 - MODEM       */
		RAC_SEQ_IRQHandler,                       /*  6 - RAC_SEQ       */
		RAC_RSM_IRQHandler,                       /*  7 - RAC_RSM       */
		BUFC_IRQHandler,                       /*  8 - BUFC       */
		LDMA_IRQHandler,                       /*  9 - LDMA       */
		GPIO_EVEN_IRQHandler,                       /*  10 - GPIO_EVEN       */
		TIMER0_IRQHandler,                       /*  11 - TIMER0       */
		USART0_RX_IRQHandler,                       /*  12 - USART0_RX       */
		USART0_TX_IRQHandler,                       /*  13 - USART0_TX       */
		ACMP0_IRQHandler,                       /*  14 - ACMP0       */
		ADC0_IRQHandler,                       /*  15 - ADC0       */
		IDAC0_IRQHandler,                       /*  16 - IDAC0       */
		I2C0_IRQHandler,                       /*  17 - I2C0       */
		GPIO_ODD_IRQHandler,                       /*  18 - GPIO_ODD       */
		TIMER1_IRQHandler,                       /*  19 - TIMER1       */
		USART1_RX_IRQHandler,                       /*  20 - USART1_RX       */
		USART1_TX_IRQHandler,                       /*  21 - USART1_TX       */
		LEUART0_IRQHandler,                       /*  22 - LEUART0       */
		PCNT0_IRQHandler,                       /*  23 - PCNT0       */
		CMU_IRQHandler,                       /*  24 - CMU       */
		MSC_IRQHandler,                       /*  25 - MSC       */
		CRYPTO0_IRQHandler,                       /*  26 - CRYPTO0       */
		LETIMER0_IRQHandler,                       /*  27 - LETIMER0       */
		AGC_IRQHandler,                       /*  28 - AGC       */
		PROTIMER_IRQHandler,                       /*  29 - PROTIMER       */
		RTCC_IRQHandler,                       /*  30 - RTCC       */
		SYNTH_IRQHandler,                       /*  31 - SYNTH       */
		CRYOTIMER_IRQHandler,                       /*  32 - CRYOTIMER       */
		RFSENSE_IRQHandler,                       /*  33 - RFSENSE       */
		FPUEH_IRQHandler,                       /*  34 - FPUEH       */
		SMU_IRQHandler,                       /*  35 - SMU       */
		WTIMER0_IRQHandler,                       /*  36 - WTIMER0       */
		WTIMER1_IRQHandler,                       /*  37 - WTIMER1       */
		PCNT1_IRQHandler,                       /*  38 - PCNT1       */
		PCNT2_IRQHandler,                       /*  39 - PCNT2       */
		USART2_RX_IRQHandler,                       /*  40 - USART2_RX       */
		USART2_TX_IRQHandler,                       /*  41 - USART2_TX       */
		I2C1_IRQHandler,                       /*  42 - I2C1       */
		USART3_RX_IRQHandler,                       /*  43 - USART3_RX       */
		USART3_TX_IRQHandler,                       /*  44 - USART3_TX       */
		VDAC0_IRQHandler,                       /*  45 - VDAC0       */
		CSEN_IRQHandler,                       /*  46 - CSEN       */
		LESENSE_IRQHandler,                       /*  47 - LESENSE       */
		CRYPTO1_IRQHandler,                       /*  48 - CRYPTO1       */
		TRNG0_IRQHandler,                       /*  49 - TRNG0       */
		Default_Handler,                          /*  50 - Reserved      */
};

/**
 * @brief Tiny loader that is aligned at 0x0, where MCU starts fetching instruction from FLASH
 *
 * The FLASH is not stable (code aligned at 0x0 has higher chance to be corrupted). Hence, a tiny loader
 * that jump to normal loader at latter location can be considered as an better choice.
 */
__attribute__ ((section(".tiny_loader"))) __attribute__ ((naked))
void tiny_loader(void)
{
	__ASM (
		"bl loader\n"
	);
}

/**
 * @brief Simple loader that set stack pointer and program counter prior to executing any code
 *
 * Since EFR32 will not automatically load static interrupt vector table, this function will load
 * first word from static interrupt vector table as stack pointer and branch to second word with
 * no condition.
 *
 * The function is expected to be executed as soon as the core starts fetching instructions
 * from FLASH memory.
 *
 * Note: no stack is allocated for this function
 */
__attribute__ ((section(".loader"))) __attribute__ ((naked))
void loader(void)
{
	__ASM (
		"ldr r0, =%0\n"             // load absolute address from static interrupt vector table
		"ldr sp, [r0]\n"            // set stack pointer from static interrupt vector table
		"ldr r0, [r0, #4]\n"        // load address of Reset_Handler (1 word offset from SP) from static interrupt vector table
		"blx r0\n"                  // branch to Reset_Handler
	:: "i" ((uint32_t) &static_vector_table) : "r0", "r1"
	);

	// should never execute beyond this point
	while (1)
	{
		continue;
	}
}

/**
 * @brief Reset the microcontroller and copy initialized data from FLASH to SRAM
 *
 * Note: no stack is allocated for this function
 */
__attribute__ ((naked))
void Reset_Handler (void)
{
	// Use static vector table for handling core fault event when coping variables
	SCB->VTOR = (uint32_t) &static_vector_table & SCB_VTOR_TBLOFF_Msk;

	// initialize floating point co-processor
	SystemInit();

	// copy initialized data
	for (register uint32_t *pSrc = &__etext, *pDest = &__data_start__; pDest < &__data_end__; )
	{
		*pDest++ = *pSrc++;
	}

	// copy uninitialized data
	for (register uint32_t *pDest = &__bss_start__; pDest < &__bss_end__; )
	{
		*pDest++ = 0x0UL;
	}

	// Remap the exception table into SRAM to allow dynamic allocation.
	SCB->VTOR = (uint32_t) &dynamic_vector_table & SCB_VTOR_TBLOFF_Msk;

	// Initialise C library.
	__libc_init_array ();

	// call main routine
	// main is not expect to return anything
	main();

	/* Hang.  */
	while (1)
	{
		continue;
	}
}




void Default_Handler(void)
{
	while (1)
	{
		continue;
	}
}

void HardFault_Handler(void)
{
	Default_Handler();
}

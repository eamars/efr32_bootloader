/**
 * @brief Simulated General Purpose Backup Register (GPBR)
 * @file simgpbr.h
 * @author Ran Bao
 * @date Aug, 2017
 */


#include <stdint.h>
#include <string.h>
#include "simgpbr.h"

#define SIMGPBR_MAGIC_NUM_1 0x52d98c18
#define SIMGPBR_MAGIC_NUM_2 0x219b9024

extern uint32_t __SIMGPBR__begin;
extern uint32_t __SIMGPBR__end;


typedef struct __attribute__ ((packed))
{
	uint32_t GPBR[SIMGPBR_DATA_SIZE];
	uint32_t SIMGPBR_MAGIC[SIMGPBR_MAGIC_DATA_SIZE];
} SIMGPBR_t;


void simgpbr_rebuild(void)
{
	// map struct to memory address
	SIMGPBR_t * simgpbr = (SIMGPBR_t *) &__SIMGPBR__begin;

	// clear entries
	memset(simgpbr, 0x0, SIMGPBR_DATA_SIZE);

	// set magic numbers
	simgpbr->SIMGPBR_MAGIC[0] = SIMGPBR_MAGIC_NUM_1;
	simgpbr->SIMGPBR_MAGIC[1] = SIMGPBR_MAGIC_NUM_2;
}
bool simgbpr_is_valid(void)
{
	// map struct to memory address
	SIMGPBR_t * simgpbr = (SIMGPBR_t *) &__SIMGPBR__begin;

	return  (simgpbr->SIMGPBR_MAGIC[0] == SIMGPBR_MAGIC_NUM_1) &
			(simgpbr->SIMGPBR_MAGIC[1] == SIMGPBR_MAGIC_NUM_2) ;
}

uint32_t simgpbr_get(uint32_t index)
{
	// map struct to memory address
	SIMGPBR_t * simgpbr = (SIMGPBR_t *) &__SIMGPBR__begin;

	return simgpbr->GPBR[index];
}

void simgpbr_set(uint32_t index, uint32_t data)
{
	// map struct to memory address
	SIMGPBR_t * simgpbr = (SIMGPBR_t *) &__SIMGPBR__begin;

	simgpbr->GPBR[index] = data;
}

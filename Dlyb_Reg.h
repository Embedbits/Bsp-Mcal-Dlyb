/**
 * \author Mr.Nobody
 * \file Dlyb_Reg.h
 * \ingroup Dlyb
 * \brief Dlyb module Reg component header file
 *
 * This component contains the functionality for register operations. It is
 * private to the module (the register access is not shared with other
 * modules). The DLYB instances are multi-instance: every accessor takes the
 * instance identification and addresses the register of that one instance
 * only.
 *
 */

#ifndef DLYB_REG_DLYB_REG_H
#define DLYB_REG_DLYB_REG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================= INCLUDES =================================== */
#include "Dlyb_Types.h"                     /* Module types definition        */
/* ============================= TYPEDEFS =================================== */

/** \brief List of the DLYB registers used by the module (one set per instance) */
typedef enum
{
    DLYB_REG_CR   = 0u, /**< DLYB control register        */
    DLYB_REG_CFGR     , /**< DLYB configuration register  */
    DLYB_REG_CNT        /**< Count of available registers */
}   dlyb_RegId_t;

/* ========================= SYMBOLIC CONSTANTS ============================= */

/* ========================= EXPORTED MACROS ================================ */

/* ========================= EXPORTED VARIABLES ============================= */

/* ======================== EXPORTED FUNCTIONS ============================== */

/* ---------------------------- Generic accessors --------------------------- */

void                 Dlyb_Set_RegBit            ( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask );
void                 Dlyb_Reset_RegBit          ( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask );
uint32_t             Dlyb_Get_RegBit            ( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask );

void                 Dlyb_Set_RegVal            ( dlyb_PeriphId_t periphId,
                                                  dlyb_RegId_t regId,
                                                  uint32_t regMask,
                                                  uint32_t regValue );
uint32_t             Dlyb_Get_RegVal            ( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t regMask );

/* ----------------------------- Register protocol -------------------------- */

dlyb_RequestState_t  Dlyb_Reg_Wait_RegBitSet    ( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask );

#ifdef __cplusplus
}
#endif

#endif /* DLYB_REG_DLYB_REG_H */

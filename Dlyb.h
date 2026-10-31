/**
 * \author Mr.Nobody
 * \file Dlyb.h
 * \ingroup Dlyb
 * \brief Dlyb module private header file
 *
 * This file contains the functionality private to the module (not shared
 * with the application or other modules through the Port file).
 *
 */

#ifndef DLYB_DLYB_H
#define DLYB_DLYB_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================= INCLUDES =================================== */
#include "Dlyb_Port.h"                      /* Module public interface        */
#include "Dlyb_Reg.h"                       /* Module register component      */
/* ============================= TYPEDEFS =================================== */

/** \brief Run-time state kept by the module for a single DLYB instance */
typedef struct
{
    dlyb_FunctionState_t Used;         /**< DLYB_FUNCTION_ACTIVE once \ref Dlyb_Init succeeded          */
    dlyb_Phase_t         CurrentPhase; /**< Output clock phase currently applied (SEL[3:0])             */
    dlyb_UnitCnt_t       UnitDelay;    /**< Unit delay value currently applied (UNIT[6:0])              */
}   dlyb_State_t;

/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Bound of the wait loop on a register write confirmation flag */
#define DLYB_REG_WAIT_LIMIT                  ( 10000u )

/** Number of selectable output clock phases on the STM32H5 delay block */
#define DLYB_PHASE_CNT_H5                    ( 12u )

/** Largest value accepted by the UNIT[6:0] field */
#define DLYB_UNIT_CNT_MAX                    ( 127u )

/** SEL[3:0] sentinel value enabling every delay cell during length detection (RM0481 27.4.4) */
#define DLYB_CFGR_SEL_ALL_CELLS              ( 12u )

/** Largest bit index of LNG[11:0] checked while looking for the unit delay count (RM0481 27.4.4) */
#define DLYB_LNG_SEARCH_BIT_MAX              ( 10u )

/** Smallest input clock frequency the delay line supports, in Hz (RM0481 27.2) */
#define DLYB_INPUT_CLOCK_MIN_HZ               ( 25000000u )

/** Module version, major component */
#define DLYB_MODULE_VERSION_MAJOR            ( 1u )
/** Module version, minor component */
#define DLYB_MODULE_VERSION_MINOR            ( 0u )
/** Module version, patch component */
#define DLYB_MODULE_VERSION_PATCH            ( 0u )

/* ========================= EXPORTED MACROS ================================ */
/* ========================= EXPORTED VARIABLES ============================= */
/* ======================== EXPORTED FUNCTIONS ============================== */

#ifdef __cplusplus
}
#endif

#endif /* DLYB_DLYB_H */

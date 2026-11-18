/**
 * \author Mr.Nobody
 * \file Dlyb_Port.h
 * \ingroup Dlyb
 * \brief Dlyb module public interface header file
 *
 * This file contains the public interface of the module, identical for all
 * supported families. Include this file to use the module from an application
 * or from another module. Every function is documented at its definition in
 * Dlyb.c.
 *
 */

#ifndef DLYB_DLYB_PORT_H
#define DLYB_DLYB_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================= INCLUDES =================================== */
#include "Dlyb_Types.h"                     /* Module types definition        */
/* ============================= TYPEDEFS =================================== */
/* ========================= SYMBOLIC CONSTANTS ============================= */
/* ========================= EXPORTED MACROS ================================ */
/* ========================= EXPORTED VARIABLES ============================= */
/* ======================== EXPORTED FUNCTIONS ============================== */

/* --------------------------- Module management ----------------------------- */

dlyb_ModuleVersion_t Dlyb_Get_ModuleVersion  ( void );

dlyb_RequestState_t  Dlyb_Init               ( const dlyb_Config_t * const dlybConfig );
dlyb_RequestState_t  Dlyb_Deinit             ( dlyb_PeriphId_t periphId );
void                 Dlyb_Task               ( void );
dlyb_RequestState_t  Dlyb_Get_DefaultConfig  ( dlyb_Config_t * const dlybConfig );

/* ----------------------------- Delay line ----------------------------------- */

dlyb_RequestState_t  Dlyb_Set_PeriphActive   ( dlyb_PeriphId_t periphId );
dlyb_RequestState_t  Dlyb_Set_PeriphInactive ( dlyb_PeriphId_t periphId );
dlyb_RequestState_t  Dlyb_Set_LengthDetect   ( dlyb_PeriphId_t periphId, dlyb_UnitCnt_t * const unitDelayCount );

/* -------------------------------- Phase -------------------------------------- */

dlyb_RequestState_t  Dlyb_Set_Phase          ( dlyb_PeriphId_t periphId, dlyb_Phase_t phase );
dlyb_RequestState_t  Dlyb_Get_Phase          ( dlyb_PeriphId_t periphId, dlyb_Phase_t * const phase );
dlyb_RequestState_t  Dlyb_Get_PhaseCount     ( dlyb_PeriphId_t periphId, dlyb_Phase_t * const phaseCount );

/* -------------------------------- Tuning -------------------------------------- */

dlyb_RequestState_t  Dlyb_Set_Tuning         ( dlyb_PeriphId_t periphId,
                                                const dlyb_TuningConfig_t * const tuningConfig,
                                                dlyb_TuningResult_t * const tuningResult );

#ifdef __cplusplus
}
#endif

#endif /* DLYB_DLYB_PORT_H */

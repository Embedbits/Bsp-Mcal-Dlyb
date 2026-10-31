/**
 * \author Mr.Nobody
 * \defgroup Dlyb Dlyb
 * \brief Dlyb module
 */

/**
 * \author Mr.Nobody
 * \file Dlyb_Types.h
 * \ingroup Dlyb
 * \brief Dlyb module global types definition
 *
 * This file contains the types definitions used across the module and are
 * available for other modules through Port file.
 *
 */

#ifndef DLYB_DLYB_TYPES_H
#define DLYB_DLYB_TYPES_H
/* ============================== INCLUDES ================================== */
#include "stdint.h"                         /* Module types definition        */
#include "Stm32.h"                          /* MCU device header (DLYB_SDMMC1, ...) */
/* ========================== SYMBOLIC CONSTANTS ============================ */

/** Null pointer definition */
#define DLYB_NULL_PTR                         ( ( void* ) 0u )

/* ========================== EXPORTED MACROS ================================ */
/* ============================== TYPEDEFS ================================== */

/** \brief Type signaling major version of SW module */
typedef uint8_t dlyb_MajorVersion_t;

/** \brief Type signaling minor version of SW module */
typedef uint8_t dlyb_MinorVersion_t;

/** \brief Type signaling patch version of SW module */
typedef uint8_t dlyb_PatchVersion_t;

/** \brief Type signaling actual version of SW module */
typedef struct
{
    dlyb_MajorVersion_t Major; /**< Major version */
    dlyb_MinorVersion_t Minor; /**< Minor version */
    dlyb_PatchVersion_t Patch; /**< Patch version */
}   dlyb_ModuleVersion_t;


/** Enumeration used to signal request processing state */
typedef enum
{
    DLYB_REQUEST_ERROR = 0u, /**< Processing request failed  */
    DLYB_REQUEST_OK          /**< Processing request succeed */
}   dlyb_RequestState_t;


/** Function status enumeration */
typedef enum
{
    DLYB_FUNCTION_INACTIVE = 0u, /**< Function status is inactive */
    DLYB_FUNCTION_ACTIVE         /**< Function status is active   */
}   dlyb_FunctionState_t;


/** \brief List of the delay block instances available on the device, each bound to one interface */
typedef enum
{
#if defined(DLYB_SDMMC1)
    DLYB_PERIPH_SDMMC1   = 0u, /**< Delay block of the SDMMC1 interface   */
#endif
#if defined(DLYB_SDMMC2)
    DLYB_PERIPH_SDMMC2       , /**< Delay block of the SDMMC2 interface   */
#endif
#if defined(DLYB_OCTOSPI1)
    DLYB_PERIPH_OCTOSPI1     , /**< Delay block of the OCTOSPI1 interface */
#endif
#if defined(DLYB_OCTOSPI2)
    DLYB_PERIPH_OCTOSPI2     , /**< Delay block of the OCTOSPI2 interface */
#endif
    DLYB_PERIPH_CNT            /**< Count of available delay block instances */
}   dlyb_PeriphId_t;


/**
 * \brief Type used to signal the input clock frequency of an instance, expressed in Hz
 *
 * Use this datatype where the following characteristics are sufficient:
 *  - Range of values : 0 Hz to 4.294 GHz
 *  - Step size        : 1 Hz
 */
typedef uint32_t dlyb_Frequency_Hz_t;


/**
 * \brief Type used to signal the number of unit delays (UNIT[6:0] field of DLYB_CFGR)
 *
 * Use this datatype where the following characteristics are sufficient:
 *  - Range of values : 0 to 127
 */
typedef uint8_t dlyb_UnitCnt_t;


/**
 * \brief Type used to signal the selected output clock phase (SEL[3:0] field of DLYB_CFGR)
 *
 * Use this datatype where the following characteristics are sufficient:
 *  - Range of values : 0 to the value returned by \ref Dlyb_Get_PhaseCount minus one
 */
typedef uint8_t dlyb_Phase_t;


/** \brief Result of one tuning test performed by the caller's test callback */
typedef enum
{
    DLYB_TEST_FAIL = 0u, /**< The tested phase does not sample the data correctly */
    DLYB_TEST_PASS       /**< The tested phase samples the data correctly         */
}   dlyb_TestResult_t;


/**
 * \brief Signature of the test function supplied by the caller for \ref Dlyb_Set_Tuning
 *
 * \param context [in] : Opaque context pointer, forwarded unchanged from \ref dlyb_TuningConfig_t.
 *
 * \return DLYB_TEST_PASS if the sampling at the currently selected phase is correct,
 *         DLYB_TEST_FAIL otherwise.
 */
typedef dlyb_TestResult_t ( *dlyb_TestCallback_t )( void * context );


/** \brief Configuration of one delay block instance, used by \ref Dlyb_Init */
typedef struct
{
    dlyb_PeriphId_t     PeriphId;      /**< Instance to configure                                   */
    dlyb_Frequency_Hz_t InputClock_Hz; /**< Frequency of the clock that passes the delay line, in Hz */
    dlyb_Phase_t        StartPhase;    /**< Output clock phase applied once the block is enabled     */
}   dlyb_Config_t;


/** \brief Configuration of one tuning run, used by \ref Dlyb_Set_Tuning */
typedef struct
{
    dlyb_TestCallback_t TestCallback;    /**< Test function invoked at every tried phase                */
    void *              Context;         /**< Opaque context pointer forwarded to \ref TestCallback     */
    dlyb_Phase_t        FirstPhase;      /**< First phase to try (inclusive)                            */
    dlyb_Phase_t        LastPhase;       /**< Last phase to try (inclusive)                             */
    dlyb_Phase_t        MinWindowWidth;  /**< Smallest passing window width accepted as a valid result  */
}   dlyb_TuningConfig_t;


/** \brief Result of a tuning run, filled by \ref Dlyb_Set_Tuning */
typedef struct
{
    dlyb_Phase_t FirstPassingPhase; /**< First phase of the longest passing window, valid if WindowWidth > 0 */
    dlyb_Phase_t LastPassingPhase;  /**< Last phase of the longest passing window, valid if WindowWidth > 0  */
    dlyb_Phase_t ChosenPhase;       /**< Middle phase of the longest passing window, applied on success      */
    dlyb_Phase_t WindowWidth;       /**< Width of the longest passing window, 0 if no phase passed           */
}   dlyb_TuningResult_t;

/* ========================= EXPORTED VARIABLES ============================= */
/* ======================== EXPORTED FUNCTIONS ============================== */

#endif /* DLYB_DLYB_TYPES_H */

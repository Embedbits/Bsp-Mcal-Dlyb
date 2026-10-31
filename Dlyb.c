/**
 * \author Mr.Nobody
 * \file Dlyb.c
 * \ingroup Dlyb
 * \brief Dlyb module source file
 *
 * Implementation of the delay block (DLYB) abstraction for STM32H5. The
 * module owns the DLYB_CR / DLYB_CFGR registers of every available instance
 * (RM0481/RM0492/RM0517 chapter "Delay block (DLYB)") and offers the length
 * detection, phase selection and tuning procedures described in README.md.
 * The DLYB peripheral clock of every instance is enabled implicitly by the
 * RCC clock enable bit of the interface it serves (SDMMCxEN, OCTOSPIxEN):
 * the module does not call Rcc itself, the caller enables its own interface
 * clock before Dlyb_Init.
 *
 */

/* ============================= INCLUDES =================================== */
#include "Dlyb.h"                           /* Module private header          */
/* ========================== PRIVATE DECLARATIONS =========================== */

/** \brief Register value without any bit set */
#define DLYB_REG_BITS_CLEAR             ( 0u )

/** \brief Mask of the LNG[10:0] bits used by the length detection stop condition (RM0481 27.4.4) */
#define DLYB_LNG_BITS_10_0_MASK         ( 0x7FFu )

/** \brief Mask of the LNG[11] bit used by the length detection stop condition (RM0481 27.4.4) */
#define DLYB_LNG_BIT_11_MASK            ( 0x800u )

/** \brief Mask of the LNG[10] bit used by the length detection stop condition (RM0481 27.4.4) */
#define DLYB_LNG_BIT_10_MASK            ( 0x400u )

/** \brief Smallest window width (one phase) accepted as "a window exists" */
#define DLYB_TUNING_WINDOW_MIN          ( 1u )

/** \brief Divisor used to find the middle phase of the chosen passing window */
#define DLYB_TUNING_MIDDLE_DIVISOR      ( 2u )

/* =========================== LOCAL VARIABLES ============================== */

/** \brief Run-time state of every available DLYB instance, indexed by dlyb_PeriphId_t */
static dlyb_State_t dlyb_State[ DLYB_PERIPH_CNT ];

/* ========================== PRIVATE FUNCTIONS ============================== */

/* ========================= EXPORTED FUNCTIONS =============================== */

/**
 * \brief Gets the module version.
 *
 * \return Module version (major, minor, patch).
 */
dlyb_ModuleVersion_t Dlyb_Get_ModuleVersion( void )
{
    dlyb_ModuleVersion_t moduleVersion;

    moduleVersion.Major = DLYB_MODULE_VERSION_MAJOR;
    moduleVersion.Minor = DLYB_MODULE_VERSION_MINOR;
    moduleVersion.Patch = DLYB_MODULE_VERSION_PATCH;

    return ( moduleVersion );
}


/**
 * \brief Fills a default configuration (first available instance, minimum input clock, phase 0).
 *
 * \param dlybConfig [out] : Configuration filled by the function.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
dlyb_RequestState_t Dlyb_Get_DefaultConfig( dlyb_Config_t * const dlybConfig )
{
    dlyb_RequestState_t retState = DLYB_REQUEST_ERROR;

    if( DLYB_NULL_PTR != dlybConfig )
    {
        dlybConfig->PeriphId      = (dlyb_PeriphId_t)0u;
        dlybConfig->InputClock_Hz = DLYB_INPUT_CLOCK_MIN_HZ;
        dlybConfig->StartPhase    = 0u;

        retState = DLYB_REQUEST_OK;
    }
    else
    {
        /* No configuration to fill, nothing is done */
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Enables a DLYB instance (DEN bit) and applies the last known / configured phase.
 *
 * \param periphId [in] : Instance to activate.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
#if defined(DLYB_CR_DEN_Msk)
dlyb_RequestState_t Dlyb_Set_PeriphActive( dlyb_PeriphId_t periphId )
{
    dlyb_RequestState_t retState = DLYB_REQUEST_ERROR;
    uint32_t             denBit   = DLYB_REG_BITS_CLEAR;

    if( DLYB_PERIPH_CNT > periphId )
    {
        Dlyb_Set_RegBit( periphId, DLYB_REG_CR, DLYB_CR_DEN_Msk );
        denBit = Dlyb_Get_RegBit( periphId, DLYB_REG_CR, DLYB_CR_DEN_Msk );

        if( DLYB_CR_DEN_Msk == denBit )
        {
            retState = DLYB_REQUEST_OK;
        }
        else
        {
            retState = DLYB_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid instance, nothing is done */
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}
#else
dlyb_RequestState_t Dlyb_Set_PeriphActive( dlyb_PeriphId_t periphId )
{
    /* This device line has no delay block (no DLYB_TypeDef in the CMSIS header):
       the function keeps its signature, suppresses the unused parameter and
       returns the module error value. */
    (void)periphId;

    return ( DLYB_REQUEST_ERROR );
}
#endif /* defined(DLYB_CR_DEN_Msk) */


/**
 * \brief Disables a DLYB instance (DEN bit); the interface then samples the undelayed clock.
 *
 * \param periphId [in] : Instance to deactivate.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
#if defined(DLYB_CR_DEN_Msk)
dlyb_RequestState_t Dlyb_Set_PeriphInactive( dlyb_PeriphId_t periphId )
{
    dlyb_RequestState_t retState = DLYB_REQUEST_ERROR;
    uint32_t             denBit   = DLYB_REG_BITS_CLEAR;

    if( DLYB_PERIPH_CNT > periphId )
    {
        Dlyb_Reset_RegBit( periphId, DLYB_REG_CR, DLYB_CR_DEN_Msk );
        denBit = Dlyb_Get_RegBit( periphId, DLYB_REG_CR, DLYB_CR_DEN_Msk );

        if( DLYB_REG_BITS_CLEAR == denBit )
        {
            retState = DLYB_REQUEST_OK;
        }
        else
        {
            retState = DLYB_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid instance, nothing is done */
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}
#else
dlyb_RequestState_t Dlyb_Set_PeriphInactive( dlyb_PeriphId_t periphId )
{
    /* This device line has no delay block: the function keeps its signature,
       suppresses the unused parameter and returns the module error value. */
    (void)periphId;

    return ( DLYB_REQUEST_ERROR );
}
#endif /* defined(DLYB_CR_DEN_Msk) */


/**
 * \brief Configures a DLYB instance and leaves it enabled on the requested start phase.
 *
 * \param dlybConfig [in] : Configuration to apply (instance, input clock, start phase).
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
dlyb_RequestState_t Dlyb_Init( const dlyb_Config_t * const dlybConfig )
{
    dlyb_RequestState_t retState = DLYB_REQUEST_ERROR;
    dlyb_PeriphId_t      periphId = DLYB_PERIPH_CNT;

    if( ( DLYB_NULL_PTR          != dlybConfig                              ) &&
        ( DLYB_PERIPH_CNT        > dlybConfig->PeriphId                     ) &&
        ( DLYB_PHASE_CNT_H5      > dlybConfig->StartPhase                   ) &&
        ( DLYB_INPUT_CLOCK_MIN_HZ <= dlybConfig->InputClock_Hz              ) &&
        ( DLYB_FUNCTION_ACTIVE   != dlyb_State[dlybConfig->PeriphId].Used   )    )
    {
        periphId = dlybConfig->PeriphId;

        dlyb_State[periphId].Used         = DLYB_FUNCTION_ACTIVE;
        dlyb_State[periphId].CurrentPhase = 0u;
        dlyb_State[periphId].UnitDelay    = 0u;

        retState = Dlyb_Set_PeriphActive( periphId );
    }
    else
    {
        /* Instance already used or an invalid parameter, nothing else is done */
        retState = DLYB_REQUEST_ERROR;
    }

    if( DLYB_REQUEST_OK == retState )
    {
        retState = Dlyb_Set_Phase( periphId, dlybConfig->StartPhase );
    }
    else
    {
        /* The block could not be enabled, the start phase is not applied */
    }

    if( ( DLYB_REQUEST_OK != retState ) &&
        ( DLYB_PERIPH_CNT > periphId  )    )
    {
        ( void )Dlyb_Deinit( periphId );
    }
    else
    {
        /* Initialization succeeded, no clean-up required */
    }

    return ( retState );
}


/**
 * \brief Disables and deinitializes a DLYB instance.
 *
 * \param periphId [in] : Instance to deinitialize.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
#if defined(DLYB_CR_DEN_Msk)
dlyb_RequestState_t Dlyb_Deinit( dlyb_PeriphId_t periphId )
{
    dlyb_RequestState_t retState = DLYB_REQUEST_ERROR;

    if( DLYB_PERIPH_CNT > periphId )
    {
        Dlyb_Reset_RegBit( periphId, DLYB_REG_CR, DLYB_CR_SEN_Msk );
        Dlyb_Reset_RegBit( periphId, DLYB_REG_CR, DLYB_CR_DEN_Msk );

        dlyb_State[periphId].Used         = DLYB_FUNCTION_INACTIVE;
        dlyb_State[periphId].CurrentPhase = 0u;
        dlyb_State[periphId].UnitDelay    = 0u;

        retState = DLYB_REQUEST_OK;
    }
    else
    {
        /* Invalid instance, nothing is done */
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}
#else
dlyb_RequestState_t Dlyb_Deinit( dlyb_PeriphId_t periphId )
{
    /* This device line has no delay block: the function keeps its signature,
       suppresses the unused parameter and returns the module error value. */
    (void)periphId;

    return ( DLYB_REQUEST_ERROR );
}
#endif /* defined(DLYB_CR_DEN_Msk) */


/**
 * \brief Module cyclic task.
 *
 * \note Every DLYB operation is handled synchronously by its caller through a bounded
 *       register wait loop (length detection, phase write, tuning): there is no
 *       asynchronous processing pending between two calls, so this function has
 *       nothing to do in this version.
 */
void Dlyb_Task( void )
{
    /* No asynchronous processing pending, nothing to do */
    return;
}


/**
 * \brief Runs the length detection sequence (RM0481/RM0492/RM0517 "Delay line length
 *        configuration procedure"): lengthens the delay line, UNIT value by UNIT value,
 *        until it spans one full input clock period, then reports how many unit delays
 *        that takes.
 *
 * \param periphId        [in]  : Instance to calibrate; the interface must not be sampling
 *                                 through the delay while this runs.
 * \param unitDelayCount   [out] : Number of unit delays spanning one input clock period.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error (clock out of the supported range).
 */
#if defined(DLYB_CR_DEN_Msk)
dlyb_RequestState_t Dlyb_Set_LengthDetect( dlyb_PeriphId_t periphId, dlyb_UnitCnt_t * const unitDelayCount )
{
    dlyb_RequestState_t  retState  = DLYB_REQUEST_ERROR;
    dlyb_RequestState_t  waitState = DLYB_REQUEST_ERROR;
    dlyb_FunctionState_t lengthOk  = DLYB_FUNCTION_INACTIVE;
    uint32_t              lngValue  = DLYB_REG_BITS_CLEAR;
    dlyb_UnitCnt_t        foundUnit = 0u;

    if( ( DLYB_NULL_PTR        != unitDelayCount                     ) &&
        ( DLYB_PERIPH_CNT      > periphId                            ) &&
        ( DLYB_FUNCTION_ACTIVE == dlyb_State[periphId].Used          )    )
    {
        Dlyb_Set_RegBit( periphId, DLYB_REG_CR, DLYB_CR_SEN_Msk );
        Dlyb_Set_RegVal( periphId,
                         DLYB_REG_CFGR,
                         DLYB_CFGR_SEL_Msk,
                         ( (uint32_t)DLYB_CFGR_SEL_ALL_CELLS << DLYB_CFGR_SEL_Pos ) );

        for( uint32_t unitRaw = 0u;
             ( DLYB_UNIT_CNT_MAX   >= unitRaw ) &&
             ( DLYB_FUNCTION_ACTIVE != lengthOk ) ;
             unitRaw++ )
        {
            Dlyb_Set_RegVal( periphId, DLYB_REG_CFGR, DLYB_CFGR_UNIT_Msk, ( unitRaw << DLYB_CFGR_UNIT_Pos ) );
            waitState = Dlyb_Reg_Wait_RegBitSet( periphId, DLYB_REG_CFGR, DLYB_CFGR_LNGF_Msk );

            if( DLYB_REQUEST_OK == waitState )
            {
                lngValue = ( Dlyb_Get_RegVal( periphId, DLYB_REG_CFGR, DLYB_CFGR_LNG_Msk ) >> DLYB_CFGR_LNG_Pos );

                if( ( 0u != ( lngValue & DLYB_LNG_BITS_10_0_MASK )                                    ) &&
                    ( ( 0u == ( lngValue & DLYB_LNG_BIT_11_MASK ) ) ||
                      ( 0u == ( lngValue & DLYB_LNG_BIT_10_MASK ) )                                    )    )
                {
                    lengthOk = DLYB_FUNCTION_ACTIVE;
                }
                else
                {
                    /* Delay line does not yet cover one full input clock period, the loop continues */
                }
            }
            else
            {
                /* The length flag timed out for this UNIT value, the loop tries the next one */
            }
        }

        Dlyb_Reset_RegBit( periphId, DLYB_REG_CR, DLYB_CR_SEN_Msk );

        if( DLYB_FUNCTION_ACTIVE == lengthOk )
        {
            for( uint32_t bitIdx = 0u; DLYB_LNG_SEARCH_BIT_MAX >= bitIdx; bitIdx++ )
            {
                if( 0u != ( lngValue & ( 1u << bitIdx ) ) )
                {
                    foundUnit = (dlyb_UnitCnt_t)bitIdx;
                }
                else
                {
                    /* This bit of LNG is not set, the search for the highest set bit continues */
                }
            }

            dlyb_State[periphId].UnitDelay = foundUnit;
            *unitDelayCount                = foundUnit;
            retState                       = DLYB_REQUEST_OK;
        }
        else
        {
            retState = DLYB_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid parameter or instance not initialized, nothing is done */
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}
#else
dlyb_RequestState_t Dlyb_Set_LengthDetect( dlyb_PeriphId_t periphId, dlyb_UnitCnt_t * const unitDelayCount )
{
    /* This device line has no delay block: the function keeps its signature,
       suppresses the unused parameters and returns the module error value. */
    (void)periphId;
    (void)unitDelayCount;

    return ( DLYB_REQUEST_ERROR );
}
#endif /* defined(DLYB_CR_DEN_Msk) */


/**
 * \brief Selects the output clock phase (SEL[3:0] field of DLYB_CFGR) and reads it back.
 *
 * \param periphId [in] : Instance to configure.
 * \param phase    [in] : Phase to select (0 .. \ref Dlyb_Get_PhaseCount minus one).
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
#if defined(DLYB_CR_DEN_Msk)
dlyb_RequestState_t Dlyb_Set_Phase( dlyb_PeriphId_t periphId, dlyb_Phase_t phase )
{
    dlyb_RequestState_t retState = DLYB_REQUEST_ERROR;
    uint32_t             selValue = DLYB_REG_BITS_CLEAR;

    if( ( DLYB_PERIPH_CNT      > periphId                   ) &&
        ( DLYB_PHASE_CNT_H5    > phase                       ) &&
        ( DLYB_FUNCTION_ACTIVE == dlyb_State[periphId].Used  )    )
    {
        Dlyb_Set_RegBit( periphId, DLYB_REG_CR, DLYB_CR_SEN_Msk );
        Dlyb_Set_RegVal( periphId, DLYB_REG_CFGR, DLYB_CFGR_SEL_Msk, ( (uint32_t)phase << DLYB_CFGR_SEL_Pos ) );
        Dlyb_Reset_RegBit( periphId, DLYB_REG_CR, DLYB_CR_SEN_Msk );

        selValue = ( Dlyb_Get_RegVal( periphId, DLYB_REG_CFGR, DLYB_CFGR_SEL_Msk ) >> DLYB_CFGR_SEL_Pos );

        if( (uint32_t)phase == selValue )
        {
            dlyb_State[periphId].CurrentPhase = phase;
            retState                          = DLYB_REQUEST_OK;
        }
        else
        {
            retState = DLYB_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid parameter or instance not initialized, nothing is done */
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}
#else
dlyb_RequestState_t Dlyb_Set_Phase( dlyb_PeriphId_t periphId, dlyb_Phase_t phase )
{
    /* This device line has no delay block: the function keeps its signature,
       suppresses the unused parameters and returns the module error value. */
    (void)periphId;
    (void)phase;

    return ( DLYB_REQUEST_ERROR );
}
#endif /* defined(DLYB_CR_DEN_Msk) */


/**
 * \brief Gets the output clock phase currently applied (no register access, no side effect).
 *
 * \param periphId [in]  : Instance to query.
 * \param phase    [out] : Phase currently applied.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
dlyb_RequestState_t Dlyb_Get_Phase( dlyb_PeriphId_t periphId, dlyb_Phase_t * const phase )
{
    dlyb_RequestState_t retState = DLYB_REQUEST_ERROR;

    if( ( DLYB_NULL_PTR        != phase                     ) &&
        ( DLYB_PERIPH_CNT      > periphId                   ) &&
        ( DLYB_FUNCTION_ACTIVE == dlyb_State[periphId].Used  )    )
    {
        *phase   = dlyb_State[periphId].CurrentPhase;
        retState = DLYB_REQUEST_OK;
    }
    else
    {
        /* Invalid parameter or instance not initialized, nothing is read */
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Gets the number of selectable output clock phases of the device (no side effect).
 *
 * \param periphId   [in]  : Instance to query.
 * \param phaseCount [out] : Number of selectable phases.
 *
 * \return State of request execution. Returns "OK" if request was success,
 *         otherwise return error.
 */
dlyb_RequestState_t Dlyb_Get_PhaseCount( dlyb_PeriphId_t periphId, dlyb_Phase_t * const phaseCount )
{
    dlyb_RequestState_t retState = DLYB_REQUEST_ERROR;

    if( ( DLYB_NULL_PTR   != phaseCount ) &&
        ( DLYB_PERIPH_CNT > periphId    )    )
    {
        *phaseCount = (dlyb_Phase_t)DLYB_PHASE_CNT_H5;
        retState    = DLYB_REQUEST_OK;
    }
    else
    {
        /* Invalid parameter, nothing is read */
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}


/**
 * \brief Runs the tuning procedure: tries every phase of the requested range with the
 *        caller's test function, then selects the middle of the longest passing window.
 *
 * \param periphId     [in]  : Instance to tune.
 * \param tuningConfig [in]  : Test function, context and phase range to try.
 * \param tuningResult [out] : Result of the tuning (window found, chosen phase).
 *
 * \return State of request execution. Returns "OK" and applies the chosen phase if a
 *         passing window at least \ref dlyb_TuningConfig_t::MinWindowWidth wide was found,
 *         otherwise returns error with an empty \p tuningResult.
 */
dlyb_RequestState_t Dlyb_Set_Tuning( dlyb_PeriphId_t periphId,
                                     const dlyb_TuningConfig_t * const tuningConfig,
                                     dlyb_TuningResult_t * const tuningResult )
{
    dlyb_RequestState_t  retState    = DLYB_REQUEST_ERROR;
    dlyb_RequestState_t  phaseState  = DLYB_REQUEST_ERROR;
    dlyb_TestResult_t     testResult  = DLYB_TEST_FAIL;
    dlyb_FunctionState_t windowOpen  = DLYB_FUNCTION_INACTIVE;
    dlyb_Phase_t          windowStart = 0u;
    dlyb_Phase_t          windowWidth = 0u;
    dlyb_Phase_t          bestStart   = 0u;
    dlyb_Phase_t          bestWidth   = 0u;

    if( ( DLYB_NULL_PTR            != tuningConfig                     ) &&
        ( DLYB_NULL_PTR            != tuningResult                     ) &&
        ( DLYB_NULL_PTR            != tuningConfig->TestCallback       ) &&
        ( DLYB_PERIPH_CNT          > periphId                          ) &&
        ( DLYB_PHASE_CNT_H5        > tuningConfig->LastPhase           ) &&
        ( tuningConfig->LastPhase  >= tuningConfig->FirstPhase         ) &&
        ( DLYB_FUNCTION_ACTIVE     == dlyb_State[periphId].Used        )    )
    {
        for( uint32_t phaseRaw = tuningConfig->FirstPhase; (uint32_t)tuningConfig->LastPhase >= phaseRaw; phaseRaw++ )
        {
            phaseState = Dlyb_Set_Phase( periphId, (dlyb_Phase_t)phaseRaw );

            if( DLYB_REQUEST_OK == phaseState )
            {
                testResult = tuningConfig->TestCallback( tuningConfig->Context );
            }
            else
            {
                testResult = DLYB_TEST_FAIL;
            }

            if( DLYB_TEST_PASS == testResult )
            {
                if( DLYB_FUNCTION_ACTIVE != windowOpen )
                {
                    windowOpen  = DLYB_FUNCTION_ACTIVE;
                    windowStart = (dlyb_Phase_t)phaseRaw;
                    windowWidth = DLYB_TUNING_WINDOW_MIN;
                }
                else
                {
                    windowWidth++;
                }

                if( windowWidth > bestWidth )
                {
                    bestWidth = windowWidth;
                    bestStart = windowStart;
                }
                else
                {
                    /* A shorter or equal window, the best one found so far is kept */
                }
            }
            else
            {
                windowOpen  = DLYB_FUNCTION_INACTIVE;
                windowWidth = 0u;
            }
        }

        if( ( 0u                           != bestWidth              ) &&
            ( tuningConfig->MinWindowWidth  <= bestWidth              )    )
        {
            tuningResult->FirstPassingPhase = bestStart;
            tuningResult->LastPassingPhase  = (dlyb_Phase_t)( bestStart + bestWidth - DLYB_TUNING_WINDOW_MIN );
            tuningResult->ChosenPhase       = (dlyb_Phase_t)( bestStart + ( bestWidth / DLYB_TUNING_MIDDLE_DIVISOR ) );
            tuningResult->WindowWidth       = bestWidth;

            retState = Dlyb_Set_Phase( periphId, tuningResult->ChosenPhase );
        }
        else
        {
            tuningResult->FirstPassingPhase = 0u;
            tuningResult->LastPassingPhase  = 0u;
            tuningResult->ChosenPhase       = 0u;
            tuningResult->WindowWidth       = 0u;

            retState = DLYB_REQUEST_ERROR;
        }
    }
    else
    {
        /* Invalid parameter or instance not initialized, nothing is done */
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}

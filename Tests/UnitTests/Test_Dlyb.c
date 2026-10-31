/**
 * \author Mr.Nobody
 * \file Test_Dlyb.c
 * \ingroup Dlyb
 * \brief Unit tests of the delay block (DLYB) module.
 *
 * Dlyb.c / Dlyb_Reg.c are compiled unchanged. The DLYB instance registers are emulated by
 * RegMem. Dlyb has no dependency on another MCAL module, so no mock is used.
 *
 * \note The length valid flag and the length value (DLYB_CFGR.LNGF / LNG[11:0]) are set by
 *       the hardware itself after a UNIT[6:0] write; plain RegMem does not emulate this, so
 *       the length detection tests activate the HW model \ref UtDlyb_HwModel_LengthDetect,
 *       which sets LNGF and a LNG value depending on the UNIT value already written (see
 *       RegMem_Set_ModelActive() of the RegMem framework).
 *
 * \note STM32H503 has no delay block (no DLYB_TypeDef / DLYB_CR_DEN_Msk in its CMSIS header).
 *       The test runner is generated from this file without running the preprocessor, so
 *       every test case function below is defined unconditionally (same list on all 12
 *       device lines); the device-dependent ones use TEST_IGNORE_MESSAGE on STM32H503 and
 *       the "NoPeriph" tests use it on every other line (their assertion only holds where
 *       DLYB_PERIPH_CNT == 0).
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "RegMem.h"                         /* Register memory emulation      */
#include "Dlyb_Port.h"                      /* Module under test              */
#include "Dlyb.h"                           /* Private interface (test only)  */
#include "Dlyb_Reg.h"                       /* Register access (test only)    */
/* ============================= TYPEDEFS =================================== */
/* ========================== SYMBOLIC CONSTANTS ============================= */

/** Input clock used by most tests (Hz), at the minimum of the supported range */
#define UT_DLYB_CLK_HZ                  ( 25000000u )

/** UNIT[6:0] value at which the HW model reports the delay line length as found */
#define UT_DLYB_TARGET_UNIT             ( 5u )

/** LNG[11:0] value reported by the HW model once UT_DLYB_TARGET_UNIT is reached (bits 0..5 set) */
#define UT_DLYB_TARGET_LNG              ( 0x3Fu )

/** UtDlyb_TestCallback_OneWindow: first / last passing phase of its single window [3..6] */
#define UT_DLYB_ONE_WINDOW_FIRST        ( 3u )
#define UT_DLYB_ONE_WINDOW_LAST         ( 6u )

/** UtDlyb_TestCallback_TwoWindows: first / last passing phase of its short window [1..2] */
#define UT_DLYB_TWO_WINDOWS_A_FIRST     ( 1u )
#define UT_DLYB_TWO_WINDOWS_A_LAST      ( 2u )

/** UtDlyb_TestCallback_TwoWindows: first / last passing phase of its longer window [7..9] */
#define UT_DLYB_TWO_WINDOWS_B_FIRST     ( 7u )
#define UT_DLYB_TWO_WINDOWS_B_LAST      ( 9u )

/** Expected result of the OneWindow tuning: width (6-3+1) and middle (3 + 4/2) phase */
#define UT_DLYB_ONE_WINDOW_WIDTH        ( 4u )
#define UT_DLYB_ONE_WINDOW_CHOSEN       ( 5u )

/** Expected result of the TwoWindows tuning: width (9-7+1) and middle (7 + 3/2) phase of [7..9] */
#define UT_DLYB_TWO_WINDOWS_WIDTH       ( 3u )
#define UT_DLYB_TWO_WINDOWS_CHOSEN      ( 8u )

/** Phase written / read back by \ref Ut_Dlyb_Set_Phase_Nominal_UpdatesCfgrAndState */
#define UT_DLYB_PHASE_ROUNDTRIP         ( 7u )

/** First / last phase of the invalid (reversed) range used by Ut_Dlyb_Set_Tuning_InvalidRange */
#define UT_DLYB_INVALID_RANGE_FIRST     ( 5u )
#define UT_DLYB_INVALID_RANGE_LAST      ( 2u )

/** MinWindowWidth accepting any passing window (the narrowest possible, one phase) */
#define UT_DLYB_MIN_WIDTH_ANY           ( 1u )

/** MinWindowWidth used by the "window below minimum" test, wider than the passing window */
#define UT_DLYB_MIN_WIDTH_STRICT        ( 5u )

/* ======================= FORWARD DECLARATIONS ============================= */

static void UtDlyb_HwModel_LengthDetect( void );

#if defined(DLYB_CR_DEN_Msk)
static dlyb_RequestState_t UtDlyb_InitNominal( void );
static dlyb_TestResult_t   UtDlyb_TestCallback_OneWindow( void * context );
static dlyb_TestResult_t   UtDlyb_TestCallback_TwoWindows( void * context );
static dlyb_TestResult_t   UtDlyb_TestCallback_NoWindow( void * context );
#endif

/* ========================== LOCAL VARIABLES ================================ */

#if defined(DLYB_CR_DEN_Msk)
/** Target instance of \ref UtDlyb_HwModel_LengthDetect, DLYB_NULL_PTR deactivates it */
static DLYB_TypeDef * utDlyb_ModelTarget;
#endif

/* ============================ TEST FIXTURE ================================= */

void setUp( void )
{
    TEST_ASSERT_EQUAL( REGMEM_REQUEST_OK, RegMem_Reset() );
#if defined(DLYB_CR_DEN_Msk)
    utDlyb_ModelTarget = DLYB_NULL_PTR;
#endif
}


void tearDown( void )
{
    ( void )RegMem_Set_ModelInactive();
}

/* ============================ HELPER FUNCTIONS ============================== */

#if defined(DLYB_CR_DEN_Msk)
/** HW model: emulates LNGF / LNG[11:0] of \ref utDlyb_ModelTarget after a UNIT[6:0] write */
static void UtDlyb_HwModel_LengthDetect( void )
{
    uint32_t unitVal = 0u;
    uint32_t lngBits = 0u;
    uint32_t cfgrVal = 0u;

    if( DLYB_NULL_PTR != utDlyb_ModelTarget )
    {
        unitVal = ( ( utDlyb_ModelTarget->CFGR & DLYB_CFGR_UNIT_Msk ) >> DLYB_CFGR_UNIT_Pos );

        if( UT_DLYB_TARGET_UNIT <= unitVal )
        {
            lngBits = UT_DLYB_TARGET_LNG;
        }
        else
        {
            lngBits = 0u;
        }

        cfgrVal = ( ( utDlyb_ModelTarget->CFGR & ~( DLYB_CFGR_LNG_Msk | DLYB_CFGR_LNGF_Msk ) ) |
                    ( lngBits << DLYB_CFGR_LNG_Pos ) | DLYB_CFGR_LNGF_Msk );

        ( void )__atomic_exchange_n( &utDlyb_ModelTarget->CFGR, cfgrVal, __ATOMIC_SEQ_CST );
    }
    else
    {
        /* No instance targeted, nothing to emulate */
    }
}


/** Initializes DLYB_PERIPH_SDMMC1 with the minimum input clock and phase 0, expects success */
static dlyb_RequestState_t UtDlyb_InitNominal( void )
{
    dlyb_Config_t dlybConfig;

    ( void )Dlyb_Get_DefaultConfig( &dlybConfig );
    dlybConfig.PeriphId      = DLYB_PERIPH_SDMMC1;
    dlybConfig.InputClock_Hz = UT_DLYB_CLK_HZ;
    dlybConfig.StartPhase    = 0u;

    return ( Dlyb_Init( &dlybConfig ) );
}


/** Tuning test callback: passes for the single phase window [3..6] */
static dlyb_TestResult_t UtDlyb_TestCallback_OneWindow( void * context )
{
    dlyb_Phase_t      currentPhase = 0u;
    dlyb_TestResult_t result       = DLYB_TEST_FAIL;

    (void)context;
    (void)Dlyb_Get_Phase( DLYB_PERIPH_SDMMC1, &currentPhase );

    if( ( UT_DLYB_ONE_WINDOW_FIRST <= currentPhase ) &&
        ( UT_DLYB_ONE_WINDOW_LAST  >= currentPhase )    )
    {
        result = DLYB_TEST_PASS;
    }
    else
    {
        result = DLYB_TEST_FAIL;
    }

    return ( result );
}


/** Tuning test callback: passes for two disjoint phase windows (a short one, a longer one) */
static dlyb_TestResult_t UtDlyb_TestCallback_TwoWindows( void * context )
{
    dlyb_Phase_t      currentPhase = 0u;
    dlyb_TestResult_t result       = DLYB_TEST_FAIL;

    (void)context;
    (void)Dlyb_Get_Phase( DLYB_PERIPH_SDMMC1, &currentPhase );

    if( ( UT_DLYB_TWO_WINDOWS_A_FIRST <= currentPhase ) &&
        ( UT_DLYB_TWO_WINDOWS_A_LAST  >= currentPhase )    )
    {
        result = DLYB_TEST_PASS;
    }
    else if( ( UT_DLYB_TWO_WINDOWS_B_FIRST <= currentPhase ) &&
             ( UT_DLYB_TWO_WINDOWS_B_LAST  >= currentPhase )    )
    {
        result = DLYB_TEST_PASS;
    }
    else
    {
        result = DLYB_TEST_FAIL;
    }

    return ( result );
}


/** Tuning test callback: never passes */
static dlyb_TestResult_t UtDlyb_TestCallback_NoWindow( void * context )
{
    (void)context;

    return ( DLYB_TEST_FAIL );
}
#else
static void UtDlyb_HwModel_LengthDetect( void )
{
    /* No delay block on this device line, the model has nothing to emulate */
}
#endif /* defined(DLYB_CR_DEN_Msk) */

/* ============================== DEVICE-INDEPENDENT TESTS ===================================== */

/**
 * \par Expected results
 * The returned version matches the module constants.
 */
void Ut_Dlyb_Get_ModuleVersion_Called_ReturnsVersion( void )
{
    dlyb_ModuleVersion_t version = Dlyb_Get_ModuleVersion();

    TEST_ASSERT_EQUAL( DLYB_MODULE_VERSION_MAJOR, version.Major );
    TEST_ASSERT_EQUAL( DLYB_MODULE_VERSION_MINOR, version.Minor );
    TEST_ASSERT_EQUAL( DLYB_MODULE_VERSION_PATCH, version.Patch );
}


/**
 * \par Expected results
 * A NULL output pointer is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Get_DefaultConfig_NullPtr_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Get_DefaultConfig( DLYB_NULL_PTR ) );
}


/**
 * \par Expected results
 * The default configuration uses the minimum input clock and phase 0.
 */
void Ut_Dlyb_Get_DefaultConfig_Called_FillsDefaults( void )
{
    dlyb_Config_t dlybConfig;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Get_DefaultConfig( &dlybConfig ) );
    TEST_ASSERT_EQUAL( DLYB_INPUT_CLOCK_MIN_HZ, dlybConfig.InputClock_Hz );
    TEST_ASSERT_EQUAL( 0u, dlybConfig.StartPhase );
}


/**
 * \par Expected results
 * The cyclic task returns immediately (no asynchronous processing in this version).
 */
void Ut_Dlyb_Task_Called_ReturnsImmediately( void )
{
    Dlyb_Task();
}


/**
 * \par Expected results
 * A NULL output pointer is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Get_PhaseCount_NullPtr_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Get_PhaseCount( (dlyb_PeriphId_t)0u, DLYB_NULL_PTR ) );
}

/* ============================== DEVICE-DEPENDENT TESTS ======================================== */
/* Every test below is defined on all 12 device lines (the generated test runner scans this file
   without running the preprocessor); TEST_IGNORE_MESSAGE is used where the assertion does not
   apply to the device line being built. */

/**
 * \par Expected results
 * A NULL configuration pointer is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Init_NullPtr_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Init( DLYB_NULL_PTR ) );
}


/**
 * \par Expected results
 * An out-of-range instance is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Init_InvalidPeriphId_ReturnsError( void )
{
    dlyb_Config_t dlybConfig;

    ( void )Dlyb_Get_DefaultConfig( &dlybConfig );
    dlybConfig.PeriphId = DLYB_PERIPH_CNT;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Init( &dlybConfig ) );
}


/**
 * \par Expected results
 * An out-of-range start phase is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Init_InvalidStartPhase_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_Config_t dlybConfig;

    ( void )Dlyb_Get_DefaultConfig( &dlybConfig );
    dlybConfig.StartPhase = (dlyb_Phase_t)DLYB_PHASE_CNT_H5;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Init( &dlybConfig ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * An input clock below the supported range is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Init_ClockBelowMinimum_ReturnsError( void )
{
    dlyb_Config_t dlybConfig;

    ( void )Dlyb_Get_DefaultConfig( &dlybConfig );
    dlybConfig.InputClock_Hz = ( DLYB_INPUT_CLOCK_MIN_HZ - 1u );

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Init( &dlybConfig ) );
}


/**
 * \par Expected results
 * A nominal configuration enables the block (DEN = 1) and applies the start phase (SEL = 0).
 */
void Ut_Dlyb_Init_Nominal_EnablesBlockAndAppliesPhase( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );
    TEST_ASSERT_EQUAL( DLYB_CR_DEN_Msk, ( DLYB_SDMMC1->CR & DLYB_CR_DEN_Msk ) );
    TEST_ASSERT_EQUAL( 0u, ( DLYB_SDMMC1->CFGR & DLYB_CFGR_SEL_Msk ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * A second \ref Dlyb_Init of the same, already used, instance is rejected.
 */
void Ut_Dlyb_Init_AlreadyUsed_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_Config_t dlybConfig;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );

    ( void )Dlyb_Get_DefaultConfig( &dlybConfig );
    dlybConfig.PeriphId = DLYB_PERIPH_SDMMC1;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Init( &dlybConfig ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * An out-of-range instance is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Deinit_InvalidPeriphId_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Deinit( DLYB_PERIPH_CNT ) );
}


/**
 * \par Expected results
 * The block is disabled (DEN = 0, SEN = 0) and the instance can be initialized again.
 */
void Ut_Dlyb_Deinit_Nominal_DisablesBlock( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Deinit( DLYB_PERIPH_SDMMC1 ) );
    TEST_ASSERT_EQUAL( 0u, ( DLYB_SDMMC1->CR & ( DLYB_CR_DEN_Msk | DLYB_CR_SEN_Msk ) ) );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * The instance is enabled (DEN read back set) and DLYB_REQUEST_OK is returned.
 */
void Ut_Dlyb_Set_PeriphActive_Nominal_SetsDenBit( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Set_PeriphActive( DLYB_PERIPH_SDMMC1 ) );
    TEST_ASSERT_EQUAL( DLYB_CR_DEN_Msk, ( DLYB_SDMMC1->CR & DLYB_CR_DEN_Msk ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * The instance is disabled (DEN read back clear) and DLYB_REQUEST_OK is returned.
 */
void Ut_Dlyb_Set_PeriphInactive_Nominal_ClearsDenBit( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Set_PeriphActive( DLYB_PERIPH_SDMMC1 ) );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Set_PeriphInactive( DLYB_PERIPH_SDMMC1 ) );
    TEST_ASSERT_EQUAL( 0u, ( DLYB_SDMMC1->CR & DLYB_CR_DEN_Msk ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * A phase requested on a not-initialized instance is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Set_Phase_NotInitialized_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Set_Phase( DLYB_PERIPH_SDMMC1, 0u ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * A phase at or beyond \ref Dlyb_Get_PhaseCount is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Set_Phase_OutOfRange_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR,
                        Dlyb_Set_Phase( DLYB_PERIPH_SDMMC1, (dlyb_Phase_t)DLYB_PHASE_CNT_H5 ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * A valid phase is written to SEL[3:0], read back and cached as the current phase.
 */
void Ut_Dlyb_Set_Phase_Nominal_UpdatesCfgrAndState( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_Phase_t readPhase = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Set_Phase( DLYB_PERIPH_SDMMC1, UT_DLYB_PHASE_ROUNDTRIP ) );
    TEST_ASSERT_EQUAL( UT_DLYB_PHASE_ROUNDTRIP, ( ( DLYB_SDMMC1->CFGR & DLYB_CFGR_SEL_Msk ) >> DLYB_CFGR_SEL_Pos ) );
    TEST_ASSERT_EQUAL( 0u, ( DLYB_SDMMC1->CR & DLYB_CR_SEN_Msk ) );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Get_Phase( DLYB_PERIPH_SDMMC1, &readPhase ) );
    TEST_ASSERT_EQUAL( UT_DLYB_PHASE_ROUNDTRIP, readPhase );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * A NULL output pointer is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Get_Phase_NullPtr_ReturnsError( void )
{
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Get_Phase( (dlyb_PeriphId_t)0u, DLYB_NULL_PTR ) );
}


/**
 * \par Expected results
 * \ref Dlyb_Get_PhaseCount reports 12 selectable phases on STM32H5, with no side effect.
 */
void Ut_Dlyb_Get_PhaseCount_Nominal_ReturnsTwelve( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_Phase_t phaseCount = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Get_PhaseCount( DLYB_PERIPH_SDMMC1, &phaseCount ) );
    TEST_ASSERT_EQUAL( DLYB_PHASE_CNT_H5, phaseCount );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * A length detection requested on a not-initialized instance is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Set_LengthDetect_NotInitialized_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_UnitCnt_t unitCnt = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Set_LengthDetect( DLYB_PERIPH_SDMMC1, &unitCnt ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * The procedure finds the unit delay count reported by the HW model and clears SEN afterwards.
 */
void Ut_Dlyb_Set_LengthDetect_Nominal_FindsUnitCount( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_UnitCnt_t unitCnt = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );

    utDlyb_ModelTarget = DLYB_SDMMC1;
    TEST_ASSERT_EQUAL( REGMEM_REQUEST_OK, RegMem_Set_ModelActive( UtDlyb_HwModel_LengthDetect ) );

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Set_LengthDetect( DLYB_PERIPH_SDMMC1, &unitCnt ) );
    TEST_ASSERT_EQUAL( UT_DLYB_TARGET_UNIT, unitCnt );
    TEST_ASSERT_EQUAL( 0u, ( DLYB_SDMMC1->CR & DLYB_CR_SEN_Msk ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * When the length flag never settles (clock out of the supported range) the procedure times
 * out and returns DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Set_LengthDetect_ClockNeverSettles_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_UnitCnt_t unitCnt = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Set_LengthDetect( DLYB_PERIPH_SDMMC1, &unitCnt ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * NULL configuration / result pointers and a missing test callback are rejected.
 */
void Ut_Dlyb_Set_Tuning_NullParams_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_TuningConfig_t tuningConfig;
    dlyb_TuningResult_t tuningResult;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );

    tuningConfig.TestCallback   = UtDlyb_TestCallback_OneWindow;
    tuningConfig.Context        = DLYB_NULL_PTR;
    tuningConfig.FirstPhase     = 0u;
    tuningConfig.LastPhase      = ( DLYB_PHASE_CNT_H5 - 1u );
    tuningConfig.MinWindowWidth = UT_DLYB_MIN_WIDTH_ANY;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR,
                        Dlyb_Set_Tuning( DLYB_PERIPH_SDMMC1, DLYB_NULL_PTR, &tuningResult ) );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR,
                        Dlyb_Set_Tuning( DLYB_PERIPH_SDMMC1, &tuningConfig, DLYB_NULL_PTR ) );

    tuningConfig.TestCallback = DLYB_NULL_PTR;
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR,
                        Dlyb_Set_Tuning( DLYB_PERIPH_SDMMC1, &tuningConfig, &tuningResult ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * A phase range with LastPhase before FirstPhase is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Set_Tuning_InvalidRange_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_TuningConfig_t tuningConfig;
    dlyb_TuningResult_t tuningResult;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );

    tuningConfig.TestCallback   = UtDlyb_TestCallback_OneWindow;
    tuningConfig.Context        = DLYB_NULL_PTR;
    tuningConfig.FirstPhase     = UT_DLYB_INVALID_RANGE_FIRST;
    tuningConfig.LastPhase      = UT_DLYB_INVALID_RANGE_LAST;
    tuningConfig.MinWindowWidth = UT_DLYB_MIN_WIDTH_ANY;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR,
                        Dlyb_Set_Tuning( DLYB_PERIPH_SDMMC1, &tuningConfig, &tuningResult ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * The single passing window [3..6] is found and the middle phase (5) is chosen and applied.
 */
void Ut_Dlyb_Set_Tuning_OneWindow_ChoosesMiddle( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_TuningConfig_t tuningConfig;
    dlyb_TuningResult_t tuningResult;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );

    tuningConfig.TestCallback   = UtDlyb_TestCallback_OneWindow;
    tuningConfig.Context        = DLYB_NULL_PTR;
    tuningConfig.FirstPhase     = 0u;
    tuningConfig.LastPhase      = ( DLYB_PHASE_CNT_H5 - 1u );
    tuningConfig.MinWindowWidth = UT_DLYB_MIN_WIDTH_ANY;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Set_Tuning( DLYB_PERIPH_SDMMC1, &tuningConfig, &tuningResult ) );
    TEST_ASSERT_EQUAL( UT_DLYB_ONE_WINDOW_FIRST, tuningResult.FirstPassingPhase );
    TEST_ASSERT_EQUAL( UT_DLYB_ONE_WINDOW_LAST,  tuningResult.LastPassingPhase );
    TEST_ASSERT_EQUAL( UT_DLYB_ONE_WINDOW_WIDTH, tuningResult.WindowWidth );
    TEST_ASSERT_EQUAL( UT_DLYB_ONE_WINDOW_CHOSEN, tuningResult.ChosenPhase );
    TEST_ASSERT_EQUAL( UT_DLYB_ONE_WINDOW_CHOSEN, ( ( DLYB_SDMMC1->CFGR & DLYB_CFGR_SEL_Msk ) >> DLYB_CFGR_SEL_Pos ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * Of the two passing windows ([1..2], [7..9]) the longer one is chosen, phase 8 (its middle).
 */
void Ut_Dlyb_Set_Tuning_TwoWindows_ChoosesLongest( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_TuningConfig_t tuningConfig;
    dlyb_TuningResult_t tuningResult;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );

    tuningConfig.TestCallback   = UtDlyb_TestCallback_TwoWindows;
    tuningConfig.Context        = DLYB_NULL_PTR;
    tuningConfig.FirstPhase     = 0u;
    tuningConfig.LastPhase      = ( DLYB_PHASE_CNT_H5 - 1u );
    tuningConfig.MinWindowWidth = UT_DLYB_MIN_WIDTH_ANY;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Set_Tuning( DLYB_PERIPH_SDMMC1, &tuningConfig, &tuningResult ) );
    TEST_ASSERT_EQUAL( UT_DLYB_TWO_WINDOWS_B_FIRST, tuningResult.FirstPassingPhase );
    TEST_ASSERT_EQUAL( UT_DLYB_TWO_WINDOWS_B_LAST,  tuningResult.LastPassingPhase );
    TEST_ASSERT_EQUAL( UT_DLYB_TWO_WINDOWS_WIDTH,   tuningResult.WindowWidth );
    TEST_ASSERT_EQUAL( UT_DLYB_TWO_WINDOWS_CHOSEN,  tuningResult.ChosenPhase );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * No phase passes: DLYB_REQUEST_ERROR is returned and the result is the empty window.
 */
void Ut_Dlyb_Set_Tuning_NoWindow_ReturnsErrorEmptyResult( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_TuningConfig_t tuningConfig;
    dlyb_TuningResult_t tuningResult;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );

    tuningConfig.TestCallback   = UtDlyb_TestCallback_NoWindow;
    tuningConfig.Context        = DLYB_NULL_PTR;
    tuningConfig.FirstPhase     = 0u;
    tuningConfig.LastPhase      = ( DLYB_PHASE_CNT_H5 - 1u );
    tuningConfig.MinWindowWidth = UT_DLYB_MIN_WIDTH_ANY;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Set_Tuning( DLYB_PERIPH_SDMMC1, &tuningConfig, &tuningResult ) );
    TEST_ASSERT_EQUAL( 0u, tuningResult.FirstPassingPhase );
    TEST_ASSERT_EQUAL( 0u, tuningResult.LastPassingPhase );
    TEST_ASSERT_EQUAL( 0u, tuningResult.ChosenPhase );
    TEST_ASSERT_EQUAL( 0u, tuningResult.WindowWidth );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * A passing window narrower than MinWindowWidth is rejected with DLYB_REQUEST_ERROR.
 */
void Ut_Dlyb_Set_Tuning_WindowBelowMinWidth_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_TuningConfig_t tuningConfig;
    dlyb_TuningResult_t tuningResult;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, UtDlyb_InitNominal() );

    tuningConfig.TestCallback   = UtDlyb_TestCallback_OneWindow;
    tuningConfig.Context        = DLYB_NULL_PTR;
    tuningConfig.FirstPhase     = 0u;
    tuningConfig.LastPhase      = ( DLYB_PHASE_CNT_H5 - 1u );
    tuningConfig.MinWindowWidth = UT_DLYB_MIN_WIDTH_STRICT;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Set_Tuning( DLYB_PERIPH_SDMMC1, &tuningConfig, &tuningResult ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}

/* ========================= STM32H503 "NO DELAY BLOCK" TESTS =================================== */
/* DLYB_PERIPH_CNT == 0 on STM32H503 only: on every other line periphId 0 is a valid instance
   (DLYB_PERIPH_SDMMC1), so these assertions are ignored there. */

/**
 * \par Expected results
 * \ref Dlyb_Set_PeriphActive / \ref Dlyb_Set_PeriphInactive keep their signature and return
 * DLYB_REQUEST_ERROR when the device has no delay block.
 */
void Ut_Dlyb_NoPeriph_SetPeriphActiveInactive_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_IGNORE_MESSAGE( "This device line has a delay block" );
#else
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Set_PeriphActive( (dlyb_PeriphId_t)0u ) );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Set_PeriphInactive( (dlyb_PeriphId_t)0u ) );
#endif
}


/**
 * \par Expected results
 * \ref Dlyb_Deinit keeps its signature and returns DLYB_REQUEST_ERROR when the device has no
 * delay block.
 */
void Ut_Dlyb_NoPeriph_Deinit_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_IGNORE_MESSAGE( "This device line has a delay block" );
#else
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Deinit( (dlyb_PeriphId_t)0u ) );
#endif
}


/**
 * \par Expected results
 * \ref Dlyb_Set_LengthDetect keeps its signature and returns DLYB_REQUEST_ERROR when the device
 * has no delay block.
 */
void Ut_Dlyb_NoPeriph_SetLengthDetect_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_IGNORE_MESSAGE( "This device line has a delay block" );
#else
    dlyb_UnitCnt_t unitCnt = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Set_LengthDetect( (dlyb_PeriphId_t)0u, &unitCnt ) );
#endif
}


/**
 * \par Expected results
 * \ref Dlyb_Set_Phase keeps its signature and returns DLYB_REQUEST_ERROR when the device has no
 * delay block.
 */
void Ut_Dlyb_NoPeriph_SetPhase_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_IGNORE_MESSAGE( "This device line has a delay block" );
#else
    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Set_Phase( (dlyb_PeriphId_t)0u, 0u ) );
#endif
}


/**
 * \par Expected results
 * \ref Dlyb_Get_PhaseCount reports no selectable phase (invalid instance, DLYB_PERIPH_CNT == 0)
 * when the device has no delay block.
 */
void Ut_Dlyb_NoPeriph_GetPhaseCount_ReturnsError( void )
{
#if defined(DLYB_CR_DEN_Msk)
    TEST_IGNORE_MESSAGE( "This device line has a delay block" );
#else
    dlyb_Phase_t phaseCount = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_ERROR, Dlyb_Get_PhaseCount( (dlyb_PeriphId_t)0u, &phaseCount ) );
#endif
}

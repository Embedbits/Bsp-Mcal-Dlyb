/**
 * \author Mr.Nobody
 * \file ItTest_Dlyb.c
 * \ingroup Dlyb
 * \brief Integration tests of the delay block (DLYB) module on target.
 *
 * Dlyb module runs on the MCU together with the real DLYB_SDMMC1 hardware. Tests
 * verify behavior which cannot be verified by unit tests (emulated registers):
 * the block enable, the phase selection round trip and the length detection
 * sequence against the real DLYB_CR / DLYB_CFGR registers.
 *
 * Used resources:
 * - DLYB_SDMMC1, SDMMC1 kernel clock (gates the DLYB_SDMMC1 register clock,
 *   RM0481 RCC chapter, AHB4ENR.SDMMC1EN: "SDMMC1 and SDMMC1 delay peripheral
 *   clock enable"), started through the public Rcc API by the test itself -
 *   Dlyb does not call Rcc (see README.md, STM32H5 note on the Rcc dependency)
 * - no external wiring; the SDMMC1 interface itself is not configured, so the
 *   length detection test may not find a stable clock period on every board
 *   (the input clock of the delay line is the SDMMC1 kernel clock, free
 *   running once enabled, but its frequency / stability without the interface
 *   configured is not guaranteed by this test set)
 *
 * \note These tests compile and link for the target but are never run on
 *       hardware by the agent that wrote them (no board access); the actual
 *       behavior on a real board is unverified.
 * \note STM32H503 has no delay block: on that line every test is ignored
 *       (TEST_IGNORE_MESSAGE), the generated test runner is not preprocessor
 *       aware so every test case function below is defined unconditionally.
 */

/* ============================= INCLUDES =================================== */
#include "unity.h"                          /* Unity testing framework        */
#include "IntegrationTesting.h"             /* Integration testing on target  */
#include "Dlyb_Port.h"                      /* Module under test              */
#if defined(DLYB_CR_DEN_Msk)
#include "Rcc_Port.h"                       /* Enables the SDMMC1 clock        */
#endif
/* ============================= TYPEDEFS =================================== */

/* ======================= FORWARD DECLARATIONS ============================= */

/* ========================= SYMBOLIC CONSTANTS ============================= */

/** Input clock used by the tests (Hz), at the minimum of the supported range */
#define IT_DLYB_CLK_HZ                       ( 25000000u )

/** Number of selectable output clock phases expected on STM32H5 (README.md, "Concept") */
#define IT_DLYB_PHASE_CNT_EXPECTED           ( 12u )

/** Phase written / read back by \ref It_Dlyb_Set_Phase_RoundTrip */
#define IT_DLYB_PHASE_ROUNDTRIP              ( 6u )

/* ========================== LOCAL VARIABLES =============================== */

/* ============================= TEST SETUP ================================= */

void setUp( void )
{
    /* Nothing to reset before a test case (fresh boot, see ItTarget_SystemReset) */
}


void tearDown( void )
{
    /* Peripheral state is cleared by system reset after the test case */
}

/* =============================== TESTS ==================================== */

/**
 * \par Expected results
 * DLYB_SDMMC1 is enabled (DEN read back set) once its register clock is gated on.
 */
void It_Dlyb_Init_Nominal_EnablesBlock( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_Config_t dlybConfig;

    TEST_ASSERT_EQUAL( RCC_REQUEST_OK, Rcc_Set_PeriphActive( RCC_PERIPH_SDMMC1_PLL1Q ) );

    (void)Dlyb_Get_DefaultConfig( &dlybConfig );
    dlybConfig.PeriphId      = DLYB_PERIPH_SDMMC1;
    dlybConfig.InputClock_Hz = IT_DLYB_CLK_HZ;
    dlybConfig.StartPhase    = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Init( &dlybConfig ) );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Deinit( DLYB_PERIPH_SDMMC1 ) );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * A phase written through \ref Dlyb_Set_Phase is read back unchanged by \ref Dlyb_Get_Phase.
 */
void It_Dlyb_Set_Phase_RoundTrip( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_Config_t dlybConfig;
    dlyb_Phase_t  readPhase = 0u;

    TEST_ASSERT_EQUAL( RCC_REQUEST_OK, Rcc_Set_PeriphActive( RCC_PERIPH_SDMMC1_PLL1Q ) );

    (void)Dlyb_Get_DefaultConfig( &dlybConfig );
    dlybConfig.PeriphId      = DLYB_PERIPH_SDMMC1;
    dlybConfig.InputClock_Hz = IT_DLYB_CLK_HZ;
    dlybConfig.StartPhase    = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Init( &dlybConfig ) );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Set_Phase( DLYB_PERIPH_SDMMC1, IT_DLYB_PHASE_ROUNDTRIP ) );
    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Get_Phase( DLYB_PERIPH_SDMMC1, &readPhase ) );
    TEST_ASSERT_EQUAL( IT_DLYB_PHASE_ROUNDTRIP, readPhase );

    (void)Dlyb_Deinit( DLYB_PERIPH_SDMMC1 );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * \ref Dlyb_Get_PhaseCount reports 12 selectable phases on STM32H5.
 */
void It_Dlyb_Get_PhaseCount_Nominal_ReturnsTwelve( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_Phase_t phaseCount = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Get_PhaseCount( DLYB_PERIPH_SDMMC1, &phaseCount ) );
    TEST_ASSERT_EQUAL( IT_DLYB_PHASE_CNT_EXPECTED, phaseCount );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}


/**
 * \par Expected results
 * The length detection sequence completes (OK) or reports a bounded timeout (ERROR); it
 * never hangs. The actual pass/fail on hardware depends on the free-running SDMMC1 kernel
 * clock of the board and is not verified by the agent that wrote this test (no board access).
 */
void It_Dlyb_Set_LengthDetect_Completes( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_Config_t        dlybConfig;
    dlyb_UnitCnt_t        unitCnt  = 0u;
    dlyb_RequestState_t   retState = DLYB_REQUEST_ERROR;

    TEST_ASSERT_EQUAL( RCC_REQUEST_OK, Rcc_Set_PeriphActive( RCC_PERIPH_SDMMC1_PLL1Q ) );

    (void)Dlyb_Get_DefaultConfig( &dlybConfig );
    dlybConfig.PeriphId      = DLYB_PERIPH_SDMMC1;
    dlybConfig.InputClock_Hz = IT_DLYB_CLK_HZ;
    dlybConfig.StartPhase    = 0u;

    TEST_ASSERT_EQUAL( DLYB_REQUEST_OK, Dlyb_Init( &dlybConfig ) );
    retState = Dlyb_Set_LengthDetect( DLYB_PERIPH_SDMMC1, &unitCnt );

    /* retState is dlyb_RequestState_t (DLYB_REQUEST_OK or DLYB_REQUEST_ERROR): the call always
       returns within the bounded wait loops of Dlyb_Set_LengthDetect, it never hangs. Which of
       the two values it is on real hardware depends on the board and is not asserted here. */
    (void)retState;

    (void)Dlyb_Deinit( DLYB_PERIPH_SDMMC1 );
#else
    TEST_IGNORE_MESSAGE( "No delay block on this device line" );
#endif
}

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
 * - DLYB_SDMMC1, SDMMC1 clock (gates the DLYB_SDMMC1 register clock, RM0433 RCC
 *   chapter, AHB3ENR.SDMMC1EN: "SDMMC1 and SDMMC1 Delay Clock Enable"; RM0477
 *   AHB5ENR.SDMMC1EN: "SDMMC1 and DB_SDMMC1 peripheral clocks enable"), enabled
 *   through the public Rcc API by the test itself - Dlyb does not call Rcc (see
 *   README.md, note on the Rcc dependency)
 * - no external wiring; the SDMMC1 interface itself is not configured, so the
 *   length detection test may not find a stable clock period on every board
 *   (the input clock of the delay line is the SDMMC1 kernel clock, free
 *   running once enabled, but its frequency / stability without the interface
 *   configured is not guaranteed by this test set)
 *
 * \note Every STM32H7 and STM32H7R / STM32H7S line has DLYB_SDMMC1. The variant
 *       of a device line without a delay block (STM32H503 in the STM32H5 family)
 *       ignores every test (TEST_IGNORE_MESSAGE); the generated test runner is
 *       not preprocessor aware so every test case function below is defined
 *       unconditionally.
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

/** Number of selectable output clock phases expected on STM32H7 (README.md, "Concept") */
#define IT_DLYB_PHASE_CNT_EXPECTED           ( 12u )

/** Phase written / read back by \ref It_Dlyb_Set_Phase_RoundTrip */
#define IT_DLYB_PHASE_ROUNDTRIP              ( 6u )

#if defined(STM32H7RS)
/** Rcc item of the SDMMC1 clock (kernel clock PLL2 S, reset value of SDMMC12SEL on STM32H7R / STM32H7S) */
#define IT_DLYB_SDMMC1_RCC                   ( RCC_PERIPH_SDMMC1_PLL2S )
#else
/** Rcc item of the SDMMC1 clock (kernel clock PLL1 Q, reset value of SDMMCSEL on the classic STM32H7 lines) */
#define IT_DLYB_SDMMC1_RCC                   ( RCC_PERIPH_SDMMC1_PLL1Q )
#endif /* STM32H7RS */

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

    TEST_ASSERT_EQUAL( RCC_REQUEST_OK, Rcc_Set_PeriphActive( IT_DLYB_SDMMC1_RCC ) );

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

    TEST_ASSERT_EQUAL( RCC_REQUEST_OK, Rcc_Set_PeriphActive( IT_DLYB_SDMMC1_RCC ) );

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
 * \ref Dlyb_Get_PhaseCount reports 12 selectable phases on STM32H7.
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
 * never hangs. The result depends on the input clock of the delay line (the SDMMC1 interface
 * itself is not configured by this test set) and is not asserted.
 */
void It_Dlyb_Set_LengthDetect_Completes( void )
{
#if defined(DLYB_CR_DEN_Msk)
    dlyb_Config_t        dlybConfig;
    dlyb_UnitCnt_t        unitCnt  = 0u;
    dlyb_RequestState_t   retState = DLYB_REQUEST_ERROR;

    TEST_ASSERT_EQUAL( RCC_REQUEST_OK, Rcc_Set_PeriphActive( IT_DLYB_SDMMC1_RCC ) );

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

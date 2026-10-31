/**
 * \author Mr.Nobody
 * \file Dlyb_Reg.c
 * \ingroup Dlyb
 * \brief Dlyb module Reg component source file
 *
 * This component contains the functionality for register operations of the
 * multi-instance DLYB peripheral: every accessor takes the instance
 * identification in addition to the register identification, the address is
 * built from the instance base address and the byte offset of the register
 * inside the CMSIS DLYB_TypeDef structure.
 *
 * STM32H503 has no delay block at all (no DLYB_TypeDef in its CMSIS header,
 * DLYB_PERIPH_CNT == 0 for it); on that device line every accessor below
 * keeps its signature, suppresses the unused parameters and returns the
 * module error / "no action" value instead of referencing the DLYB
 * registers.
 *
 */

/* ============================= INCLUDES =================================== */
#include "Dlyb_Reg.h"                       /* Module Reg component header    */
#include "stddef.h"                         /* offsetof() definition          */
/* ========================== PRIVATE DECLARATIONS =========================== */

/** \brief Count of read-back iterations of a wait on a register bit */
#define DLYB_REG_WAIT_TIMEOUT_RAW      ( 10000u )

/** \brief Register value without any bit set */
#define DLYB_REG_BITS_CLEAR            ( 0u )

#if defined(DLYB_CR_DEN_Msk)
/* =========================== LOCAL VARIABLES ============================== */

/** \brief Base address of every available DLYB instance, indexed by dlyb_PeriphId_t */
static DLYB_TypeDef * const dlyb_InstanceBase[ DLYB_PERIPH_CNT ] =
{
#if defined(DLYB_SDMMC1)
    DLYB_SDMMC1  , /**< DLYB_PERIPH_SDMMC1 base address   */
#endif
#if defined(DLYB_SDMMC2)
    DLYB_SDMMC2  , /**< DLYB_PERIPH_SDMMC2 base address   */
#endif
#if defined(DLYB_OCTOSPI1)
    DLYB_OCTOSPI1, /**< DLYB_PERIPH_OCTOSPI1 base address */
#endif
#if defined(DLYB_OCTOSPI2)
    DLYB_OCTOSPI2, /**< DLYB_PERIPH_OCTOSPI2 base address */
#endif
};

_Static_assert( ( sizeof( dlyb_InstanceBase ) / sizeof( DLYB_TypeDef * ) ) == DLYB_PERIPH_CNT,
                 "Dlyb_Reg: dlyb_InstanceBase has incorrect size." );


/** \brief Byte offset of every used DLYB register inside DLYB_TypeDef, indexed by dlyb_RegId_t */
static const uint32_t dlyb_RegOffset[ DLYB_REG_CNT ] =
{
    offsetof( DLYB_TypeDef, CR   ), /**< DLYB_REG_CR   */
    offsetof( DLYB_TypeDef, CFGR ), /**< DLYB_REG_CFGR */
};

_Static_assert( ( sizeof( dlyb_RegOffset ) / sizeof( uint32_t ) ) == DLYB_REG_CNT,
                 "Dlyb_Reg: dlyb_RegOffset has incorrect size." );

/* ========================== PRIVATE FUNCTIONS ============================== */

/**
 * \brief Builds the register address of an instance / register pair
 *
 * \param periphId [in] : Instance identification.
 * \param regId    [in] : Register identification.
 *
 * \return Address of the register, or DLYB_NULL_PTR for an invalid parameter.
 */
static volatile uint32_t * Dlyb_Reg_GetAddr( dlyb_PeriphId_t periphId, dlyb_RegId_t regId )
{
    volatile uint32_t *regAddr = DLYB_NULL_PTR;

    if( ( DLYB_PERIPH_CNT > periphId ) &&
        ( DLYB_REG_CNT    > regId    )    )
    {
        regAddr = (volatile uint32_t *)( (uint8_t *)dlyb_InstanceBase[periphId] + dlyb_RegOffset[regId] );
    }
    else
    {
        /* Invalid instance or register ID, no address is built */
        regAddr = DLYB_NULL_PTR;
    }

    return ( regAddr );
}

/* ========================= EXPORTED FUNCTIONS =============================== */

/**
 * \brief Sets the register bit(s) with the given mask.
 *
 * \param periphId [in] : DLYB instance ID
 * \param regId    [in] : DLYB register ID
 * \param bitMask  [in] : Mask to apply to the register
 */
void Dlyb_Set_RegBit( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask )
{
    volatile uint32_t *regAddr = Dlyb_Reg_GetAddr( periphId, regId );

    if( DLYB_NULL_PTR != regAddr )
    {
        *regAddr |= bitMask;
    }
    else
    {
        /* Invalid instance or register ID, no action required */
    }
}


/**
 * \brief Resets the register bit(s) with the given mask.
 *
 * \param periphId [in] : DLYB instance ID
 * \param regId    [in] : DLYB register ID
 * \param bitMask  [in] : Mask to apply to the register
 */
void Dlyb_Reset_RegBit( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask )
{
    volatile uint32_t *regAddr = Dlyb_Reg_GetAddr( periphId, regId );

    if( DLYB_NULL_PTR != regAddr )
    {
        *regAddr &= ~bitMask;
    }
    else
    {
        /* Invalid instance or register ID, no action required */
    }
}


/**
 * \brief Gets the register bit(s) value with the given mask.
 *
 * \param periphId [in] : DLYB instance ID
 * \param regId    [in] : DLYB register ID
 * \param bitMask  [in] : Mask to apply to the register
 *
 * \return Register bit value with applied mask, DLYB_REG_BITS_CLEAR for an invalid parameter.
 */
uint32_t Dlyb_Get_RegBit( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask )
{
    volatile uint32_t *regAddr = Dlyb_Reg_GetAddr( periphId, regId );
    uint32_t            retValue = DLYB_REG_BITS_CLEAR;

    if( DLYB_NULL_PTR != regAddr )
    {
        retValue = ( *regAddr & bitMask );
    }
    else
    {
        /* Invalid instance or register ID, no bit is set */
        retValue = DLYB_REG_BITS_CLEAR;
    }

    return ( retValue );
}


/**
 * \brief Updates the register value with the given mask and value.
 *
 * \param periphId [in] : DLYB instance ID
 * \param regId    [in] : DLYB register ID
 * \param regMask  [in] : Mask to apply to the register
 * \param regValue [in] : Value to set in the register
 */
void Dlyb_Set_RegVal( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t regMask, uint32_t regValue )
{
    volatile uint32_t *regAddr = Dlyb_Reg_GetAddr( periphId, regId );

    if( DLYB_NULL_PTR != regAddr )
    {
        *regAddr = ( ( *regAddr & ~regMask ) | ( regValue & regMask ) );
    }
    else
    {
        /* Invalid instance or register ID, no action required */
    }
}


/**
 * \brief Gets the register value with the given mask.
 *
 * \param periphId [in] : DLYB instance ID
 * \param regId    [in] : DLYB register ID
 * \param regMask  [in] : Mask to apply to the register
 *
 * \return Register value with applied mask, DLYB_REG_BITS_CLEAR for an invalid parameter.
 */
uint32_t Dlyb_Get_RegVal( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t regMask )
{
    volatile uint32_t *regAddr = Dlyb_Reg_GetAddr( periphId, regId );
    uint32_t            retValue = DLYB_REG_BITS_CLEAR;

    if( DLYB_NULL_PTR != regAddr )
    {
        retValue = ( *regAddr & regMask );
    }
    else
    {
        /* Invalid instance or register ID, no bit is set */
        retValue = DLYB_REG_BITS_CLEAR;
    }

    return ( retValue );
}


/**
 * \brief Waits (bounded) until every bit of the mask is set in the register.
 *
 * \param periphId [in] : DLYB instance ID
 * \param regId    [in] : DLYB register ID
 * \param bitMask  [in] : Mask of the bit(s) to wait for
 *
 * \return DLYB_REQUEST_OK once every bit of the mask is set, DLYB_REQUEST_ERROR on timeout.
 */
dlyb_RequestState_t Dlyb_Reg_Wait_RegBitSet( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask )
{
    dlyb_RequestState_t retState = DLYB_REQUEST_ERROR;
    uint32_t             bitValue = DLYB_REG_BITS_CLEAR;

    for( uint32_t idx = 0u;
         ( DLYB_REG_WAIT_TIMEOUT_RAW > idx ) &&
         ( bitMask                   != bitValue ) ;
         idx++ )
    {
        bitValue = Dlyb_Get_RegBit( periphId, regId, bitMask );
    }

    if( bitMask == bitValue )
    {
        retState = DLYB_REQUEST_OK;
    }
    else
    {
        retState = DLYB_REQUEST_ERROR;
    }

    return ( retState );
}

#else /* !defined(DLYB_CR_DEN_Msk): no delay block on this device line */

void Dlyb_Set_RegBit( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask )
{
    (void)periphId;
    (void)regId;
    (void)bitMask;
}

void Dlyb_Reset_RegBit( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask )
{
    (void)periphId;
    (void)regId;
    (void)bitMask;
}

uint32_t Dlyb_Get_RegBit( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask )
{
    (void)periphId;
    (void)regId;
    (void)bitMask;

    return ( DLYB_REG_BITS_CLEAR );
}

void Dlyb_Set_RegVal( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t regMask, uint32_t regValue )
{
    (void)periphId;
    (void)regId;
    (void)regMask;
    (void)regValue;
}

uint32_t Dlyb_Get_RegVal( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t regMask )
{
    (void)periphId;
    (void)regId;
    (void)regMask;

    return ( DLYB_REG_BITS_CLEAR );
}

dlyb_RequestState_t Dlyb_Reg_Wait_RegBitSet( dlyb_PeriphId_t periphId, dlyb_RegId_t regId, uint32_t bitMask )
{
    (void)periphId;
    (void)regId;
    (void)bitMask;

    return ( DLYB_REQUEST_ERROR );
}

#endif /* defined(DLYB_CR_DEN_Msk) */

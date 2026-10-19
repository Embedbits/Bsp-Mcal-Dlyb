# DLYB Peripheral Driver

This module provides an abstraction layer for the **delay block** (`DLYB`) of STM32 MCUs: a programmable delay line that shifts
the sampling clock of a fast external interface (the Octo-SPI memory interface, the SD / eMMC host interface) by a fraction of the clock
period, so that the data are sampled in the middle of the valid window. It offers the **calibration** of the delay line to
the clock frequency, the selection of the output phase and a **tuning** procedure that finds the best phase by a test supplied by
the interface that uses it. It is a low-level MCAL module: it owns the delay block hardware, the application and the other
modules never touch the registers.

The public interface (`Dlyb_Port.h`, `Dlyb_Types.h`) is **identical for all supported families**. The implementation
for a family lives in its own branch (`Dev/<Family>` for development, `Releases/<Family>` for the released versions), so
a module or an application written against the interface does not change when the MCU family changes.

This README is the landing page and the **specification of the interface** (master). The family branches contain their
own README with everything that is specific to the family: the instances of the device, their settings and the status of
the implementation.

> **Status: design.** No implementation exists yet, the interface below is a proposal. Stories and branches follow.

---

## Intent and purpose

- At the clock rates above about 100 MHz the data coming from an external memory or a card arrive with a delay that depends
  on the board, the temperature and the voltage; the sampling edge of the interface has to be **moved into the data eye**. The delay
  block is a chain of the unit delays (tens of picoseconds each) followed by a phase selector; one setting of it (the number of
  unit delays) is a fraction of the clock period.
- The setting is **not a constant**: the length of the unit delay changes with the voltage and the temperature, so the line has to be
  calibrated for the clock frequency (how many unit delays make one period) and then the phase has to be chosen. The module
  does both and gives the interfaces a single operation: *"tune the sampling for this clock with this test"*.
- The users are the Ospi module (the double transfer rate with the data strobe, the high-frequency memory-mapped read) and the
  Sdmmc module (the SDR104 / HS200 / DDR50 modes). They call the Dlyb module; the application does not (it only enables the fast modes).
- The module hides the differences between the families: the instances (one per interface), the maximum number of the unit delays
  and phases, the input clock range and the associated peripherals differ, the interface stays the same.

What the module is **not**: it does not know the memory or the card - the test that decides whether a phase works is supplied by
the caller (a read-back of a known pattern, the tuning block of the SD card), it does not set the clock of the interface (the
RCC and the interface modules do) and it is not a general purpose delay.

---

## Concept

An instance is identified by `dlyb_PeriphId_t` (`DLYB_PERIPH_OCTOSPI1`, `DLYB_PERIPH_SDMMC1`, ... - the instances of the device, each
bound to one interface). For the instance the module keeps:

| Item | Meaning |
|---|---|
| **Input clock** | The frequency of the clock that passes the delay line (the clock of the interface; the line works from about 25 MHz up to the maximum of the interface). |
| **Unit delay count** | The number of the unit delays that make exactly one clock period (found by the calibration, `Dlyb_Set_LengthDetect`). |
| **Phase** | The selected output phase: the delay in the steps of the period divided by the number of the phases (up to 12 on the newest hardware). |
| **Tuning result** | The window of the phases that passed the test, the chosen phase (the middle of the longest passing window) and the margin. |

Rules:

1. `Dlyb_Init` enables the block of the instance. The configuration (the length detection, the phase) is changed only while the
   interface does not use the delay (the interface module disables its sampling path before the call - the Ospi and the Sdmmc
   module follow the order).
2. `Dlyb_Set_LengthDetect` runs the **length detection**: the delay line is lengthened until it is one clock period long, the module waits for
   the flag with a timeout and returns the number of the unit delays. A failure (the clock outside of the range) returns an error and
   leaves the line disabled.
3. `Dlyb_Set_Phase( phase )` writes the phase and reads it back. A phase beyond the maximum of the device is rejected.
4. `Dlyb_Set_Tuning( tuningConfig )` runs the tuning: for every phase it sets the phase, calls the **test function** of the caller
   (`dlyb_TestCallback_t`: return pass / fail), collects the result and chooses the middle of the longest passing window. If no phase
   passes, it returns an error with the empty window; the interface then reduces its clock.
5. The calibration is valid for the **clock frequency and the conditions** at the time of the detection. After a change of the
   frequency or after a large change of the temperature the procedure is repeated (`Dlyb_Set_Tuning` can be called again at any time).
6. All parameters are checked before a register is written; the module follows the TrustZone aliases of the family.

---

## Supported families

Every family gets its own story, its own tasks and its own branches `Dev/<Family>` and `Releases/<Family>` cut from this
master.

| Family | Branch | Delay blocks | Status |
|---|---|---|---|
| STM32H5 | `Dev/STM32H5` | `DLYBOS1` (OCTOSPI1), `DLYBSD1` (SDMMC1), `DLYBSD2` (SDMMC2 on STM32H562 / H563 / H573); `DLYBOS2` on the STM32H5E / H5F lines; input clock 25 MHz to the maximum of the interface, up to 12 phases | planned (first) |
| STM32U5 | `Dev/STM32U5` | `DLYB` of the OCTOSPI and the SDMMC interfaces | planned |
| STM32H7 | `Dev/STM32H7` | `DLYB` of the SDMMC and the OCTOSPI interfaces | planned |
| STM32L4 | `Dev/STM32L4` | `DLYB` of the OCTOSPI interfaces of the L4+ lines that have it | planned |
| STM32U3 | `Dev/STM32U3` | device dependent | planned |
| STM32F4, STM32F7, STM32G4 | `Dev/STM32F4` ... | no delay block on the device - the module is a stub that returns `DLYB_REQUEST_ERROR` | planned |

A function without a meaning in a family stays in the interface, returns `DLYB_REQUEST_ERROR` and suppresses the unused
parameters (for example an instance the device does not have, a phase beyond the number of the phases of the delay block).

---

## Public interface in short (proposal)

| Group | Functions |
|---|---|
| Module management | `Dlyb_Get_ModuleVersion`, `Dlyb_Init`, `Dlyb_Deinit`, `Dlyb_Task`, `Dlyb_Get_DefaultConfig` |
| Delay line | `Dlyb_Set_PeriphActive`, `Dlyb_Set_PeriphInactive`, `Dlyb_Set_LengthDetect`, `Dlyb_Get_UnitDelayCount` |
| Phase | `Dlyb_Set_Phase`, `Dlyb_Get_Phase`, `Dlyb_Get_PhaseCount` |
| Tuning | `Dlyb_Set_Tuning`, `Dlyb_Get_TuningResult` |

```c
dlyb_ModuleVersion_t Dlyb_Get_ModuleVersion     ( void );

dlyb_RequestState_t  Dlyb_Init                  ( const dlyb_Config_t * const dlybConfig );
dlyb_RequestState_t  Dlyb_Deinit                ( dlyb_PeriphId_t periphId );
void                 Dlyb_Task                  ( void );
dlyb_RequestState_t  Dlyb_Get_DefaultConfig     ( dlyb_Config_t * const dlybConfig );

dlyb_RequestState_t  Dlyb_Set_PeriphActive      ( dlyb_PeriphId_t periphId );
dlyb_RequestState_t  Dlyb_Set_PeriphInactive    ( dlyb_PeriphId_t periphId );
dlyb_RequestState_t  Dlyb_Set_LengthDetect      ( dlyb_PeriphId_t periphId, dlyb_UnitCnt_t * const unitDelayCount );

dlyb_RequestState_t  Dlyb_Set_Phase             ( dlyb_PeriphId_t periphId, dlyb_Phase_t phase );
dlyb_RequestState_t  Dlyb_Get_Phase             ( dlyb_PeriphId_t periphId, dlyb_Phase_t * const phase );
dlyb_RequestState_t  Dlyb_Get_PhaseCount        ( dlyb_PeriphId_t periphId, dlyb_Phase_t * const phaseCount );

dlyb_RequestState_t  Dlyb_Set_Tuning            ( dlyb_PeriphId_t periphId, const dlyb_TuningConfig_t * const tuningConfig, dlyb_TuningResult_t * const tuningResult );
```

Every function returns `DLYB_REQUEST_OK` / `DLYB_REQUEST_ERROR` and checks its parameters and the state of the module
before it touches a register.

| Type | Meaning |
|---|---|
| `dlyb_Config_t` | Instance, input clock frequency (Hz), the start phase |
| `dlyb_TuningConfig_t` | The test function `dlyb_TestCallback_t` and its context pointer, the first and the last phase to try, the minimum window width accepted |
| `dlyb_TestCallback_t` | `dlyb_TestResult_t ( void * context )` returning `DLYB_TEST_PASS` / `DLYB_TEST_FAIL` |
| `dlyb_TuningResult_t` | The first and the last passing phase, the chosen phase, the width of the window |
| `dlyb_UnitCnt_t`, `dlyb_Phase_t` | The number of the unit delays, the phase number |

---

## Usage

### Tuning of the Octo-SPI sampling (the Ospi module does this inside)

```c
#include "Dlyb_Port.h"

static dlyb_TestResult_t Ospi_ReadPatternTest( void * context )
{
    dlyb_TestResult_t testResult = DLYB_TEST_FAIL;

    /* read the known pattern from the memory in the fast mode and compare it */

    return testResult;
}

dlyb_Config_t       dlybConfig;
dlyb_TuningConfig_t tuningConfig;
dlyb_TuningResult_t tuningResult;

(void)Dlyb_Get_DefaultConfig( &dlybConfig );
dlybConfig.PeriphId       = DLYB_PERIPH_OCTOSPI1;
dlybConfig.InputClock_Hz  = 100000000u;
(void)Dlyb_Init( &dlybConfig );

tuningConfig.TestCallback = Ospi_ReadPatternTest;
tuningConfig.Context      = DLYB_NULL_PTR;
(void)Dlyb_Set_Tuning( DLYB_PERIPH_OCTOSPI1, &tuningConfig, &tuningResult );
/* tuningResult.ChosenPhase is applied; an empty window means: lower the clock of the interface */
```

---

## Behaviour and limits

- **Clock range.** The length detection works only for the input clock inside the range of the block; the Ospi / Sdmmc module do not call it
  for the slow modes that do not need the delay.
- **Test quality.** The tuning is only as good as the test: a test that reads a long, varied pattern (several kilobytes) is much better than a single
  word; the caller decides the cost.
- **Drift.** The phase found at one temperature may fail at another; a product at the limit repeats the tuning when the
  error counters of the interface grow.
- **Order.** The interface samples through the delay only after it has been enabled by its module; a change while it samples
  produces glitches, so the callers stop the traffic first.
- **Not part of the first version:** the continuous background retuning, the storing of the result across the reset, the
  delay of the output (transmit) path where the hardware has one.

---

## Dependencies

- **Rcc** - peripheral clock of the delay block
- **Ospi**, **Sdmmc** - the interfaces that use the delay (they call this module)
- **RAL (Register Abstraction Layer)** - used internally to access the registers

All mandatory RCC configurations are handled internally.

---

## Testing

- **Unit tests** (host, Unity + CMock + RegMem): the length detection sequence and its timeout, the phase limits per instance, the tuning with a
  simulated pass / fail pattern (one window, two windows, none, a window at the edge), the choice of the middle phase, the rejected calls.
- **Integration tests** (target, probe-rs): the tuned phase of an Octo-SPI memory and an SD card across the clock rates, the window width
  versus the temperature on the bench, the failure of the tuning at a clock that is too fast.

---

## Interface conventions

The module follows the common rules of the MCAL layer, so that it behaves like the other modules:

- **Lists instead of free members.** Everything that the hardware restricts (the pins, the kernel clock sources, the DMA
  channels, the triggers and the other selectable sources) is a `typedef enum` list with exactly the items that are valid
  for the device line; a pin is an item `DLYB_<SIGNAL>_PIN_<INSTANCE>_P<port><pin>` and there is an `*_UNUSED` item. A choice
  that is not in the list cannot be written.
- **Own peripheral only.** The module calls only the registers / LL functions of its own peripheral. The clocks, the
  interrupts, the pins, the DMA and the EXTI lines go through the API of the Rcc, Nvic, Gpio, Gpdma and Exti modules; a
  missing function is added to that module, it is not bypassed.
- **Init verifies itself.** `Dlyb_Init` checks the result at the end; when it is not as required, the module is
  deinitialized and an error is returned. The waiting for the hardware is a loop on the register state with a timeout,
  never a blind delay.
- **Transfer modes.** Where the module moves data, the POLL (served by `Dlyb_Task`), ISR and DMA modes have the same
  configuration structure, the same request and the same callbacks (error, half transfer, transfer complete).
- **Flags and states are enums of the module** (`dlyb_FunctionState_t`, `dlyb_FlagState_t`), the limits and the constants are
  named, there are no magic numbers in the interface.
- **Unsupported functions stay.** A function without a meaning in a family keeps its signature, suppresses the unused
  parameters and returns `DLYB_REQUEST_ERROR`.
- **Documentation.** Every function and type has a Doxygen description, the main header has the `\defgroup`.

---

## Repository layout and branches

The master holds this README and the license. The family branches (`Dev/<Family>`, `Releases/<Family>`) hold:

| Path | Content |
|---|---|
| `README.md` | README of the family: the instances of the device, specifics, errata, status |
| `Dlyb_Port.h`, `Dlyb_Types.h` | Public interface (identical in all families) |
| `Dlyb*.c`, `Dlyb.h` | Implementation of the family |
| `CMakeLists.txt` | Library `Dlyb_Lib` of the EmBi platform |
| `Tests/UnitTests` | Unit tests (Unity + CMock + RegMem) |
| `Tests/IntegrationTests` | Integration tests on the target (probe-rs) |

A change of the public interface is made in all families.

---

## CMake integration

```cmake
target_link_libraries( User_Lib PRIVATE Dlyb_Lib )
```

1. Link `Dlyb_Lib` to your CMake library.
2. Include `Dlyb_Port.h` in your project.
3. Configure the module as needed for your hardware.

---

## License

This project is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)**.

You are free to use, modify, and share this work for **non-commercial purposes**, provided appropriate credit is given.

See [LICENSE.md](LICENSE.md) for full terms or visit [creativecommons.org/licenses/by-nc/4.0](https://creativecommons.org/licenses/by-nc/4.0/).

---

## Authors

- **Mr.Nobody** - [embedbits.com](https://embedbits.com)

Contributions are welcome! Please open a pull request.

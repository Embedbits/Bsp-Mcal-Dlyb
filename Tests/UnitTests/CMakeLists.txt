#======================== Unit tests configuration ============================#
# Unit tests of Dlyb module. Registered in module CMakeLists.txt by
# UnitTesting_AddPath() and created by UnitTesting_Generate() in "UnitTest"
# build (see EmBi_Platform/CMake/UnitTesting/README.md).

get_filename_component(DLYB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(MCAL_DIR "${DLYB_DIR}/.." ABSOLUTE)

# The length valid flag (DLYB_CFGR.LNGF) and the length value (DLYB_CFGR.LNG) are set by the
# hardware itself after a UNIT write, which plain RegMem does not emulate: the tests that need
# the length detection to find a result activate the HW model UtDlyb_HwModel_LengthDetect (see
# the test source). Dlyb has no dependency on another MCAL module, so no mock is needed.
UnitTesting_Add_Test(
    NAME            Dlyb
    TEST_SOURCE     Test_Dlyb.c
    UUT_SOURCES     ${DLYB_DIR}/Dlyb.c
                    ${DLYB_DIR}/Dlyb_Reg.c
    INCLUDE_DIRS    ${DLYB_DIR}
    LINK_LIBS       ut_RegMem                           # Registers in host memory
                    ut_CmsisHost                         # CMSIS core intrinsics on host
    FORCE_INCLUDES  ${UNIT_TESTING_CMSIS_HOST_HEADER}    # Replaces cmsis_gcc.h
    HOST_LIBS       Ral_Lib                              # ST LL compiled for host
    COVERAGE_EXCLUDE ".*/Ral/.*"
    RUN_SERIAL
)

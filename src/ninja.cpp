#include <e32def.h>
#include <e32std.h>

GLDEF_C TInt E32Dll(TDllReason /* aReason*/)
{
    return KErrNone;
}

__declspec(naked) EXPORT_C void set_reg16(TUint16 aValue, TUint anAddr)
{
    __asm("mov\tr2, #0x58000000");
    __asm("strh\tr0, [r2, r1]");
    __asm("mov\tpc, lr");
}

__declspec(naked) EXPORT_C void set_reg32(TUint32 aValue, TUint anAddr)
{
    __asm("mov\tr2, #0x58000000");
    __asm("str\tr0, [r2, r1]");
    __asm("mov\tpc, lr");
}

EXPORT_C TInt locltest_1(void)
{
    return 0;
}

EXPORT_C TInt locltest_2(void)
{
    return 0;
}

EXPORT_C TInt locltest_3(void)
{
    return 0;
}

EXPORT_C TInt locltest_4(void)
{
    return 0;
}

EXPORT_C TInt locltest_5(void)
{
    return 0;
}

EXPORT_C TInt locltest_6(void)
{
    return 0;
}

EXPORT_C TInt locltest_7(void)
{
    return 0;
}

EXPORT_C TInt locltest_8(void)
{
    return 0;
}

EXPORT_C TInt locltest_9(void)
{
    return 0;
}

EXPORT_C TInt locltest_10(void)
{
    return 0;
}

EXPORT_C TInt locltest_11(void)
{
    return 0;
}

EXPORT_C TInt locltest_12(void)
{
    return 0;
}

EXPORT_C TInt locltest_13(void)
{
    return 0;
}

EXPORT_C TInt locltest_14(void)
{
    return 0;
}

EXPORT_C TInt locltest_15(void)
{
    return 0;
}

EXPORT_C TInt locltest_16(void)
{
    set_reg16(0x2bf0, 0x149000);
    set_reg32(0x180f8000, 0x120018);
    set_reg32(0x8028000, 0x120014);

    return 0;
}

EXPORT_C TInt locltest_17(void)
{
    return 0;
}

EXPORT_C TInt locltest_18(void)
{
    return 0;
}

EXPORT_C TInt locltest_19(void)
{
    return 0;
}

EXPORT_C TInt locltest_20(void)
{
    return 0;
}

EXPORT_C TInt locltest_21(void)
{
    return 0;
}

/*
* fixed12 range of (-2047 to 2047)[12bits] and precision of (0.0625 - 0.9375)[4bits]
* v5
* lnk: https://bogaardryan.com
* src: https://github.com/KuroPSPiso/?
*/

#define DoubleToFixed(x) (x*(double)(1<<4))
#define FixedToDouble(x) ((double)x/ (double)(1<<4))

#include <iostream>
#include <stdint.h>

#define UINT8 uint8_t
#define UINT16 uint16_t

#define INT16 signed short
#define INT8 signed char
#define BOOL unsigned char
#define TRUE 1
#define FALSE 0

//constants do not copy in GBDK
const int intMultiplier = 10000; //update along with lowbits

//constants (32bits)
const int signMaskInt32 = 0x80000000;

//constants (32bits)
typedef INT16 fixed16;
const short fullBits = 16;
const char lowBits = 4; //update to change calculations
const char highBits = fullBits - lowBits;
const fixed16 fullMaskF16 = 0xFFFF;
const fixed16 fractionMaskF16 = 0xFFFF >> highBits;
const fixed16 signMaskF16 = 0x8000;
const fixed16 baseMaskF16 = (-1 ^ fractionMaskF16); //includes sign

//define macros
#define SIGNISSET(x) (x < 0)
#define SIGNINT32(x) (x & signMaskInt32)
#define SIGNF16(x) (x & signMaskF16)
#define BASEF16(x) (x & baseMaskF16)
#define FRACTIONF16(x) (x & fractionMaskF16)
#define PI 0x0032

//define macros do not copy in GBDK
#define BASEF16TOINT(x) ((BASEF16(x) >> lowBits) * intMultiplier) 

//int16 operators (TODO: works on fixed16 too)
INT16 int16Power(INT16 base, UINT8 exp)
{
    if (exp == 0) return 1;
    if (exp == 1) return base;
    INT16 ret = base;
    for (; exp > 1; exp--)
    {
        ret *= base;
    }
    return ret;
}

//fixed16 operators
fixed16 absF16(fixed16 x)
{
    if (!SIGNISSET(x)) return x;
    return 0 - x;
}

fixed16 addF16(fixed16 x, fixed16 add)
{
    return x + add;
}

fixed16 subF16(fixed16 x, fixed16 sub)
{
    return x - sub;
}

fixed16 mulF16(fixed16 x, fixed16 mul)
{
    if (mul == 0 || x == 0) return 0;
    if ((x & fractionMaskF16) == 0 && (mul & fractionMaskF16) == 0) return ((x >> lowBits) * (mul >> lowBits)) << lowBits; //16 bit multiplication of base numbers only (cheapest)
    //int32 calc (fixed16*fixed16) (expensive)
    signed int res = ((signed int) x) * ((signed int) mul);
    //rounding
    
    if ((res & 0xF) >= ((fractionMaskF16/2) + 0x01))
    {
        res &= 0xFFFFFFF0;
        res += 0x10;
    }
    return (res >> 4) & fullMaskF16; //bit shift and AND 32bit to 16 bit (auto floors)
}

fixed16 divF16(fixed16 x, fixed16 div)
{
    if (div == 0 || x == 0) return x; //undefined but since result and errors can't be handled return incomming value. TODO: check for setting flag val;
    return ((x << (lowBits - 1)) / (div>>1)); //additional lost precision on div, to improve possible precision after calculation based on initial value and range of original value.
}

fixed16 floorF16(fixed16 x)
{
    return (x & (fullMaskF16 - fractionMaskF16));
}

fixed16 ceilF16(fixed16 x)
{
    if (FRACTIONF16(x) <= 0) return x;

    return (x & (fullMaskF16 - fractionMaskF16)) + (0x0001 << lowBits);
}

//do not copy in GBDK (execute calculations here first)
int f16ToInt(fixed16 x)
{
    if (x == 0) return 0;
    int ret;
    UINT8 frac = x & fractionMaskF16;
    //base calculation
    ret = BASEF16TOINT(x);

    //fraction calculation
    INT16 fracValue;
    char loopLowBits;

    for (loopLowBits = 1; loopLowBits <= lowBits; loopLowBits++)
    {
        fracValue = 0;
        if (((frac >> (lowBits - loopLowBits)) & 0x01) > 0) fracValue = intMultiplier / int16Power(2, loopLowBits);
        ret += fracValue;
    }

    return ret;
}

fixed16 intToF16(int x)
{
    if (x == 0) return 0x0000;

    BOOL sign = SIGNISSET(x); //is negative?
    
    //base calculation
    INT16 base = 0;
    base = ((x / intMultiplier) << lowBits);

    //fraction calculation
    INT16 fracValue = 0;
    UINT8 frac = 0;
    fracValue = (x - ((base >> lowBits) * intMultiplier));

    INT16 lowBitMultiplier;
    char loopLowBits;

    if (sign)
    {
        for (loopLowBits = 1; loopLowBits <= lowBits && fracValue < 0; loopLowBits++)
        {
            lowBitMultiplier = intMultiplier / int16Power(2, loopLowBits);
            if (fracValue + lowBitMultiplier <= 0)
            { 
                fracValue += lowBitMultiplier;
                frac |= (0x01 << (lowBits - loopLowBits));
            }
        }

        return base - (frac & fractionMaskF16);
    }
    else
    {
        for (loopLowBits = 1; loopLowBits <= lowBits && fracValue > 0; loopLowBits++)
        {
            lowBitMultiplier = intMultiplier / int16Power(2, loopLowBits);
            if (fracValue - lowBitMultiplier >= 0) 
            { 
                fracValue -= lowBitMultiplier;
                frac |= (0x01 << (lowBits - loopLowBits));
            }
        }

        return base + (frac & fractionMaskF16);
    }
}

int main()
{
    fixed16 f1;

    f1 = DoubleToFixed(00000);
    printf("+0.0000 = %04X = %lf\n", f1 & 0xFFFF, FixedToDouble(f1));
    f1 = DoubleToFixed(-0.5);
    printf("-0.5000 = %04X = %lf\n", f1 & 0xFFFF, FixedToDouble(f1));
    f1 = DoubleToFixed(-1.5);
    printf("-1.5000 = %04X = %lf\n", f1 & 0xFFFF, FixedToDouble(f1));
    f1 = DoubleToFixed(-1.565);
    printf("-1.5650 = %04X = %lf\n", f1 & 0xFFFF, FixedToDouble(f1));
    f1 = DoubleToFixed(0.5);
    printf("+0.5000 = %04X = %lf\n", f1 & 0xFFFF, FixedToDouble(f1));
    f1 = DoubleToFixed(2.565);
    printf("+2.5650 = %04X = %lf\n", f1 & 0xFFFF, FixedToDouble(f1));
    f1 = DoubleToFixed(-3.8476 * 4.1976);
    printf("-3.8476 * 4.1976 = %04X = %lf\n", f1 & 0xFFFF, FixedToDouble(f1));

    //SET TEST
    printf("-----------[SET]-----------\n");
    f1 = intToF16(00000);   //0.0000
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1) & 0xFFFF, FRACTIONF16(f1));
    f1 = intToF16(-5000);           //-0.5
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1) & 0xFFFF, FRACTIONF16(f1));
    f1 = intToF16(-15000);          //-1.5
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1) & 0xFFFF, FRACTIONF16(f1));
    f1 = intToF16(-15650);          //-1.5
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1) & 0xFFFF, FRACTIONF16(f1));
    f1 = intToF16(5000);            //0.5
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1) & 0xFFFF, FRACTIONF16(f1));
    f1 = intToF16(25650);           //2.5625
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1) & 0xFFFF, FRACTIONF16(f1));

    //abs
    printf("-----------[ABS]-----------\n");
    f1 = absF16(intToF16(5000)); //0.5 -> 0.5
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    f1 = absF16(intToF16(-5000)); //-0.5 -> 0.5
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    f1 = absF16(intToF16(-49999)); //-4.9375 -> 4.9375
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));

    //add
    printf("-----------[ADD]-----------\n");
    int a1, a2;
    a1 = 15000; //1.5
    a2 = 28938; //2.875
    f1 = addF16(intToF16(a1), intToF16(a2)); //4.375
    printf("%d + %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    //add minus
    a1 = 7500;
    a2 = -7500;
    f1 = addF16(intToF16(a1), intToF16(a2));
    printf("%d + %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    //add minus2
    a1 = -7500;
    a2 = -7500;
    f1 = addF16(intToF16(a1), intToF16(a2));
    printf("%d + %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));

    //sub 
    printf("-----------[SUB]-----------\n");
    a1 = 15000; //1.5
    a2 = 28938; //2.875
    f1 = subF16(intToF16(a1), intToF16(a2)); //4.375
    printf("%d - %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    //sub minus
    a1 = 7500;
    a2 = -7500;
    f1 = subF16(intToF16(a1), intToF16(a2));
    printf("%d - %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));

    //mul 
    printf("-----------[MUL]-----------\n");
    a1 = 10000;
    a2 = 20000;
    f1 = mulF16(intToF16(a1), intToF16(a2));
    printf("%d * %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    a1 = 5000;
    a2 = 20000;
    f1 = mulF16(intToF16(a1), intToF16(a2));
    printf("%d * %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    a1 = 20000;
    a2 = -30000;
    f1 = mulF16(intToF16(a1), intToF16(a2));
    printf("%d * %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    a1 = 10000;
    a2 = 5000;
    f1 = mulF16(intToF16(a1), intToF16(a2));
    printf("%d * %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    a1 = -38476;
    a2 = 41976;
    f1 = mulF16(intToF16(a1), intToF16(a2));
    printf("%d * %d\n", a1, a2);
    printf("%x * %x\n", intToF16(a1), intToF16(a2));
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));


    //div 
    printf("-----------[DIV]-----------\n");
    a1 = 10000;
    a2 = 20000;
    f1 = divF16(intToF16(a1), intToF16(a2));
    printf("%d / %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    a1 = 10000;
    a2 = 0;
    f1 = divF16(intToF16(a1), intToF16(a2));
    printf("%d / %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    a1 = 984948;
    a2 = 543515;
    f1 = divF16(intToF16(a1), intToF16(a2));
    printf("%d / %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    a1 = -984948;
    a2 = 543515;
    f1 = divF16(intToF16(a1), intToF16(a2));
    printf("%d / %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    a1 = 984948;
    a2 = -543515;
    f1 = divF16(intToF16(a1), intToF16(a2));
    printf("%d / %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));
    a1 = -984948;
    a2 = -543515;
    f1 = divF16(intToF16(a1), intToF16(a2));
    printf("%d / %d\n", a1, a2);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));

    //pi
    a1 = 31416;
    f1 = intToF16(a1);
    printf("pi = %d\n", a1);
    printf("I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f1), f1 & 0xFFFF, SIGNISSET(f1), BASEF16(f1), FRACTIONF16(f1));

    fixed16 f2;
    f2 = floorF16(f1);
    printf("FLOOR : I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f2), f2 & 0xFFFF, SIGNISSET(f2), BASEF16(f2), FRACTIONF16(f2));
    f2 = ceilF16(f1);
    printf("CEIL : I32Conv: %12d, F16: %04X, sign: %1X, base: %4X, frac: %1X\n\n", f16ToInt(f2), f2 & 0xFFFF, SIGNISSET(f2), BASEF16(f2), FRACTIONF16(f2));


    return 0;
}
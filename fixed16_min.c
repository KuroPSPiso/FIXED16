#define FULLBITS 16
#define LOWBITS 4
#define HIGHBITS (FULLBITS - LOWBITS)
#define SIGNMASKINT32 0x80000000
#define FULLMASKF16 0xFFFF
#define SIGNMASKF16 0x8000

typedef INT16 fixed16;
const fixed16 fractionMaskF16 = (0xFFFF >> HIGHBITS);
const fixed16 baseMaskF16 = (-1 ^ (0xFFFF >> HIGHBITS)); //includes sign

#define SIGNISSET(x) (x < 0)
#define SIGNINT32(x) (x & SIGNMASKINT32)
#define SIGNF16(x) (x & SIGNMASKF16)
#define BASEF16(x) (x & baseMaskF16)
#define FRACTIONF16(x) (x & fractionMaskF16)
#define PI 0x0032
#define INT16TOF16(x) (x << LOWBITS) //quick standard int16 to fixed16, whole numbers
#define F16TOINT16(x) (x >> LOWBITS) //quick standard int16 to fixed16, whole numbers

//fixed16 operators
fixed16 absF16(fixed16 x)
{
    if (!SIGNISSET(x)) return x;
    return 0 - x;
}

fixed16 mulF16(fixed16 x, fixed16 mul)
{
    if (mul == 0 || x == 0) return 0;
    if ((x & fractionMaskF16) == 0 && (mul & fractionMaskF16) == 0) return ((x >> LOWBITS) * (mul >> LOWBITS)) << LOWBITS;
    signed int res = ((signed int)x) * ((signed int)mul);
    if ((res & 0xF) >= ((fractionMaskF16 / 2) + 0x01))
    {
        res &= 0xFFFFFFF0;
        res += 0x10;
    }
    return (res >> 4) & FULLMASKF16;
}

fixed16 divF16(fixed16 x, fixed16 div)
{
    if (div == 0 || x == 0) return x;
    return ((x << (LOWBITS - 1)) / (div >> 1));
}

fixed16 floorF16(fixed16 x)
{
    return (x & (FULLMASKF16 - fractionMaskF16));
}

fixed16 ceilF16(fixed16 x)
{
    if (FRACTIONF16(x) <= 0) return x;

    return (x & (FULLMASKF16 - fractionMaskF16)) + (0x0001 << LOWBITS);
}

fixed16 sinF16(fixed16 x) {
    fixed16 res = 0;
    fixed16 term = x;
    int k = 1;

    while (addF16(res, term) != res) {
        res += term;
        k += 2;
        term = mulF16(term, -x);
        term = mulF16(term, x);
        term = divF16(term, INT16TOF16(k));
        term = divF16(term, INT16TOF16(k - 1));
    }

    return res;
}

fixed16 cosF16(fixed16 x) {
    fixed16 res = 0;
    fixed16 term = 0x10;
    int k = 0;

    while (addF16(res, term) != res) {
        res += term;
        k += 2;
        term = mulF16(term, -0x10);
        term = mulF16(term, x);
        term = mulF16(term, x);
        term = divF16(term, INT16TOF16(k));
        term = divF16(term, INT16TOF16(k - 1));
    }

    return res;
}
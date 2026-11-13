#include <cstdlib>

#include "vlong.hh"

// 0x080eada0  vlong.c:47
vlong_montgomery::vlong_montgomery(const vlong & /*M*/) { abort(); /* STUB */ }

// 0x080eb5a0  vlong.c:56
void vlong_montgomery::mul(vlong & /*x*/, const vlong & /*y*/) { abort(); /* STUB */ }

// 0x080eb790  vlong.c:73
vlong vlong_montgomery::exp(const vlong & /*x*/, const vlong & /*e*/) { abort(); /* STUB */ }

// 0x080ebe20  vlong.c:95
vlong_flex_unit::vlong_flex_unit() { abort(); /* STUB */ }

// 0x080ebe80  vlong.c:103
vlong_flex_unit::~vlong_flex_unit() { abort(); /* STUB */ }

// 0x080ebec0  vlong.c:111
unsigned int vlong_flex_unit::get(unsigned int /*i*/) const { abort(); /* STUB */ }

// 0x080ebee0  vlong.c:118
void vlong_flex_unit::clear() { abort(); /* STUB */ }

// 0x080ebef0  vlong.c:124
void vlong_flex_unit::reserve(unsigned int /*x*/) { abort(); /* STUB */ }

// 0x080ebf60  vlong.c:139
void vlong_flex_unit::set(unsigned int /*i*/, unsigned int /*x*/) { abort(); /* STUB */ }

// 0x080ec050  vlong.c:156
void vlong_flex_unit::fast_mul(vlong_flex_unit & /*x*/, vlong_flex_unit & /*y*/, unsigned int /*keep*/) { abort(); /* STUB */ }

// 0x080ec290  vlong.c:203
vlong_value::operator unsigned int() { abort(); /* STUB */ }

// 0x080ec2b0  vlong.c:209
int vlong_value::is_zero() const { abort(); /* STUB */ }

// 0x080ec2d0  vlong.c:215
int vlong_value::test(unsigned int /*i*/) const { abort(); /* STUB */ }

// 0x080ec300  vlong.c:221
unsigned int vlong_value::bits() const { abort(); /* STUB */ }

// 0x080ec350  vlong.c:229
int vlong_value::cf(vlong_value & /*x*/) const { abort(); /* STUB */ }

// 0x080ec3e0  vlong.c:244
void vlong_value::shl() { abort(); /* STUB */ }

// 0x080ec440  vlong.c:257
void vlong_value::shr() { abort(); /* STUB */ }

// 0x080ec4a0  vlong.c:271
void vlong_value::shr(unsigned int /*x*/) { abort(); /* STUB */ }

// 0x080ec540  vlong.c:287
void vlong_value::add(vlong_value & /*x*/) { abort(); /* STUB */ }

// 0x080ec630  vlong.c:302
void vlong_value::subtract(vlong_value & /*x*/) { abort(); /* STUB */ }

// 0x080ec6b0  vlong.c:321
void vlong_value::init(unsigned int /*x*/) { abort(); /* STUB */ }

// 0x080ec6e0  vlong.c:328
void vlong_value::copy(vlong_value & /*x*/) { abort(); /* STUB */ }

// 0x080ec760  vlong.c:336
vlong_value::vlong_value() { abort(); /* STUB */ }

// 0x080ec790  vlong.c:342
void vlong_value::mul(vlong_value & /*x*/, vlong_value & /*y*/) { abort(); /* STUB */ }

// 0x080ec7e0  vlong.c:348
void vlong_value::divide(vlong_value & /*x*/, vlong_value & /*y*/, vlong_value & /*rem*/) { abort(); /* STUB */ }

// 0x080ecaa0  vlong.c:378
vlong::vlong(unsigned int /*x*/) { abort(); /* STUB */ }

// 0x080ecb20  vlong.c:389
vlong::vlong(const vlong & /*x*/) { abort(); /* STUB */ }

// 0x080ecbb0  vlong.c:397
vlong::~vlong() { abort(); /* STUB */ }

// 0x080ecc90  vlong.c:403
void vlong::docopy() { abort(); /* STUB */ }

// 0x080eccf0  vlong.c:416
int vlong::cf(const vlong /*x*/) const { abort(); /* STUB */ }

// 0x080ecd80  vlong.c:426
vlong &vlong::operator=(const vlong & /*x*/) { abort(); /* STUB */ }

// 0x080ece00  vlong.c:436
vlong::operator unsigned int() { abort(); /* STUB */ }

// 0x080ece20  vlong.c:442
vlong &vlong::operator+=(const vlong & /*x*/) { abort(); /* STUB */ }

// 0x080ecf60  vlong.c:464
vlong &vlong::operator-=(const vlong & /*x*/) { abort(); /* STUB */ }

// 0x080ed0a0  vlong.c:487
vlong operator+(const vlong & /*x*/, const vlong & /*y*/) { abort(); /* STUB */ }

// 0x080ed100  vlong.c:495
vlong operator-(const vlong & /*x*/, const vlong & /*y*/) { abort(); /* STUB */ }

// 0x080ed160  vlong.c:503
vlong operator*(const vlong & /*x*/, const vlong & /*y*/) { abort(); /* STUB */ }

// 0x080ed230  vlong.c:512
vlong operator/(const vlong & /*x*/, const vlong & /*y*/) { abort(); /* STUB */ }

// 0x080ed340  vlong.c:522
vlong operator%(const vlong & /*x*/, const vlong & /*y*/) { abort(); /* STUB */ }

// 0x080ed440  vlong.c:532
vlong vlong::modexp(const vlong & /*x*/, const vlong & /*e*/, const vlong & /*m*/) { abort(); /* STUB */ }

// 0x080ed510  vlong.c:539
vlong vlong::gcd(const vlong & /*X*/, const vlong & /*Y*/) { abort(); /* STUB */ }

// 0x080ed6f0  vlong.c:552
vlong vlong::modinv(const vlong & /*a*/, const vlong & /*m*/) { abort(); /* STUB */ }

// 0x080edb90  vlong.c:571
void vlong::convert(const char * /*pDecimal*/, vlong & /*Number*/) { abort(); /* STUB */ }

// 0x080eddf0  vlong.c:590
void vlong::convert(vlong /*Number*/, char * /*pDecimal*/, unsigned short /*DecimalLength*/) { abort(); /* STUB */ }

// 0x080ee120  vlong.c:623
void vlong::convert(const unsigned char * /*pData*/, unsigned short /*DataLength*/, vlong & /*Number*/) { abort(); /* STUB */ }

// 0x080ee200  vlong.c:675
void vlong::convert(vlong /*Number*/, unsigned char * /*pData*/, unsigned short /*DataLength*/) { abort(); /* STUB */ }

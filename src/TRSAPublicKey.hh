// TRSAPublicKey.hh: the length of an RSA block in bytes and the public
// exponent.

#ifndef PATELLA_TRSAPUBLICKEY_HH
#define PATELLA_TRSAPUBLICKEY_HH

#include "vlong.hh"

const unsigned short RSA_PUBLICKEY_LENGTH = 128;

static const vlong RSA_EXPONENT(65537);

#endif

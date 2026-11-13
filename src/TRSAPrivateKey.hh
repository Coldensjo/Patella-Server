#ifndef PATELLA_TRSAPRIVATEKEY_HH
#define PATELLA_TRSAPRIVATEKEY_HH

#include "vlong.hh"

struct TRSAPrivateKey {
	TRSAPrivateKey();
	virtual ~TRSAPrivateKey();
	void init(const char *PrimeP, const char *PrimeQ);
	void decrypt(unsigned char *Data) const;
private:
	vlong m_PrimeP;
	vlong m_PrimeQ;
	vlong m_U;
	vlong m_DP;
	vlong m_DQ;
};

#endif

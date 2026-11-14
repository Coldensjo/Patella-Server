#ifndef PATELLA_TXTEASYMMETRICKEY_HH
#define PATELLA_TXTEASYMMETRICKEY_HH

#include <cstdint>

const unsigned short XTEA_KEY_LENGTH = 16;
const unsigned short XTEA_BLOCK_LENGTH = 8;

struct TXTEASymmetricKey {
	TXTEASymmetricKey();
	virtual ~TXTEASymmetricKey();
	void init(const unsigned char *SymmetricKey);
	bool isInitialized() const;
	void encrypt(unsigned char *Data) const;
	void decrypt(unsigned char *Data) const;
private:
	static void encrypt(uint32_t *v, uint32_t *k);
	static void decrypt(uint32_t *v, uint32_t *k);
	unsigned char m_SymmetricKey[XTEA_KEY_LENGTH];
};

#endif

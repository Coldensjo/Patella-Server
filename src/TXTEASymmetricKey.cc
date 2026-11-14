#include <cstdint>
#include <cstring>

#include "TXTEASymmetricKey.hh"

TXTEASymmetricKey::TXTEASymmetricKey() {
	memset(m_SymmetricKey, 0, XTEA_KEY_LENGTH);
}

TXTEASymmetricKey::~TXTEASymmetricKey() {
}

void TXTEASymmetricKey::init(const unsigned char *SymmetricKey) {
	if (SymmetricKey != nullptr) {
		memcpy(m_SymmetricKey, SymmetricKey, XTEA_KEY_LENGTH);
	}
}

void TXTEASymmetricKey::encrypt(unsigned char *Data) const {
	if (Data != nullptr) {
		encrypt((uint32_t *)Data, (uint32_t *)m_SymmetricKey);
	}
}

void TXTEASymmetricKey::decrypt(unsigned char *Data) const {
	if (Data != nullptr) {
		decrypt((uint32_t *)Data, (uint32_t *)m_SymmetricKey);
	}
}

void TXTEASymmetricKey::encrypt(uint32_t *v, uint32_t *k) {
	uint32_t v0 = v[0], v1 = v[1], i;
	uint32_t sum = 0, delta = 0x9E3779B9;

	for (i = 0; i < 32; i++) {
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
		sum += delta;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
	}

	v[0] = v0;
	v[1] = v1;
}

void TXTEASymmetricKey::decrypt(uint32_t *v, uint32_t *k) {
	uint32_t v0 = v[0], v1 = v[1], i;
	uint32_t sum = 0xC6EF3720, delta = 0x9E3779B9;

	for (i = 0; i < 32; i++) {
		v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
		sum -= delta;
		v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
	}

	v[0] = v0;
	v[1] = v1;
}

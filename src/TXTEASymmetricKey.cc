#include <cstdint>
#include <cstring>

#include "TXTEASymmetricKey.hh"

// 0x080ea270  TXTEASymmetricKey.c:32
TXTEASymmetricKey::TXTEASymmetricKey() {
	memset(m_SymmetricKey, 0, XTEA_KEY_LENGTH);
}

// 0x080ea2b0  TXTEASymmetricKey.c:39
TXTEASymmetricKey::~TXTEASymmetricKey() {
}

// 0x080ea2e0  TXTEASymmetricKey.c:45
void TXTEASymmetricKey::init(const unsigned char *SymmetricKey) {
	if (SymmetricKey != nullptr) {
		memcpy(m_SymmetricKey, SymmetricKey, XTEA_KEY_LENGTH);
	}
}

// 0x080ea310  TXTEASymmetricKey.c:55
bool TXTEASymmetricKey::isInitialized() const {
	for (int i = 0; i < XTEA_KEY_LENGTH; i++) {
		if (m_SymmetricKey[i] != 0) {
			return true;
		}
	}

	return false;
}

// 0x080ea340  TXTEASymmetricKey.c:64
void TXTEASymmetricKey::encrypt(unsigned char *Data) const {
	if (Data != nullptr) {
		encrypt((uint32_t *)Data, (uint32_t *)m_SymmetricKey);
	}
}

// 0x080ea3e0  TXTEASymmetricKey.c:91
void TXTEASymmetricKey::decrypt(unsigned char *Data) const {
	if (Data != nullptr) {
		decrypt((uint32_t *)Data, (uint32_t *)m_SymmetricKey);
	}
}

// 0x080ea480  TXTEASymmetricKey.c:123
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

// 0x080ea510  TXTEASymmetricKey.c:137
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

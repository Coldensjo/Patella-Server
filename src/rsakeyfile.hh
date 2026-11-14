// rsakeyfile.hh: the server's RSA private key as a PEM file (rsakeyfile.cc),
// read by the game server and the login server, written by rsakeygen.
//
// A key file holds an unencrypted RSA private key, either PKCS#1
// ("-----BEGIN RSA PRIVATE KEY-----") or PKCS#8 ("-----BEGIN PRIVATE
// KEY-----", algorithm rsaEncryption), the formats OpenSSL writes. The 7.x
// client encrypts its login block with the public exponent 65537 for a
// 1024-bit modulus (128-byte blocks), so a key must have exactly those.
// TRSAPrivateKey::init takes the two primes as decimal texts.

#ifndef PATELLA_RSAKEYFILE_HH
#define PATELLA_RSAKEYFILE_HH

#include <string>
#include <vector>

// The bits of the modulus and the public exponent the client needs.
const int RSA_KEY_BITS = 1024;
const unsigned int RSA_KEY_EXPONENT = 65537;

// A key read from a file: its primes and modulus as decimal texts. PrimeP is
// the smaller prime: TRSAPrivateKey's decryption (by the Chinese remainder
// theorem) adds PrimeQ once to a difference that is only sure to become
// positive when PrimeQ is the larger one.
struct TRSAKeyFile {
	std::string PrimeP;
	std::string PrimeQ;
	std::string Modulus;
};

// Reads and checks the key file FileName. Throws std::runtime_error with a
// one-line text "<FileName>: <what is wrong>" when the file cannot be read,
// is not an unencrypted RSA private key in PEM, or the key is not one the
// client can use (modulus of other than 1024 bits, exponent other than
// 65537, modulus not the product of the primes, a factor that is not an odd
// prime). The primality test reads /dev/urandom. The texts never hold a
// prime.
TRSAKeyFile ReadRSAKeyFile(const std::string &FileName);

// The same for the text of a key file; Name stands for the file in errors.
TRSAKeyFile ParseRSAKeyPEM(const std::string &Text, const std::string &Name);

// The key of the primes PrimeP and PrimeQ (decimal texts, in either order) as
// PKCS#1 DER: the modulus, the exponent 65537, the private exponent, the
// primes in the order given, the two exponents of the primes and the
// coefficient (the inverse of PrimeQ modulo PrimeP). Throws
// std::runtime_error when the primes do not make a key the client can use.
std::vector<unsigned char> MakeRSAKeyDER(const std::string &PrimeP, const std::string &PrimeQ);

// The same as a PEM text ("-----BEGIN RSA PRIVATE KEY-----").
std::string MakeRSAKeyPEM(const std::string &PrimeP, const std::string &PrimeQ);

// DER as a PEM text with the label Label: base64 in lines of 64 characters
// between the BEGIN and END lines, each line ended by '\n'.
std::string PEMEncode(const std::string &Label, const std::vector<unsigned char> &DER);

// Writes Text into the new file FileName with mode 0600. Throws
// std::runtime_error when the file exists (it is never overwritten) or
// cannot be written.
void WriteRSAKeyFile(const std::string &FileName, const std::string &Text);

// Reads RSA_PRIME_P and RSA_PRIME_Q (decimal texts in string literals) from
// the text of a header in the format of the old RSA_PrivateKey.hh. Throws
// std::runtime_error "<Name>: <what>" when one is missing.
void ParseRSAKeyHeader(const std::string &Text, const std::string &Name, std::string &PrimeP,
		std::string &PrimeQ);

// Two new random 512-bit primes, different, each with its two top bits set
// (so that their product has 1024 bits) and with 65537 not dividing the
// prime less one, as decimal texts. Reads /dev/urandom; throws
// std::runtime_error when it cannot.
void GenerateRSAPrimes(std::string &PrimeP, std::string &PrimeQ);

#endif

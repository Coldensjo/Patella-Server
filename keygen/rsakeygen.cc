// rsakeygen: makes and reads the key file (key.pem) of the game server and
// the login server (src/rsakeyfile.hh): an RSA private key of 1024 bits with
// the exponent 65537, in PKCS#1 PEM. Clients need its modulus, which every
// command prints as RsaModulus=...; the primes are never printed.
//
// usage:
//   rsakeygen FILE
//       writes a new key with two random 512-bit primes into FILE
//   rsakeygen --modulus FILE
//       checks the key file FILE (PKCS#1 or PKCS#8) and prints its modulus
//   rsakeygen --from-header HEADER FILE
//       writes the key of a header in the old format (RSA_PRIME_P and
//       RSA_PRIME_Q as decimal texts, as in the original RSA_PrivateKey.hh)
//       into FILE
// FILE is created with mode 0600 and never overwritten.

#include "rsakeyfile.hh"

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

void usage(const char *Program) {
	fprintf(stderr,
		"usage: %s FILE                       write a new key into FILE\n"
		"       %s --modulus FILE             print the modulus of the key file FILE\n"
		"       %s --from-header HEADER FILE  write the key of an old RSA_PrivateKey.hh into FILE\n"
		"FILE is a PEM file, written with mode 0600 and never overwritten. Each\n"
		"command prints the RsaModulus= clients need.\n",
		Program, Program, Program);
}

std::string readHeader(const char *FileName) {
	FILE *File = fopen(FileName, "rb");
	if (File == nullptr) {
		throw std::runtime_error(std::string(FileName) + ": cannot open the header");
	}
	std::string Text;
	char Buffer[4096];
	size_t Count;
	while ((Count = fread(Buffer, 1, sizeof(Buffer), File)) > 0) {
		Text.append(Buffer, Count);
	}
	bool Failed = ferror(File) != 0;
	fclose(File);
	if (Failed) {
		throw std::runtime_error(std::string(FileName) + ": cannot read the header");
	}
	return Text;
}

// Writes the key of the primes into FileName and prints its modulus, as read
// back from the file.
void writeKey(const std::string &PrimeP, const std::string &PrimeQ, const char *FileName, const char *What) {
	WriteRSAKeyFile(FileName, MakeRSAKeyPEM(PrimeP, PrimeQ));
	TRSAKeyFile Key = ReadRSAKeyFile(FileName);
	printf("rsakeygen: %s in %s\nRsaModulus=%s\n", What, FileName, Key.Modulus.c_str());
}

} // namespace

int main(int argc, char **argv) {
	try {
		if (argc == 2 && argv[1][0] != '-') {
			std::string PrimeP;
			std::string PrimeQ;
			GenerateRSAPrimes(PrimeP, PrimeQ);
			writeKey(PrimeP, PrimeQ, argv[1], "new RSA key");
		} else if (argc == 3 && strcmp(argv[1], "--modulus") == 0) {
			TRSAKeyFile Key = ReadRSAKeyFile(argv[2]);
			printf("RsaModulus=%s\n", Key.Modulus.c_str());
		} else if (argc == 4 && strcmp(argv[1], "--from-header") == 0) {
			std::string PrimeP;
			std::string PrimeQ;
			ParseRSAKeyHeader(readHeader(argv[2]), argv[2], PrimeP, PrimeQ);
			writeKey(PrimeP, PrimeQ, argv[3], (std::string("the RSA key of ") + argv[2]).c_str());
		} else {
			usage(argv[0]);
			return argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) ? 0 : 2;
		}
	} catch (const std::exception &e) {
		fprintf(stderr, "rsakeygen: %s\n", e.what());
		return 1;
	}
	return 0;
}

// test_rsakeyfile.cc: the key files of src/rsakeyfile.hh. A key made when the
// test runs goes through the writer and the reader, as PKCS#1 and wrapped as
// PKCS#8; then keys built here field by field, and files broken on purpose,
// must be refused with the right text, and no text may hold a prime.

#include "rsakeyfile.hh"
#include "test.hh"
#include "vlong.hh"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

typedef std::vector<unsigned char> Bytes;

// ---------------------------------------------------------------- numbers, through vlong

Bytes strip(const Bytes &number) {
	size_t first = 0;
	while (first < number.size() && number[first] == 0) {
		first++;
	}
	return Bytes(number.begin() + static_cast<std::ptrdiff_t>(first), number.end());
}

vlong toVlong(const Bytes &number) {
	Bytes padded((4 - number.size() % 4) % 4, 0);
	padded.insert(padded.end(), number.begin(), number.end());
	vlong result;
	if (!padded.empty()) {
		vlong::convert(padded.data(), static_cast<unsigned short>(padded.size()), result);
	}
	return result;
}

Bytes toBytes(const vlong &number) {
	unsigned char buffer[264];
	vlong::convert(number, buffer, sizeof(buffer));
	return strip(Bytes(buffer, buffer + sizeof(buffer)));
}

Bytes fromDecimal(const std::string &text) {
	vlong number;
	vlong::convert(text.c_str(), number);
	return toBytes(number);
}

std::string toDecimal(vlong number) {
	std::string digits;
	while (number > 0) {
		digits.insert(digits.begin(), static_cast<char>('0' + static_cast<unsigned int>(number % 10)));
		number = number / 10;
	}
	return digits.empty() ? "0" : digits;
}

bool equal(const vlong &a, const vlong &b) {
	return !(a < b) && !(b < a);
}

// The first odd number from Start up that no number below 2000 but 1 divides.
vlong noSmallFactor(const Bytes &start) {
	vlong number = toVlong(start);
	if (number % 2 == 0) {
		number = number + 1;
	}
	for (;;) {
		bool found = true;
		for (int k = 3; k < 2000 && found; k += 2) {
			found = number % k != 0;
		}
		if (found) {
			return number;
		}
		number = number + 2;
	}
}

// ---------------------------------------------------------------- DER, built here

Bytes cat(std::initializer_list<Bytes> parts) {
	Bytes out;
	for (const Bytes &part : parts) {
		out.insert(out.end(), part.begin(), part.end());
	}
	return out;
}

Bytes element(unsigned char tag, const Bytes &content) {
	Bytes out{tag};
	if (content.size() < 0x80) {
		out.push_back(static_cast<unsigned char>(content.size()));
	} else if (content.size() < 0x100) {
		out.push_back(0x81);
		out.push_back(static_cast<unsigned char>(content.size()));
	} else {
		out.push_back(0x82);
		out.push_back(static_cast<unsigned char>(content.size() >> 8));
		out.push_back(static_cast<unsigned char>(content.size()));
	}
	return cat({out, content});
}

Bytes integer(const Bytes &magnitude) {
	Bytes content = strip(magnitude);
	if (content.empty() || (content[0] & 0x80) != 0) {
		content.insert(content.begin(), 0);
	}
	return element(0x02, content);
}

Bytes sequence(const Bytes &content) {
	return element(0x30, content);
}

const Bytes RSA_OID = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01};

// The nine integers of a PKCS#1 key, version first.
struct Fields {
	Bytes version, n, e, d, p, q, dp, dq, qinv;

	Bytes der() const {
		return sequence(cat({integer(version), integer(n), integer(e), integer(d), integer(p), integer(q), integer(dp),
			integer(dq), integer(qinv)}));
	}
};

// Reads a writer's PKCS#1 DER back, trusting its layout.
Fields readFields(const Bytes &der) {
	size_t pos = 0;
	auto header = [&](unsigned char tag) {
		CHECK(der.at(pos) == tag);
		pos++;
		size_t length = der.at(pos++);
		if (length > 0x80) {
			size_t count = length & 0x7F;
			length = 0;
			while (count-- > 0) {
				length = (length << 8) | der.at(pos++);
			}
		}
		return length;
	};
	auto next = [&]() {
		size_t length = header(0x02);
		Bytes value(der.begin() + static_cast<std::ptrdiff_t>(pos), der.begin() + static_cast<std::ptrdiff_t>(pos + length));
		pos += length;
		return strip(value);
	};
	CHECK(header(0x30) == der.size() - pos);
	Fields f;
	f.version = next();
	f.n = next();
	f.e = next();
	f.d = next();
	f.p = next();
	f.q = next();
	f.dp = next();
	f.dq = next();
	f.qinv = next();
	CHECK(pos == der.size());
	return f;
}

Bytes pkcs8(const Bytes &pkcs1, const Bytes &algorithm, const Bytes &extra = Bytes()) {
	return sequence(cat({integer(Bytes()), sequence(algorithm), element(0x04, pkcs1), extra}));
}

// ---------------------------------------------------------------- checks

std::string P;
std::string Q;
std::string Modulus;

std::string errorOf(const std::string &text) {
	try {
		ParseRSAKeyPEM(text, "test.pem");
	} catch (const std::runtime_error &e) {
		return e.what();
	}
	return "";
}

// The text is refused with an error that names the file, holds What, and
// holds neither prime.
void refused(const std::string &text, const std::string &what, int line) {
	std::string error = errorOf(text);
	bool ok = error.compare(0, 10, "test.pem: ") == 0 && error.find(what) != std::string::npos
		&& error.find(P) == std::string::npos && error.find(Q) == std::string::npos;
	if (!ok) {
		std::fprintf(stderr, "expected \"test.pem: ...%s...\", got \"%s\"\n", what.c_str(), error.c_str());
	}
	test::report(ok, ("refused: " + what).c_str(), __FILE__, line);
}

#define REFUSED(text, what) refused(text, what, __LINE__)

std::string pem(const Bytes &der) {
	return PEMEncode("RSA PRIVATE KEY", der);
}

void testRoundTrip(const Fields &key) {
	std::string text = MakeRSAKeyPEM(P, Q);
	TRSAKeyFile read = ParseRSAKeyPEM(text, "test.pem");
	bool pSmaller = toVlong(fromDecimal(P)) < toVlong(fromDecimal(Q));
	CHECK(read.PrimeP == (pSmaller ? P : Q));
	CHECK(read.PrimeQ == (pSmaller ? Q : P));
	CHECK(read.Modulus == Modulus);
	CHECK(read.Modulus.size() >= 308);

	// The primes in the other order make the same key.
	TRSAKeyFile swapped = ParseRSAKeyPEM(MakeRSAKeyPEM(Q, P), "test.pem");
	CHECK(swapped.PrimeP == read.PrimeP && swapped.PrimeQ == read.PrimeQ && swapped.Modulus == read.Modulus);

	// PEM lines: BEGIN, base64 of 64 characters (the last one up to 64), END.
	std::vector<std::string> lines;
	for (size_t start = 0; start < text.size();) {
		size_t end = text.find('\n', start);
		CHECK(end != std::string::npos);
		lines.push_back(text.substr(start, end - start));
		start = end + 1;
	}
	CHECK(lines.size() > 3);
	CHECK(lines.front() == "-----BEGIN RSA PRIVATE KEY-----");
	CHECK(lines.back() == "-----END RSA PRIVATE KEY-----");
	for (size_t i = 1; i + 1 < lines.size(); i++) {
		CHECK(i + 2 == lines.size() ? lines[i].size() <= 64 && !lines[i].empty() : lines[i].size() == 64);
	}

	// The fields the writer computed (with the primes in the order given).
	vlong n = toVlong(key.n);
	vlong e = toVlong(key.e);
	vlong d = toVlong(key.d);
	vlong p = toVlong(key.p);
	vlong q = toVlong(key.q);
	vlong phi = (p - 1) * (q - 1);
	CHECK(key.version.empty());
	CHECK(key.p == fromDecimal(P) && key.q == fromDecimal(Q));
	CHECK(equal(n, p * q));
	CHECK(toDecimal(n) == Modulus);
	CHECK(key.e == Bytes({0x01, 0x00, 0x01}));
	CHECK((d * e) % phi == 1);
	CHECK(d < phi);
	CHECK(equal(toVlong(key.dp), d % (p - 1)));
	CHECK(equal(toVlong(key.dq), d % (q - 1)));
	CHECK((toVlong(key.qinv) * q) % p == 1);
	CHECK(toVlong(key.qinv) < p);

	// The key works: a block raised to e and then to d comes back.
	Bytes block(128);
	for (size_t i = 1; i < block.size(); i++) {
		block[i] = static_cast<unsigned char>(i * 31 + 7);
	}
	vlong plain = toVlong(block);
	CHECK(equal(vlong::modexp(vlong::modexp(plain, e, n), d, n), plain));
}

void testPKCS8(const Fields &key) {
	Bytes der = key.der();
	Bytes algorithm = cat({element(0x06, RSA_OID), element(0x05, Bytes())});
	TRSAKeyFile read = ParseRSAKeyPEM(PEMEncode("PRIVATE KEY", pkcs8(der, algorithm)), "test.pem");
	CHECK(read.Modulus == Modulus);
	CHECK(read.PrimeP == ParseRSAKeyPEM(pem(der), "test.pem").PrimeP);

	// Without the NULL parameters, and with attributes.
	CHECK(errorOf(PEMEncode("PRIVATE KEY", pkcs8(der, element(0x06, RSA_OID)))) == "");
	CHECK(errorOf(PEMEncode("PRIVATE KEY", pkcs8(der, algorithm, element(0xA0, Bytes{0x30, 0x00})))) == "");

	// Another algorithm (id-ecPublicKey), parameters that are not NULL, data
	// after the key, another version.
	const Bytes ecOid = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01};
	REFUSED(PEMEncode("PRIVATE KEY", pkcs8(der, element(0x06, ecOid))), "not an RSA key");
	REFUSED(PEMEncode("PRIVATE KEY", pkcs8(der, cat({element(0x06, RSA_OID), element(0x05, Bytes{0})}))), "not NULL");
	REFUSED(PEMEncode("PRIVATE KEY", pkcs8(der, algorithm, integer(Bytes{1}))), "data after the private key");
	REFUSED(PEMEncode("PRIVATE KEY", sequence(cat({integer(Bytes{1}), sequence(algorithm), element(0x04, der)}))),
		"version other than 0");
	Bytes inner = der;
	inner.push_back(0);
	REFUSED(PEMEncode("PRIVATE KEY", pkcs8(inner, algorithm)), "data after the key");

	// Each format under the other's label.
	REFUSED(PEMEncode("PRIVATE KEY", der), "another type of element");
	REFUSED(pem(pkcs8(der, algorithm)), "another type of element");
}

void testFields(const Fields &key) {
	Fields f = key;
	f.e = Bytes{3};
	REFUSED(pem(f.der()), "the public exponent is 3; the client needs 65537");
	f.e = Bytes(20, 0x11);
	REFUSED(pem(f.der()), "the public exponent is very large");

	// Moduli of 1023, 1025 and 2048 bits.
	f = key;
	f.n = toBytes(toVlong(key.n) / 2);
	REFUSED(pem(f.der()), "the modulus has 1023 bits; the client needs 1024");
	f.n = cat({Bytes{1}, key.n});
	REFUSED(pem(f.der()), "the modulus has 1025 bits");
	f.n = Bytes(256, 0xC5);
	REFUSED(pem(f.der()), "the modulus has 2048 bits");

	// A modulus that is not the product of the primes.
	f = key;
	f.n.back() ^= 0x02;
	REFUSED(pem(f.der()), "the modulus is not the product of the two primes");
	f = key;
	f.p = toBytes(toVlong(key.p) + 2);
	REFUSED(pem(f.der()), "the modulus is not the product of the two primes");
	f = key;
	f.p = Bytes{1};
	f.q = key.n;
	REFUSED(pem(f.der()), "a prime of the key is 0 or 1");
	f = key;
	f.q = key.p;
	// p * p has 1024 bits too: p has 512, the top two set.
	f.n = toBytes(toVlong(key.p) * toVlong(key.p));
	REFUSED(pem(f.der()), "the two primes of the key are the same");

	// Factors that are not odd primes, with the modulus their product: an
	// even one, one with the factor 3, and one with no factor below 2000
	// (the product of two 256-bit numbers), which only Miller-Rabin finds.
	vlong q = toVlong(key.q);
	f = key;
	f.p = toBytes(toVlong(key.p) + 1);
	f.n = toBytes(toVlong(f.p) * q);
	REFUSED(pem(f.der()), "a factor of the modulus is even; the key needs two odd primes");
	vlong threes = toVlong(key.p) + 2;
	while (threes % 3 != 0) {
		threes = threes + 2;
	}
	f.p = toBytes(threes);
	f.n = toBytes(threes * q);
	REFUSED(pem(f.der()), "a factor of the modulus is not a prime");
	vlong composite = noSmallFactor(Bytes(32, 0xF1)) * noSmallFactor(Bytes(32, 0xE3));
	f.p = toBytes(composite);
	f.n = toBytes(composite * q);
	CHECK(f.p.size() == 64);
	REFUSED(pem(f.der()), "a factor of the modulus is not a prime");
	REFUSED(pem(Fields{Bytes(), f.n, key.e, key.d, f.q, f.p, key.dp, key.dq, key.qinv}.der()),
		"a factor of the modulus is not a prime");
	CHECK_THROWS(MakeRSAKeyDER(toDecimal(composite), Q), std::runtime_error);
	CHECK_THROWS(MakeRSAKeyDER(Q, toDecimal(threes)), std::runtime_error);

	// Version 1 (more than two primes) and other versions.
	f = key;
	f.version = Bytes{1};
	REFUSED(pem(f.der()), "version 1");
	f.version = Bytes{2};
	REFUSED(pem(f.der()), "unknown version");

	// A tenth integer.
	Bytes der = key.der();
	Bytes content(der.begin() + 4, der.end());
	REFUSED(pem(sequence(cat({content, integer(Bytes{5})}))), "more than the nine integers");

	// A private integer larger than the modulus.
	f = key;
	f.d = Bytes(130, 0x22);
	REFUSED(pem(f.der()), "larger than the modulus");
}

void testDER(const Fields &key) {
	Bytes der = key.der();

	// Every shorter piece of the key is refused.
	int truncated = 0;
	for (size_t size = 0; size < der.size(); size++) {
		std::string error = errorOf(pem(Bytes(der.begin(), der.begin() + static_cast<std::ptrdiff_t>(size))));
		if (error.compare(0, 10, "test.pem: ") == 0) {
			truncated++;
		}
	}
	CHECK(truncated == static_cast<int>(der.size()));
	REFUSED(pem(Bytes(der.begin(), der.end() - 1)), "runs past the end of the key data");

	// Data after the key.
	Bytes trailing = der;
	trailing.push_back(0);
	REFUSED(pem(trailing), "there is data after the key");

	// Lengths: longer than what follows, too long to hold, indefinite, not minimal.
	REFUSED(pem(Bytes{0x30, 0x84, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x01, 0x00}), "runs past the end of the key data");
	REFUSED(pem(Bytes{0x30, 0x88, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x01, 0x00}),
		"more than four bytes");
	REFUSED(pem(Bytes{0x30, 0x80, 0x02, 0x01, 0x00, 0x00, 0x00}), "indefinite length");
	REFUSED(pem(Bytes{0x30, 0x81, 0x03, 0x02, 0x01, 0x00}), "not minimal");
	REFUSED(pem(Bytes{0x30, 0x82, 0x00, 0x03, 0x02, 0x01, 0x00}), "not minimal");
	REFUSED(pem(Bytes{0x30, 0x06, 0x02, 0x84, 0x7F, 0xFF, 0xFF, 0xFF}), "runs past the end of the key data");
	REFUSED(pem(Bytes{0x30, 0x02, 0x02, 0x81}), "ends in the length");

	// Integers: empty, negative, not minimal.
	REFUSED(pem(sequence(Bytes{0x02, 0x00})), "empty integer");
	REFUSED(pem(sequence(cat({integer(Bytes()), Bytes{0x02, 0x01, 0x80}}))), "the modulus is negative");
	REFUSED(pem(sequence(Bytes{0x02, 0x02, 0x00, 0x01})), "not minimal");
	REFUSED(pem(Bytes{0x02, 0x01, 0x00}), "another type of element");
}

void testPEM(const Fields &key) {
	std::string good = pem(key.der());

	// Empty and blank files.
	REFUSED("", "the file is empty");
	REFUSED("\n  \n\t\n", "only blank lines");

	// Encrypted keys, both kinds.
	std::string body = good.substr(good.find('\n') + 1);
	REFUSED("-----BEGIN RSA PRIVATE KEY-----\nProc-Type: 4,ENCRYPTED\nDEK-Info: AES-128-CBC,00112233445566778899AABBCCDDEEFF\n\n"
		+ body, "the key is encrypted");
	REFUSED("-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIIBAA==\n-----END ENCRYPTED PRIVATE KEY-----\n",
		"the key is encrypted");

	// Other headers, labels, a missing END line, text around the block.
	REFUSED("-----BEGIN RSA PRIVATE KEY-----\nComment: hello\n" + body, "line 2 is a PEM header");
	REFUSED(PEMEncode("PUBLIC KEY", key.der()), "holds a \"PUBLIC KEY\", not an RSA private key");
	REFUSED(PEMEncode("EC PRIVATE KEY", key.der()), "not an RSA private key");
	REFUSED(good.substr(0, good.rfind("-----END")), "there is no \"-----END RSA PRIVATE KEY-----\" line");
	REFUSED(good.substr(0, good.rfind("-----END")) + "-----END PRIVATE KEY-----\n", "line");
	REFUSED("Some text first\n" + good, "line 1 is not the BEGIN line");
	REFUSED(good + "more\n", "text after the END line");

	// Bad base64: a character outside the alphabet, a missing character,
	// padding inside, padding bits that are not zero.
	std::string star = good;
	star[40] = '*';
	REFUSED(star, "line 2 has a character that is not base64");
	std::string shorter = good;
	shorter.erase(40, 1);
	REFUSED(shorter, "the base64 text is malformed");
	std::string inside = good;
	inside[40] = '=';
	REFUSED(inside, "the base64 text is malformed");
	REFUSED("-----BEGIN RSA PRIVATE KEY-----\nQR==\n-----END RSA PRIVATE KEY-----\n", "the base64 text is malformed");
	REFUSED("-----BEGIN RSA PRIVATE KEY-----\nQQ===\n-----END RSA PRIVATE KEY-----\n", "the base64 text is malformed");
	// "QQ==" is good base64 (one byte, 0x41), which is no key.
	REFUSED("-----BEGIN RSA PRIVATE KEY-----\nQQ==\n-----END RSA PRIVATE KEY-----\n", "another type of element");

	// What is allowed around the base64: Windows line ends, blank lines
	// before and after, spaces at line ends, other line lengths.
	std::string windows;
	for (char c : good) {
		if (c == '\n') {
			windows += '\r';
		}
		windows += c;
	}
	CHECK(errorOf(windows) == "");
	CHECK(errorOf("\n\n" + good + "\n  \n") == "");
	std::string joined = "-----BEGIN RSA PRIVATE KEY-----\n";
	for (char c : body.substr(0, body.find("-----"))) {
		if (c != '\n') {
			joined += c;
		}
	}
	CHECK(errorOf(joined + "  \n-----END RSA PRIVATE KEY-----") == "");
}

void testFiles() {
	char folder[] = "/tmp/patella-rsakeyfile-XXXXXX";
	CHECK(mkdtemp(folder) != nullptr);
	std::string file = std::string(folder) + "/key.pem";

	std::string missing;
	try {
		ReadRSAKeyFile(file);
	} catch (const std::runtime_error &e) {
		missing = e.what();
	}
	CHECK(missing == file + ": cannot open the key file: No such file or directory");

	std::string text = MakeRSAKeyPEM(P, Q);
	WriteRSAKeyFile(file, text);
	struct stat status;
	CHECK(stat(file.c_str(), &status) == 0 && (status.st_mode & 0777) == 0600);
	CHECK(ReadRSAKeyFile(file).Modulus == Modulus);
	CHECK_THROWS(WriteRSAKeyFile(file, "other"), std::runtime_error);
	CHECK(ReadRSAKeyFile(file).Modulus == Modulus);

	std::string directory;
	try {
		ReadRSAKeyFile(folder);
	} catch (const std::runtime_error &e) {
		directory = e.what();
	}
	CHECK(directory.find(": cannot read the key file") != std::string::npos);

	std::string large = std::string(folder) + "/large.pem";
	WriteRSAKeyFile(large, text + std::string(70 * 1024, '\n'));
	std::string tooLarge;
	try {
		ReadRSAKeyFile(large);
	} catch (const std::runtime_error &e) {
		tooLarge = e.what();
	}
	CHECK(tooLarge == large + ": the file is larger than 64 KiB, not a key file");

	unlink(file.c_str());
	unlink(large.c_str());
	CHECK(rmdir(folder) == 0);
}

void testHeader() {
	std::string header =
		"#ifndef PATELLA_RSA_PRIVATEKEY_HH\n"
		"#define PATELLA_RSA_PRIVATEKEY_HH\n"
		"// RSA_PRIME_P and RSA_PRIME_Q are the primes.\n"
		"static const char RSA_PRIME_P[156] = \"" + P + "\";\n"
		"static const char RSA_PRIME_Q[156] =\n\t\"" + Q.substr(0, 50) + "\"\n\t\"" + Q.substr(50) + "\";\n"
		"#endif\n";
	std::string p;
	std::string q;
	ParseRSAKeyHeader(header, "old.hh", p, q);
	CHECK(p == P && q == Q);
	CHECK(ParseRSAKeyPEM(MakeRSAKeyPEM(p, q), "test.pem").Modulus == Modulus);
	CHECK_THROWS(ParseRSAKeyHeader("static const char RSA_PRIME_P[] = \"7\";\n", "old.hh", p, q), std::runtime_error);
	CHECK_THROWS(ParseRSAKeyHeader("static const char XRSA_PRIME_P[] = \"7\";\nRSA_PRIME_Q = \"5\";\n", "old.hh", p, q),
		std::runtime_error);

	// Primes that make no key.
	CHECK_THROWS(MakeRSAKeyDER("3", "5"), std::runtime_error);
	CHECK_THROWS(MakeRSAKeyDER(P, P), std::runtime_error);
	CHECK_THROWS(MakeRSAKeyDER(P, "12x4"), std::runtime_error);
	CHECK_THROWS(MakeRSAKeyDER("", Q), std::runtime_error);
}

void all() {
	GenerateRSAPrimes(P, Q);
	CHECK(P != Q);
	vlong n = toVlong(fromDecimal(P)) * toVlong(fromDecimal(Q));
	Modulus = toDecimal(n);
	Fields key = readFields(MakeRSAKeyDER(P, Q));
	testRoundTrip(key);
	testPKCS8(key);
	testFields(key);
	testDER(key);
	testPEM(key);
	testFiles();
	testHeader();
}

} // namespace

int main() {
	return test::run(all);
}

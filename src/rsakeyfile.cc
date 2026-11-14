// rsakeyfile.cc: the server's RSA private key as a PEM file (rsakeyfile.hh):
// reading and checking a key file, writing one, reading the primes of an old
// RSA_PrivateKey.hh, and making new primes. Base64 and DER are read and
// written here, strictly: definite lengths of at most four bytes, minimal
// encodings, every length checked against what is left, nothing after the
// key. Numbers are big-endian bytes without leading zeros (zero is empty);
// the arithmetic is vlong's.

#include "rsakeyfile.hh"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

#include "vlong.hh"

namespace {

typedef std::vector<unsigned char> TBytes;

// The largest key file read: a 1024-bit key takes about 900 bytes in PEM.
const size_t MAX_KEY_FILE_SIZE = 64 * 1024;
// The largest integer read, in bytes: a modulus of 8192 bits with the zero
// byte that keeps it positive, so that a key of the wrong size is reported
// with its size.
const size_t MAX_INTEGER_SIZE = 1025;
// The largest private integer (private exponent, exponents of the primes,
// coefficient), in bytes: below the modulus, with the zero byte.
const size_t MAX_PRIVATE_INTEGER_SIZE = RSA_KEY_BITS / 8 + 1;
// Bytes of the buffer vlong numbers are converted through: a product of two
// numbers of up to 1024 bits, rounded up to whole 32-bit words.
const unsigned short VLONG_BUFFER_SIZE = 2 * RSA_KEY_BITS / 8 + 8;
// Longest decimal text of a prime taken from a header.
const size_t MAX_DECIMAL_DIGITS = 1000;
// Base64 characters per PEM line.
const size_t PEM_LINE_LENGTH = 64;

// DER tags.
const unsigned char DER_INTEGER = 0x02;
const unsigned char DER_OCTET_STRING = 0x04;
const unsigned char DER_NULL = 0x05;
const unsigned char DER_OID = 0x06;
const unsigned char DER_SEQUENCE = 0x30;
// [0], constructed: the attributes of a PKCS#8 key.
const unsigned char DER_CONTEXT_0 = 0xA0;

// rsaEncryption, 1.2.840.113549.1.1.1.
const unsigned char RSA_ENCRYPTION_OID[] = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01};

const char BASE64_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const char ENCRYPTED_TEXT[] = "the key is encrypted; write it out unencrypted first (openssl pkey -in FILE -out key.pem)";

[[noreturn]] void Fail(const std::string &Name, const std::string &What) {
	throw std::runtime_error(Name + ": " + What);
}

// ---------------------------------------------------------------- numbers

TBytes Strip(const TBytes &Number) {
	size_t First = 0;
	while (First < Number.size() && Number[First] == 0) {
		First++;
	}
	return TBytes(Number.begin() + static_cast<std::ptrdiff_t>(First), Number.end());
}

// Compares two stripped numbers: <0, 0 or >0.
int Compare(const TBytes &A, const TBytes &B) {
	if (A.size() != B.size()) {
		return A.size() < B.size() ? -1 : 1;
	}
	for (size_t i = 0; i < A.size(); i++) {
		if (A[i] != B[i]) {
			return A[i] < B[i] ? -1 : 1;
		}
	}
	return 0;
}

// Bits of a stripped number.
size_t BitsOf(const TBytes &Number) {
	if (Number.empty()) {
		return 0;
	}
	size_t Bits = Number.size() * 8;
	for (unsigned char Top = Number[0]; (Top & 0x80) == 0; Top = static_cast<unsigned char>(Top << 1)) {
		Bits--;
	}
	return Bits;
}

std::string ToDecimal(const TBytes &Value) {
	TBytes Number = Strip(Value);
	std::string Digits;
	while (!Number.empty()) {
		unsigned int Remainder = 0;
		for (unsigned char &Byte : Number) {
			unsigned int Part = Remainder * 256 + Byte;
			Byte = static_cast<unsigned char>(Part / 10);
			Remainder = Part % 10;
		}
		Digits.push_back(static_cast<char>('0' + Remainder));
		Number = Strip(Number);
	}
	if (Digits.empty()) {
		return "0";
	}
	std::reverse(Digits.begin(), Digits.end());
	return Digits;
}

// Returns false for a text that is not a decimal number.
bool FromDecimal(const std::string &Text, TBytes &Number) {
	if (Text.empty() || Text.size() > MAX_DECIMAL_DIGITS) {
		return false;
	}
	Number.clear();
	for (char Digit : Text) {
		if (Digit < '0' || Digit > '9') {
			return false;
		}
		unsigned int Carry = static_cast<unsigned int>(Digit - '0');
		for (auto Byte = Number.rbegin(); Byte != Number.rend(); ++Byte) {
			unsigned int Part = *Byte * 10u + Carry;
			*Byte = static_cast<unsigned char>(Part);
			Carry = Part >> 8;
		}
		if (Carry != 0) {
			Number.insert(Number.begin(), static_cast<unsigned char>(Carry));
		}
	}
	Number = Strip(Number);
	return true;
}

// vlong reads whole 32-bit words from the front of the data, so the bytes are
// padded at the front to a multiple of four.
vlong ToVlong(const TBytes &Number) {
	vlong Result;
	TBytes Padded((4 - Number.size() % 4) % 4, 0);
	Padded.insert(Padded.end(), Number.begin(), Number.end());
	if (!Padded.empty()) {
		vlong::convert(Padded.data(), static_cast<unsigned short>(Padded.size()), Result);
	}
	return Result;
}

// For numbers below 2^(8 * VLONG_BUFFER_SIZE).
TBytes FromVlong(const vlong &Number) {
	unsigned char Buffer[VLONG_BUFFER_SIZE];
	vlong::convert(Number, Buffer, VLONG_BUFFER_SIZE);
	return Strip(TBytes(Buffer, Buffer + VLONG_BUFFER_SIZE));
}

// ---------------------------------------------------------------- base64 and PEM

bool DecodeBase64(const std::string &Text, TBytes &Data) {
	if (Text.empty() || Text.size() % 4 != 0) {
		return false;
	}
	size_t Padding = 0;
	while (Padding < 2 && Text[Text.size() - 1 - Padding] == '=') {
		Padding++;
	}
	Data.clear();
	unsigned int Bits = 0;
	int Count = 0;
	for (size_t i = 0; i < Text.size() - Padding; i++) {
		const char *Found = Text[i] == 0 ? nullptr : strchr(BASE64_ALPHABET, Text[i]);
		if (Found == nullptr) {
			return false;
		}
		Bits = (Bits << 6) | static_cast<unsigned int>(Found - BASE64_ALPHABET);
		Count += 6;
		if (Count >= 8) {
			Count -= 8;
			Data.push_back(static_cast<unsigned char>(Bits >> Count));
			Bits &= (1u << Count) - 1;
		}
	}
	// The bits the padding leaves over must be zero.
	return Bits == 0;
}

std::string EncodeBase64(const TBytes &Data) {
	std::string Text;
	for (size_t i = 0; i < Data.size(); i += 3) {
		unsigned int Group = static_cast<unsigned int>(Data[i]) << 16;
		if (i + 1 < Data.size()) {
			Group |= static_cast<unsigned int>(Data[i + 1]) << 8;
		}
		if (i + 2 < Data.size()) {
			Group |= Data[i + 2];
		}
		Text += BASE64_ALPHABET[(Group >> 18) & 63];
		Text += BASE64_ALPHABET[(Group >> 12) & 63];
		Text += i + 1 < Data.size() ? BASE64_ALPHABET[(Group >> 6) & 63] : '=';
		Text += i + 2 < Data.size() ? BASE64_ALPHABET[Group & 63] : '=';
	}
	return Text;
}

bool IsBlank(const std::string &Line) {
	return Line.find_first_not_of(" \t") == std::string::npos;
}

// The lines of a text, without their '\n' or "\r\n".
std::vector<std::string> SplitLines(const std::string &Text) {
	std::vector<std::string> Lines;
	size_t Start = 0;
	while (Start < Text.size()) {
		size_t End = Text.find('\n', Start);
		if (End == std::string::npos) {
			End = Text.size();
		}
		std::string Line = Text.substr(Start, End - Start);
		if (!Line.empty() && Line.back() == '\r') {
			Line.pop_back();
		}
		Lines.push_back(Line);
		Start = End + 1;
	}
	return Lines;
}

std::string LineText(size_t Index) {
	return "line " + std::to_string(Index + 1);
}

// ---------------------------------------------------------------- DER

struct TKeyParts {
	TBytes Modulus;
	TBytes PublicExponent;
	TBytes PrivateExponent;
	TBytes Prime1;
	TBytes Prime2;
	TBytes Exponent1;
	TBytes Exponent2;
	TBytes Coefficient;
};

// Reads the element at Pos, which must have the tag Tag, and moves Pos past
// it; Start and Length give its content. End is where the enclosing element
// ends.
void ReadElement(const unsigned char *Data, size_t &Pos, size_t End, unsigned char Tag, const std::string &What,
		const std::string &Name, size_t &Start, size_t &Length) {
	if (Pos >= End) {
		Fail(Name, "the key data ends before " + What);
	}
	if (Data[Pos] != Tag) {
		Fail(Name, "the key data has another type of element where " + What + " belongs");
	}
	Pos++;
	if (Pos >= End) {
		Fail(Name, "the key data ends in the length of " + What);
	}
	unsigned char First = Data[Pos++];
	size_t Size = First;
	if (First == 0x80) {
		Fail(Name, "the key data gives " + What + " an indefinite length (not DER)");
	}
	if (First > 0x80) {
		size_t Count = First & 0x7Fu;
		if (Count > 4) {
			Fail(Name, "the length of " + What + " has more than four bytes");
		}
		if (Count > End - Pos) {
			Fail(Name, "the key data ends in the length of " + What);
		}
		if (Data[Pos] == 0) {
			Fail(Name, "the length of " + What + " is not minimal (not DER)");
		}
		Size = 0;
		for (size_t i = 0; i < Count; i++) {
			Size = (Size << 8) | Data[Pos++];
		}
		if (Size < 0x80) {
			Fail(Name, "the length of " + What + " is not minimal (not DER)");
		}
	}
	if (Size > End - Pos) {
		Fail(Name, What + " runs past the end of the key data");
	}
	Start = Pos;
	Length = Size;
	Pos += Size;
}

// Reads a non-negative INTEGER of at most Limit content bytes into Value.
void ReadInteger(const unsigned char *Data, size_t &Pos, size_t End, const std::string &What, const std::string &Name,
		size_t Limit, TBytes &Value) {
	size_t Start;
	size_t Length;
	ReadElement(Data, Pos, End, DER_INTEGER, What, Name, Start, Length);
	if (Length == 0) {
		Fail(Name, What + " is an empty integer");
	}
	if ((Data[Start] & 0x80) != 0) {
		Fail(Name, What + " is negative");
	}
	if (Length > 1 && Data[Start] == 0 && (Data[Start + 1] & 0x80) == 0) {
		Fail(Name, What + " is not minimal (not DER)");
	}
	if (Length > Limit) {
		Fail(Name, What + " is too large");
	}
	Value = Strip(TBytes(Data + Start, Data + Start + Length));
}

// An RSAPrivateKey (PKCS#1) filling Data[Start..End).
void ParsePKCS1(const unsigned char *Data, size_t Start, size_t End, const std::string &Name, TKeyParts &Parts) {
	size_t Pos = Start;
	size_t Content;
	size_t Length;
	ReadElement(Data, Pos, End, DER_SEQUENCE, "the key's sequence", Name, Content, Length);
	if (Pos != End) {
		Fail(Name, "there is data after the key");
	}
	Pos = Content;
	End = Content + Length;
	TBytes Version;
	ReadInteger(Data, Pos, End, "the key's version", Name, 4, Version);
	if (!Version.empty()) {
		Fail(Name, Version == TBytes{1} ? "the key has version 1, a key of more than two primes; it is not supported"
				: "the key has an unknown version");
	}
	ReadInteger(Data, Pos, End, "the modulus", Name, MAX_INTEGER_SIZE, Parts.Modulus);
	ReadInteger(Data, Pos, End, "the public exponent", Name, MAX_INTEGER_SIZE, Parts.PublicExponent);
	ReadInteger(Data, Pos, End, "the private exponent", Name, MAX_INTEGER_SIZE, Parts.PrivateExponent);
	ReadInteger(Data, Pos, End, "the first prime", Name, MAX_INTEGER_SIZE, Parts.Prime1);
	ReadInteger(Data, Pos, End, "the second prime", Name, MAX_INTEGER_SIZE, Parts.Prime2);
	ReadInteger(Data, Pos, End, "the first prime's exponent", Name, MAX_INTEGER_SIZE, Parts.Exponent1);
	ReadInteger(Data, Pos, End, "the second prime's exponent", Name, MAX_INTEGER_SIZE, Parts.Exponent2);
	ReadInteger(Data, Pos, End, "the coefficient", Name, MAX_INTEGER_SIZE, Parts.Coefficient);
	if (Pos != End) {
		Fail(Name, "the key's sequence has more than the nine integers of a key of two primes");
	}
}

// A PrivateKeyInfo (PKCS#8) of an RSA key filling Data[0..Size).
void ParsePKCS8(const unsigned char *Data, size_t Size, const std::string &Name, TKeyParts &Parts) {
	size_t Pos = 0;
	size_t Content;
	size_t Length;
	ReadElement(Data, Pos, Size, DER_SEQUENCE, "the PKCS#8 sequence", Name, Content, Length);
	if (Pos != Size) {
		Fail(Name, "there is data after the key");
	}
	Pos = Content;
	size_t End = Content + Length;
	TBytes Version;
	ReadInteger(Data, Pos, End, "the PKCS#8 version", Name, 4, Version);
	if (!Version.empty()) {
		Fail(Name, "the PKCS#8 key has a version other than 0; it is not supported");
	}

	size_t Algorithm;
	ReadElement(Data, Pos, End, DER_SEQUENCE, "the PKCS#8 algorithm", Name, Algorithm, Length);
	size_t AlgorithmEnd = Algorithm + Length;
	size_t Oid;
	ReadElement(Data, Algorithm, AlgorithmEnd, DER_OID, "the PKCS#8 algorithm's identifier", Name, Oid, Length);
	if (Length != sizeof(RSA_ENCRYPTION_OID) || memcmp(Data + Oid, RSA_ENCRYPTION_OID, Length) != 0) {
		Fail(Name, "the PKCS#8 key is not an RSA key (its algorithm is not rsaEncryption)");
	}
	if (Algorithm < AlgorithmEnd) {
		size_t Parameters;
		ReadElement(Data, Algorithm, AlgorithmEnd, DER_NULL, "the parameters of rsaEncryption (NULL)", Name,
				Parameters, Length);
		if (Length != 0) {
			Fail(Name, "the parameters of rsaEncryption are not NULL");
		}
	}
	if (Algorithm != AlgorithmEnd) {
		Fail(Name, "the PKCS#8 algorithm has more than an identifier and NULL");
	}

	size_t Key;
	ReadElement(Data, Pos, End, DER_OCTET_STRING, "the PKCS#8 private key", Name, Key, Length);
	ParsePKCS1(Data, Key, Key + Length, Name, Parts);
	if (Pos < End && Data[Pos] == DER_CONTEXT_0) {
		size_t Attributes;
		ReadElement(Data, Pos, End, DER_CONTEXT_0, "the PKCS#8 attributes", Name, Attributes, Length);
	}
	if (Pos != End) {
		Fail(Name, "the PKCS#8 sequence has data after the private key");
	}
}

void AppendLength(TBytes &Out, size_t Length) {
	if (Length < 0x80) {
		Out.push_back(static_cast<unsigned char>(Length));
		return;
	}
	TBytes Bytes;
	for (size_t Rest = Length; Rest != 0; Rest >>= 8) {
		Bytes.insert(Bytes.begin(), static_cast<unsigned char>(Rest));
	}
	Out.push_back(static_cast<unsigned char>(0x80 | Bytes.size()));
	Out.insert(Out.end(), Bytes.begin(), Bytes.end());
}

void AppendInteger(TBytes &Out, const TBytes &Value) {
	TBytes Content = Strip(Value);
	if (Content.empty() || (Content[0] & 0x80) != 0) {
		Content.insert(Content.begin(), 0);
	}
	Out.push_back(DER_INTEGER);
	AppendLength(Out, Content.size());
	Out.insert(Out.end(), Content.begin(), Content.end());
}

// ---------------------------------------------------------------- checks

// Checks that the primes P and Q make a key the client can use, whose
// modulus is Modulus (when given), and returns the private exponent.
TBytes CheckPrimes(const TBytes &P, const TBytes &Q, const TBytes *Modulus, const std::string &Name) {
	if (Compare(P, TBytes{1}) <= 0 || Compare(Q, TBytes{1}) <= 0) {
		Fail(Name, "a prime of the key is 0 or 1");
	}
	if (Compare(P, Q) == 0) {
		Fail(Name, "the two primes of the key are the same");
	}
	// The product of numbers of a and b bits has a + b - 1 or a + b bits.
	size_t Bits = BitsOf(P) + BitsOf(Q);
	if (Bits < static_cast<size_t>(RSA_KEY_BITS) || Bits > static_cast<size_t>(RSA_KEY_BITS) + 1) {
		Fail(Name, Modulus != nullptr ? "the modulus is not the product of the two primes"
				: "the product of the two primes does not have " + std::to_string(RSA_KEY_BITS) + " bits");
	}
	vlong p = ToVlong(P);
	vlong q = ToVlong(Q);
	TBytes Product = FromVlong(p * q);
	if (Modulus != nullptr && Compare(Product, *Modulus) != 0) {
		Fail(Name, "the modulus is not the product of the two primes");
	}
	if (BitsOf(Product) != static_cast<size_t>(RSA_KEY_BITS)) {
		Fail(Name, "the product of the two primes does not have " + std::to_string(RSA_KEY_BITS) + " bits");
	}
	// TRSAPrivateKey::init computes these two inverses; they exist for two
	// different primes with 65537 not dividing either less one.
	vlong e(RSA_KEY_EXPONENT);
	vlong Phi = (p - 1) * (q - 1);
	vlong d = vlong::modinv(e, Phi);
	vlong U = vlong::modinv(p, q);
	if ((d * e) % Phi != 1 || (U * p) % q != 1) {
		Fail(Name, "the primes do not make a usable key (65537 has no inverse, or they are not primes)");
	}
	return FromVlong(d);
}

TRSAKeyFile CheckKey(const TKeyParts &Parts, const std::string &Name) {
	size_t Bits = BitsOf(Parts.Modulus);
	if (Bits != static_cast<size_t>(RSA_KEY_BITS)) {
		Fail(Name, "the modulus has " + std::to_string(Bits) + " bits; the client needs " + std::to_string(RSA_KEY_BITS));
	}
	vlong Exponent(RSA_KEY_EXPONENT);
	if (Compare(Parts.PublicExponent, FromVlong(Exponent)) != 0) {
		Fail(Name, "the public exponent is " + (Parts.PublicExponent.size() <= 8 ? ToDecimal(Parts.PublicExponent)
				: std::string("very large")) + "; the client needs " + std::to_string(RSA_KEY_EXPONENT));
	}
	for (const TBytes *Private : {&Parts.PrivateExponent, &Parts.Exponent1, &Parts.Exponent2, &Parts.Coefficient}) {
		if (Private->size() > MAX_PRIVATE_INTEGER_SIZE) {
			Fail(Name, "a private integer of the key is larger than the modulus");
		}
	}
	CheckPrimes(Parts.Prime1, Parts.Prime2, &Parts.Modulus, Name);

	TRSAKeyFile Key;
	bool FirstSmaller = Compare(Parts.Prime1, Parts.Prime2) < 0;
	Key.PrimeP = ToDecimal(FirstSmaller ? Parts.Prime1 : Parts.Prime2);
	Key.PrimeQ = ToDecimal(FirstSmaller ? Parts.Prime2 : Parts.Prime1);
	Key.Modulus = ToDecimal(Parts.Modulus);
	return Key;
}

// ---------------------------------------------------------------- primes

const unsigned int SMALL_PRIMES_LIMIT = 2000;
const int MILLER_RABIN_ROUNDS = 40;
const size_t PRIME_BYTES = RSA_KEY_BITS / 16;

class TRandom {
public:
	TRandom() : File(fopen("/dev/urandom", "rb")) {
		if (File == nullptr) {
			throw std::runtime_error(std::string("cannot open /dev/urandom: ") + strerror(errno));
		}
	}
	~TRandom() {
		fclose(File);
	}
	TRandom(const TRandom &) = delete;
	TRandom &operator=(const TRandom &) = delete;

	TBytes bytes(size_t Size) {
		TBytes Data(Size);
		if (fread(Data.data(), 1, Size, File) != Size) {
			throw std::runtime_error("cannot read /dev/urandom");
		}
		return Data;
	}

private:
	FILE *File;
};

bool IsProbablePrime(const vlong &n, TRandom &Random) {
	for (unsigned int p = 3; p < SMALL_PRIMES_LIMIT; p += 2) {
		bool PrimeP = true;
		for (unsigned int k = 3; k * k <= p; k += 2) {
			if (p % k == 0) {
				PrimeP = false;
				break;
			}
		}
		if (PrimeP && n % vlong(p) == 0) {
			return false;
		}
	}

	vlong NMinus1 = n - 1;
	vlong d = NMinus1;
	int s = 0;
	while (d % 2 == 0) {
		d = d / 2;
		s++;
	}

	for (int Round = 0; Round < MILLER_RABIN_ROUNDS; Round++) {
		vlong a = ToVlong(Random.bytes(PRIME_BYTES)) % (n - 3) + 2;
		vlong x = vlong::modexp(a, d, n);
		if (x == 1 || (x + 1) % n == 0) {
			continue;
		}
		bool Composite = true;
		for (int r = 1; r < s; r++) {
			x = vlong::modexp(x, 2, n);
			if ((x + 1) % n == 0) {
				Composite = false;
				break;
			}
		}
		if (Composite) {
			return false;
		}
	}
	return true;
}

TBytes RandomPrime(TRandom &Random) {
	for (;;) {
		TBytes Data = Random.bytes(PRIME_BYTES);
		Data[0] |= 0xC0;
		Data[PRIME_BYTES - 1] |= 0x01;
		vlong Candidate = ToVlong(Data);
		if ((Candidate - 1) % vlong(RSA_KEY_EXPONENT) == 0) {
			continue;
		}
		if (IsProbablePrime(Candidate, Random)) {
			return Data;
		}
	}
}

bool IsIdentifierCharacter(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

size_t SkipSpace(const std::string &Text, size_t Pos) {
	while (Pos < Text.size() && (Text[Pos] == ' ' || Text[Pos] == '\t' || Text[Pos] == '\n' || Text[Pos] == '\r')) {
		Pos++;
	}
	return Pos;
}

// The text of the string literals after "Key[...] =" (adjacent literals
// joined), at the first place where Key is defined that way.
bool FindHeaderString(const std::string &Text, const std::string &Key, std::string &Value) {
	for (size_t At = Text.find(Key); At != std::string::npos; At = Text.find(Key, At + 1)) {
		size_t Pos = At + Key.size();
		if ((At > 0 && IsIdentifierCharacter(Text[At - 1])) || (Pos < Text.size() && IsIdentifierCharacter(Text[Pos]))) {
			continue;
		}
		Pos = SkipSpace(Text, Pos);
		if (Pos < Text.size() && Text[Pos] == '[') {
			Pos = Text.find(']', Pos);
			if (Pos == std::string::npos) {
				return false;
			}
			Pos = SkipSpace(Text, Pos + 1);
		}
		if (Pos >= Text.size() || Text[Pos] != '=') {
			continue;
		}
		Pos = SkipSpace(Text, Pos + 1);
		std::string Joined;
		bool Found = false;
		while (Pos < Text.size() && Text[Pos] == '"') {
			size_t Close = Text.find('"', Pos + 1);
			if (Close == std::string::npos) {
				return false;
			}
			Joined += Text.substr(Pos + 1, Close - Pos - 1);
			Pos = SkipSpace(Text, Close + 1);
			Found = true;
		}
		if (Found) {
			Value = Joined;
			return true;
		}
	}
	return false;
}

} // namespace

TRSAKeyFile ParseRSAKeyPEM(const std::string &Text, const std::string &Name) {
	if (Text.empty()) {
		Fail(Name, "the file is empty");
	}
	std::vector<std::string> Lines = SplitLines(Text);
	size_t Index = 0;
	while (Index < Lines.size() && IsBlank(Lines[Index])) {
		Index++;
	}
	if (Index == Lines.size()) {
		Fail(Name, "the file holds only blank lines");
	}

	const std::string Begin = "-----BEGIN ";
	const std::string Dashes = "-----";
	const std::string &First = Lines[Index];
	if (First.compare(0, Begin.size(), Begin) != 0 || First.size() < Begin.size() + Dashes.size()
			|| First.compare(First.size() - Dashes.size(), Dashes.size(), Dashes) != 0) {
		Fail(Name, LineText(Index) + " is not the BEGIN line of a PEM key (-----BEGIN RSA PRIVATE KEY----- or "
				"-----BEGIN PRIVATE KEY-----)");
	}
	std::string Label = First.substr(Begin.size(), First.size() - Begin.size() - Dashes.size());
	if (Label == "ENCRYPTED PRIVATE KEY") {
		Fail(Name, ENCRYPTED_TEXT);
	}
	if (Label != "RSA PRIVATE KEY" && Label != "PRIVATE KEY") {
		Fail(Name, "the file holds a \"" + Label + "\", not an RSA private key (-----BEGIN RSA PRIVATE KEY----- or "
				"-----BEGIN PRIVATE KEY-----)");
	}

	const std::string EndLine = "-----END " + Label + "-----";
	std::string Base64;
	bool Ended = false;
	for (Index++; Index < Lines.size(); Index++) {
		std::string Line = Lines[Index];
		if (Line == EndLine) {
			Ended = true;
			break;
		}
		if (Line.find(':') != std::string::npos) {
			if (Line.find("ENCRYPTED") != std::string::npos) {
				Fail(Name, ENCRYPTED_TEXT);
			}
			Fail(Name, LineText(Index) + " is a PEM header; a key file has none");
		}
		if (Line.compare(0, Dashes.size(), Dashes) == 0) {
			Fail(Name, LineText(Index) + " is not \"" + EndLine + "\"");
		}
		while (!Line.empty() && (Line.back() == ' ' || Line.back() == '\t')) {
			Line.pop_back();
		}
		for (char c : Line) {
			if (c == 0 || (c != '=' && strchr(BASE64_ALPHABET, c) == nullptr)) {
				Fail(Name, LineText(Index) + " has a character that is not base64");
			}
		}
		Base64 += Line;
	}
	if (!Ended) {
		Fail(Name, "there is no \"" + EndLine + "\" line");
	}
	for (Index++; Index < Lines.size(); Index++) {
		if (!IsBlank(Lines[Index])) {
			Fail(Name, LineText(Index) + ": there is text after the END line");
		}
	}

	TBytes DER;
	if (!DecodeBase64(Base64, DER)) {
		Fail(Name, "the base64 text is malformed (its length or padding)");
	}
	TKeyParts Parts;
	if (Label == "RSA PRIVATE KEY") {
		ParsePKCS1(DER.data(), 0, DER.size(), Name, Parts);
	} else {
		ParsePKCS8(DER.data(), DER.size(), Name, Parts);
	}
	return CheckKey(Parts, Name);
}

TRSAKeyFile ReadRSAKeyFile(const std::string &FileName) {
	FILE *File = fopen(FileName.c_str(), "rb");
	if (File == nullptr) {
		Fail(FileName, std::string("cannot open the key file: ") + strerror(errno));
	}
	std::string Text;
	char Buffer[4096];
	size_t Count;
	while ((Count = fread(Buffer, 1, sizeof(Buffer), File)) > 0 && Text.size() <= MAX_KEY_FILE_SIZE) {
		Text.append(Buffer, Count);
	}
	bool Failed = ferror(File) != 0;
	int Error = errno;
	fclose(File);
	if (Failed) {
		Fail(FileName, std::string("cannot read the key file: ") + strerror(Error));
	}
	if (Text.size() > MAX_KEY_FILE_SIZE) {
		Fail(FileName, "the file is larger than " + std::to_string(MAX_KEY_FILE_SIZE / 1024) + " KiB, not a key file");
	}
	return ParseRSAKeyPEM(Text, FileName);
}

std::vector<unsigned char> MakeRSAKeyDER(const std::string &PrimeP, const std::string &PrimeQ) {
	const std::string Name = "the key";
	TBytes P;
	TBytes Q;
	if (!FromDecimal(PrimeP, P) || !FromDecimal(PrimeQ, Q)) {
		Fail(Name, "a prime is not a decimal number");
	}
	TBytes d = CheckPrimes(P, Q, nullptr, Name);
	vlong p = ToVlong(P);
	vlong q = ToVlong(Q);
	vlong dv = ToVlong(d);

	TBytes Content;
	AppendInteger(Content, TBytes());
	AppendInteger(Content, FromVlong(p * q));
	AppendInteger(Content, FromVlong(vlong(RSA_KEY_EXPONENT)));
	AppendInteger(Content, d);
	AppendInteger(Content, P);
	AppendInteger(Content, Q);
	AppendInteger(Content, FromVlong(dv % (p - 1)));
	AppendInteger(Content, FromVlong(dv % (q - 1)));
	AppendInteger(Content, FromVlong(vlong::modinv(q, p)));
	TBytes DER;
	DER.push_back(DER_SEQUENCE);
	AppendLength(DER, Content.size());
	DER.insert(DER.end(), Content.begin(), Content.end());
	return DER;
}

std::string PEMEncode(const std::string &Label, const std::vector<unsigned char> &DER) {
	std::string Base64 = EncodeBase64(DER);
	std::string Text = "-----BEGIN " + Label + "-----\n";
	for (size_t i = 0; i < Base64.size(); i += PEM_LINE_LENGTH) {
		Text += Base64.substr(i, PEM_LINE_LENGTH) + "\n";
	}
	Text += "-----END " + Label + "-----\n";
	return Text;
}

std::string MakeRSAKeyPEM(const std::string &PrimeP, const std::string &PrimeQ) {
	return PEMEncode("RSA PRIVATE KEY", MakeRSAKeyDER(PrimeP, PrimeQ));
}

void WriteRSAKeyFile(const std::string &FileName, const std::string &Text) {
	int File = open(FileName.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, S_IRUSR | S_IWUSR);
	if (File < 0) {
		if (errno == EEXIST) {
			Fail(FileName, "the file exists; a key file is never overwritten");
		}
		Fail(FileName, std::string("cannot create the key file: ") + strerror(errno));
	}
	size_t Written = 0;
	while (Written < Text.size()) {
		ssize_t Count = write(File, Text.data() + Written, Text.size() - Written);
		if (Count < 0 && errno == EINTR) {
			continue;
		}
		if (Count <= 0) {
			int Error = errno;
			close(File);
			unlink(FileName.c_str());
			Fail(FileName, std::string("cannot write the key file: ") + strerror(Error));
		}
		Written += static_cast<size_t>(Count);
	}
	if (close(File) != 0) {
		int Error = errno;
		unlink(FileName.c_str());
		Fail(FileName, std::string("cannot write the key file: ") + strerror(Error));
	}
}

void ParseRSAKeyHeader(const std::string &Text, const std::string &Name, std::string &PrimeP, std::string &PrimeQ) {
	if (!FindHeaderString(Text, "RSA_PRIME_P", PrimeP)) {
		Fail(Name, "no RSA_PRIME_P = \"...\" in the header");
	}
	if (!FindHeaderString(Text, "RSA_PRIME_Q", PrimeQ)) {
		Fail(Name, "no RSA_PRIME_Q = \"...\" in the header");
	}
}

void GenerateRSAPrimes(std::string &PrimeP, std::string &PrimeQ) {
	TRandom Random;
	TBytes P = RandomPrime(Random);
	TBytes Q = RandomPrime(Random);
	while (Compare(Strip(P), Strip(Q)) == 0) {
		Q = RandomPrime(Random);
	}
	PrimeP = ToDecimal(P);
	PrimeQ = ToDecimal(Q);
}

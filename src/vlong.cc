#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "vlong.hh"

vlong_montgomery::vlong_montgomery(const vlong &M) {
	m = M;
	N = 0;
	R = 1;
	while (R < M) {
		R += R;
		N += 1;
	}
	R1 = vlong::modinv(R - m, m);
	n1 = R - vlong::modinv(m, R);
}

void vlong_montgomery::mul(vlong &x, const vlong &y) {
	// T = x * y
	T.value->fast_mul(*x.value, *y.value, N * 2);

	// k = (T * n1) % R
	k.value->fast_mul(*T.value, *n1.value, N);

	// x = (T + k * m) / R
	x.value->fast_mul(*k.value, *m.value, N * 2);
	x += T;
	x.value->shr(N);

	if (x >= m) {
		x -= m;
	}
}

vlong vlong_montgomery::exp(const vlong &x, const vlong &e) {
	vlong result = R - m, t = (x * R) % m;
	unsigned int bits = e.value->bits();
	unsigned int i = 0;
	while (true) {
		if (e.value->test(i)) {
			mul(result, t);
		}
		i += 1;
		if (i == bits) {
			break;
		}
		mul(t, t);
	}
	return (result * R1) % m;
}

vlong_flex_unit::vlong_flex_unit() {
	z = 0;
	a = nullptr;
	n = 0;
}

vlong_flex_unit::~vlong_flex_unit() {
	unsigned int i = z;
	while (i) {
		i -= 1;
		a[i] = 0;
	}
	delete[] a;
}

unsigned int vlong_flex_unit::get(unsigned int i) const {
	if (i >= n) {
		return 0;
	}
	return a[i];
}

void vlong_flex_unit::clear() {
	n = 0;
}

void vlong_flex_unit::reserve(unsigned int x) {
	if (x > z) {
		unsigned int *na = new unsigned int[x];
		for (unsigned int i = 0; i < n; i += 1) {
			na[i] = a[i];
		}
		delete[] a;
		a = na;
		z = x;
	}
}

void vlong_flex_unit::set(unsigned int i, unsigned int x) {
	if (i < n) {
		a[i] = x;
		if (x == 0) {
			while (n && a[n - 1] == 0) {
				n -= 1;
			}
		}
	} else if (x) {
		reserve(i + 1);
		for (unsigned int j = n; j < i; j += 1) {
			a[j] = 0;
		}
		a[i] = x;
		n = i + 1;
	}
}

void vlong_flex_unit::fast_mul(vlong_flex_unit &x, vlong_flex_unit &y, unsigned int keep) {
	// *this = (x * y) % (2 ** keep)
	unsigned int i, limit = (keep + 31) / 32;
	reserve(limit);
	for (i = 0; i < limit; i += 1) {
		a[i] = 0;
	}
	unsigned int min = x.n;
	if (min > limit) {
		min = limit;
	}
	for (i = 0; i < min; i += 1) {
		unsigned int m = x.a[i];
		unsigned int c = 0;
		unsigned int min = i + y.n;
		if (min > limit) {
			min = limit;
		}
		unsigned int j;
		for (j = i; j < min; j += 1) {
			// c:a[j] = a[j] + c + m * y.a[j - i], from 16-bit halves
			unsigned int w, v = a[j], p = y.a[j - i];
			v += c;
			c = (v < c);
			w = (p & 0xFFFF) * (m & 0xFFFF);
			v += w;
			c += (v < w);
			w = (p & 0xFFFF) * (m >> 16);
			c += w >> 16;
			w = w << 16;
			v += w;
			c += (v < w);
			w = (p >> 16) * (m & 0xFFFF);
			c += w >> 16;
			w = w << 16;
			v += w;
			c += (v < w);
			c += (p >> 16) * (m >> 16);
			a[j] = v;
		}
		while (c && j < limit) {
			a[j] += c;
			c = a[j] < c;
			j += 1;
		}
	}

	keep %= 32;
	if (keep) {
		a[limit - 1] &= (1u << keep) - 1;
	}

	while (limit && a[limit - 1] == 0) {
		limit -= 1;
	}
	n = limit;
}

vlong_value::operator unsigned int() {
	return get(0);
}

int vlong_value::is_zero() const {
	return n == 0;
}

int vlong_value::test(unsigned int i) const {
	return (get(i / 32) & (1u << (i % 32))) != 0;
}

unsigned int vlong_value::bits() const {
	unsigned int x = n * 32;
	while (x && test(x - 1) == 0) {
		x -= 1;
	}
	return x;
}

int vlong_value::cf(vlong_value &x) const {
	if (n > x.n) {
		return +1;
	}
	if (n < x.n) {
		return -1;
	}
	unsigned int i = n;
	while (i) {
		i -= 1;
		if (get(i) > x.get(i)) {
			return +1;
		}
		if (get(i) < x.get(i)) {
			return -1;
		}
	}
	return 0;
}

void vlong_value::shl() {
	unsigned int carry = 0;
	unsigned int N = n;
	for (unsigned int i = 0; i <= N; i += 1) {
		unsigned int u = get(i);
		set(i, (u << 1) + carry);
		carry = u >> 31;
	}
}

void vlong_value::shr() {
	unsigned int carry = 0;
	unsigned int i = n;
	while (i) {
		i -= 1;
		unsigned int u = get(i);
		set(i, (u >> 1) + carry);
		carry = u << 31;
	}
}

void vlong_value::shr(unsigned int x) {
	unsigned int delta = x / 32;
	x %= 32;
	for (unsigned int i = 0; i < n; i += 1) {
		unsigned int u = get(i + delta);
		if (x) {
			u >>= x;
			u += get(i + delta + 1) << (32 - x);
		}
		set(i, u);
	}
}

void vlong_value::add(vlong_value &x) {
	unsigned int carry = 0;
	unsigned int max = n;
	if (max < x.n) {
		max = x.n;
	}
	reserve(max);
	for (unsigned int i = 0; i < max + 1; i += 1) {
		unsigned int u = get(i);
		u = u + carry;
		carry = (u < carry);
		unsigned int ux = x.get(i);
		u = u + ux;
		carry += (u < ux);
		set(i, u);
	}
}

void vlong_value::subtract(vlong_value &x) {
	unsigned int carry = 0;
	unsigned int N = n;
	for (unsigned int i = 0; i < N; i += 1) {
		unsigned int ux = x.get(i);
		ux += carry;
		// ux wrapped to 0 only with carry == 1: the unit and the carry stay.
		if (ux >= carry) {
			unsigned int u = get(i);
			unsigned int nu = u - ux;
			carry = nu > u;
			set(i, nu);
		}
	}
}

void vlong_value::init(unsigned int x) {
	clear();
	set(0, x);
}

void vlong_value::copy(vlong_value &x) {
	clear();
	unsigned int i = x.n;
	while (i) {
		i -= 1;
		set(i, x.get(i));
	}
}

vlong_value::vlong_value() {
	share = 0;
}

void vlong_value::mul(vlong_value &x, vlong_value &y) {
	fast_mul(x, y, x.bits() + y.bits());
}

void vlong_value::divide(vlong_value &x, vlong_value &y, vlong_value &rem) {
	init(0);
	rem.copy(x);
	vlong_value m, s;
	m.copy(y);
	s.init(1);
	while (rem.cf(m) > 0) {
		m.shl();
		s.shl();
	}
	while (rem.cf(y) >= 0) {
		while (rem.cf(m) < 0) {
			m.shr();
			s.shr();
		}
		rem.subtract(m);
		add(s);
	}
}

vlong::vlong(unsigned int x) {
	value = new vlong_value;
	negative = 0;
	value->init(x);
}

vlong::vlong(const vlong &x) {
	negative = x.negative;
	value = x.value;
	value->share += 1;
}

vlong::~vlong() {
	if (value->share) {
		value->share -= 1;
	} else {
		delete value;
	}
}

void vlong::docopy() {
	if (value->share) {
		value->share -= 1;
		vlong_value *nv = new vlong_value;
		nv->copy(*value);
		value = nv;
	}
}

int vlong::cf(const vlong x) const {
	int neg = negative && !value->is_zero();
	if (neg == (x.negative && !x.value->is_zero())) {
		return value->cf(*x.value);
	} else if (neg) {
		return -1;
	} else {
		return +1;
	}
}

vlong &vlong::operator=(const vlong &x) {
	if (value->share) {
		value->share -= 1;
	} else {
		delete value;
	}
	value = x.value;
	value->share += 1;
	negative = x.negative;
	return *this;
}

vlong::operator unsigned int() {
	return *value;
}

vlong &vlong::operator+=(const vlong &x) {
	if (negative == x.negative) {
		docopy();
		value->add(*x.value);
	} else if (value->cf(*x.value) >= 0) {
		docopy();
		value->subtract(*x.value);
	} else {
		vlong tmp = *this;
		*this = x;
		*this += tmp;
	}
	return *this;
}

vlong &vlong::operator-=(const vlong &x) {
	if (negative != x.negative) {
		docopy();
		value->add(*x.value);
	} else if (value->cf(*x.value) >= 0) {
		docopy();
		value->subtract(*x.value);
	} else {
		vlong tmp = *this;
		*this = x;
		*this -= tmp;
		negative = 1 - negative;
	}
	return *this;
}

vlong operator+(const vlong &x, const vlong &y) {
	vlong result = x;
	result += y;
	return result;
}

vlong operator-(const vlong &x, const vlong &y) {
	vlong result = x;
	result -= y;
	return result;
}

vlong operator*(const vlong &x, const vlong &y) {
	vlong result;
	result.value->mul(*x.value, *y.value);
	result.negative = x.negative ^ y.negative;
	return result;
}

vlong operator/(const vlong &x, const vlong &y) {
	vlong result;
	vlong_value rem;
	result.value->divide(*x.value, *y.value, rem);
	result.negative = x.negative ^ y.negative;
	return result;
}

vlong operator%(const vlong &x, const vlong &y) {
	vlong result;
	vlong_value divide;
	divide.divide(*x.value, *y.value, *result.value);
	result.negative = x.negative;
	return result;
}

vlong vlong::modexp(const vlong &x, const vlong &e, const vlong &m) {
	vlong_montgomery me(m);
	return me.exp(x, e);
}

vlong vlong::gcd(const vlong &X, const vlong &Y) {
	vlong x = X, y = Y;
	while (true) {
		if (y == 0) {
			return x;
		}
		x = x % y;
		if (x == 0) {
			return y;
		}
		y = y % x;
	}
}

vlong vlong::modinv(const vlong &a, const vlong &m) {
	vlong j = 1, i = 0, b = m, c = a, x, y;
	while (c != 0) {
		x = b / c;
		y = b - x * c;
		b = c;
		c = y;
		y = j;
		j = i - j * x;
		i = y;
	}
	if (i < 0) {
		i += m;
	}
	return i;
}

void vlong::convert(const char *pDecimal, vlong &Number) {
	if (pDecimal == nullptr) {
		throw "pDecimal pointer is NULL";
	}

	// Buffer[1] is 1, not a terminator. atoi() stops at it all the same.
	char Buffer[2] = {0, 1};
	Number = 0;
	while (*pDecimal != 0) {
		Buffer[0] = *pDecimal;
		int Digit = atoi(Buffer);
		if (Digit < 0 || Digit > 9) {
			throw "digit out of range";
		}

		Number = Number * 10 + Digit;
		pDecimal++;
	}
}

void vlong::convert(vlong Number, char *pDecimal, unsigned short DecimalLength) {
	if (pDecimal == nullptr) {
		throw "pDecimal pointer is NULL";
	}

	if (DecimalLength == 0) {
		throw "DecimalLength = 0";
	}

	memset(pDecimal, 0, DecimalLength);
	if (Number == 0) {
		pDecimal[0] = '0';
	} else {
		char Buffer[2] = {0, 1};
		int Digit;
		int i = 0;
		while (Number > 0) {
			if (i >= DecimalLength) {
				throw "large number does not fit in char array (DecimalLength too small)";
			}

			Digit = static_cast<unsigned int>(Number % 10);
			Number = Number / 10;
			sprintf(Buffer, "%d", Digit);
			pDecimal[i] = Buffer[0];
			i++;
		}

		// The digits are in reverse order here. The original loop that
		// turns them around changes neither j nor i, so it never ends
		// for a number with two or more digits.
		int j = 0;
		i--;
		while (j < i) {
			Buffer[0] = pDecimal[j];
			pDecimal[j] = pDecimal[i];
			pDecimal[i] = Buffer[0];
		}
	}
}

void vlong::convert(const unsigned char *pData, unsigned short DataLength, vlong &Number) {
	if (pData == nullptr) {
		throw "pData pointer is NULL";
	}

	// No docopy(): a value that is shared with other vlongs changes for all of them.
	Number.value->clear();

	int i;
	int j;
	int Size = (DataLength + 3) / 4;
	for (i = 0; i < DataLength; i += 4) {
		unsigned int x = 0;
		for (j = 0; j < 4; j++) {
			if (i + j < DataLength) {
				x += static_cast<unsigned int>(pData[i + j]) << ((3 - j) * 8);
			}
		}

		Number.value->set(Size - i / 4 - 1, x);
	}
}

void vlong::convert(vlong Number, unsigned char *pData, unsigned short DataLength) {
	if (pData == nullptr) {
		throw "pData pointer is NULL";
	}

	memset(pData, 0, DataLength);

	int i;
	int j;
	int Size = (DataLength + 3) / 4;
	for (i = 0; i < DataLength; i += 4) {
		unsigned int x = Number.value->get(Size - i / 4 - 1);
		for (j = 0; j < 4; j++) {
			if (i + j < DataLength) {
				pData[i + j] = static_cast<unsigned char>(x >> ((3 - j) * 8));
			}
		}
	}
}

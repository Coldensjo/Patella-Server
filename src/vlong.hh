#ifndef PATELLA_VLONG_HH
#define PATELLA_VLONG_HH

#include <cstdlib>

struct vlong_flex_unit {
	unsigned int n;
private:
	unsigned int *a;
	unsigned int z;
public:
	vlong_flex_unit();
	~vlong_flex_unit();
	void clear();
	unsigned int get(unsigned int i) const;
	void set(unsigned int i, unsigned int x);
	void reserve(unsigned int x);
	void fast_mul(vlong_flex_unit &x, vlong_flex_unit &y, unsigned int keep);
};

struct vlong_value : public vlong_flex_unit {
	unsigned int share;
	int is_zero() const;
	int test(unsigned int i) const;
	unsigned int bits() const;
	int cf(vlong_value &x) const;
	void shl();
	void shr();
	void shr(unsigned int x);
	void add(vlong_value &x);
	void subtract(vlong_value &x);
	void init(unsigned int x);
	void copy(vlong_value &x);
	operator unsigned int();
	vlong_value();
	void mul(vlong_value &x, vlong_value &y);
	void divide(vlong_value &x, vlong_value &y, vlong_value &rem);
};

struct vlong {
	vlong(unsigned int x = 0);
	vlong(const vlong &x);
	virtual ~vlong();
	vlong &operator=(const vlong &x);
	vlong &operator+=(const vlong &x);
	vlong &operator-=(const vlong &x);
	operator unsigned int();
	static vlong modexp(const vlong &x, const vlong &e, const vlong &m);
	static vlong gcd(const vlong &X, const vlong &Y);
	static vlong modinv(const vlong &a, const vlong &m);
	static void convert(const char *pDecimal, vlong &Number);
	static void convert(vlong Number, char *pDecimal, unsigned short DecimalLength);
	static void convert(const unsigned char *pData, unsigned short DataLength, vlong &Number);
	static void convert(vlong Number, unsigned char *pData, unsigned short DataLength);
private:
	int cf(const vlong x) const;
	void docopy();
	vlong_value *value;
	int negative;
};

// inline  vlong.h:123
inline vlong operator+(const vlong & /*x*/, const int & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:125
inline vlong operator-(const vlong & /*x*/, const int & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:127
inline vlong operator*(const vlong & /*x*/, const int & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:129
inline vlong operator/(const vlong & /*x*/, const int & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:131
inline vlong operator%(const vlong & /*x*/, const int & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:136
inline int operator!=(const vlong & /*x*/, const int & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:138
inline int operator==(const vlong & /*x*/, const int & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:139
inline int operator>=(const vlong & /*x*/, const vlong & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:144
inline int operator>(const vlong & /*x*/, const int & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:145
inline int operator<(const vlong & /*x*/, const vlong & /*y*/) { abort(); /* STUB */ }

// inline  vlong.h:146
inline int operator<(const vlong & /*x*/, const int & /*y*/) { abort(); /* STUB */ }

struct vlong_montgomery {
private:
	vlong R;
	vlong R1;
	vlong m;
	vlong n1;
	vlong T;
	vlong k;
	unsigned int N;
	void mul(vlong &x, const vlong &y);
public:
	vlong exp(const vlong &x, const vlong &e);
	vlong_montgomery(const vlong &M);
};

vlong operator+(const vlong &x, const vlong &y);
vlong operator-(const vlong &x, const vlong &y);
vlong operator*(const vlong &x, const vlong &y);
vlong operator/(const vlong &x, const vlong &y);
vlong operator%(const vlong &x, const vlong &y);

#endif

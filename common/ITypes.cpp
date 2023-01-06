#include "ITypes.h"

Bitstring::Bitstring(const UInt32 inLength) : data(nullptr), length() { Alloc(inLength); }

void Bitstring::Alloc(UInt32 inLength) {
	Dispose();

	inLength = (inLength + 7) & ~7;
	length = inLength >> 3;

	data = new UInt8[length];
}

void Bitstring::Dispose() const {
	delete [] data;
}

void Bitstring::Clear() const { std::memset(data, 0, length); }

void Bitstring::Clear(const UInt32 idx) const {
	ASSERT_STR(idx < (length << 3), "Bitstring::Clear: out of range");
	data[idx >> 3] &= ~(1 << (idx & 7));
}

void Bitstring::Set(const UInt32 idx) const {
	ASSERT_STR(idx < (length << 3), "Bitstring::Set: out of range");
	data[idx >> 3] |= (1 << (idx & 7));
}

bool Bitstring::IsSet(const UInt32 idx) const {
	ASSERT_STR(idx < (length << 3), "Bitstring::IsSet: out of range");
	return (data[idx >> 3] & (1 << (idx & 7))) ? true : false;
}

bool Bitstring::IsClear(const UInt32 idx) const {
	ASSERT_STR(idx < (length << 3), "Bitstring::IsClear: out of range");
	return (data[idx >> 3] & (1 << (idx & 7))) ? false : true;
}

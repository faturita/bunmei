#ifndef MD5_H
#define MD5_H

#include <cstddef>
#include <string>

// RFC 1321 MD5, self-contained (no dependency pulled in for it).
//
// Used ONLY as an integrity check on a savegame payload -- to catch a truncated, half-written
// or edited file before the loaders walk off the end of it. MD5 is broken for anything
// adversarial and this is not the place to rely on it for that.

#define MD5_DIGEST_SIZE 16

// Raw 16-byte digest of `len` bytes at `data`.
void md5(const void* data, size_t len, unsigned char digest[MD5_DIGEST_SIZE]);

// The same digest as 32 lowercase hex characters, for messages and logs.
std::string md5hex(const void* data, size_t len);

#endif // MD5_H

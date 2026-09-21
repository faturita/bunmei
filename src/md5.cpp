#include <cstdint>
#include <cstdio>
#include <cstring>

#include "md5.h"

// RFC 1321, straight off the specification. Verified against the RFC's own test vectors in
// testcase_068 -- a hash that is subtly wrong would still "work" (it is only ever compared
// against itself), so the known-answer tests are what keep this honest.

namespace {

// K[i] = floor(|sin(i+1)| * 2^32). Tabulated rather than computed with sin() at startup:
// libm is not required to be bit-identical across platforms, and a digest must be.
const uint32_t K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

// Per-round left-rotation amounts.
const int S[64] = {
    7,12,17,22,  7,12,17,22,  7,12,17,22,  7,12,17,22,
    5, 9,14,20,  5, 9,14,20,  5, 9,14,20,  5, 9,14,20,
    4,11,16,23,  4,11,16,23,  4,11,16,23,  4,11,16,23,
    6,10,15,21,  6,10,15,21,  6,10,15,21,  6,10,15,21
};

inline uint32_t rotl(uint32_t x, int c)
{
    return (uint32_t)((x << c) | (x >> (32 - c)));
}

// Little-endian load, done byte by byte rather than by casting the buffer to uint32_t*:
// that would both assume the host's endianness and read through a misaligned pointer.
inline uint32_t load32(const unsigned char* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

inline void store32(unsigned char* p, uint32_t v)
{
    p[0] = (unsigned char)(v & 0xff);
    p[1] = (unsigned char)((v >> 8) & 0xff);
    p[2] = (unsigned char)((v >> 16) & 0xff);
    p[3] = (unsigned char)((v >> 24) & 0xff);
}

void transform(uint32_t state[4], const unsigned char block[64])
{
    uint32_t M[16];
    for (int i = 0; i < 16; i++)
        M[i] = load32(block + i*4);

    uint32_t A = state[0], B = state[1], C = state[2], D = state[3];

    for (int i = 0; i < 64; i++)
    {
        uint32_t F;
        int g;

        if (i < 16)      { F = (B & C) | (~B & D);          g = i; }
        else if (i < 32) { F = (D & B) | (~D & C);          g = (5*i + 1) % 16; }
        else if (i < 48) { F = B ^ C ^ D;                   g = (3*i + 5) % 16; }
        else             { F = C ^ (B | ~D);                g = (7*i) % 16; }

        F = F + A + K[i] + M[g];
        A = D;
        D = C;
        C = B;
        B = B + rotl(F, S[i]);
    }

    state[0] += A;
    state[1] += B;
    state[2] += C;
    state[3] += D;
}

} // namespace

void md5(const void* data, size_t len, unsigned char digest[MD5_DIGEST_SIZE])
{
    uint32_t state[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };

    const unsigned char* p = (const unsigned char*)data;

    size_t whole = len / 64;
    for (size_t i = 0; i < whole; i++)
        transform(state, p + i*64);

    // Tail: the remaining bytes, a 0x80 terminator, zero padding out to 56 mod 64, then the
    // message length in BITS as a little-endian 64-bit value. Two blocks are needed when the
    // remainder leaves no room for the length.
    unsigned char tail[128];
    memset(tail, 0, sizeof(tail));

    size_t rem = len - whole*64;
    memcpy(tail, p + whole*64, rem);
    tail[rem] = 0x80;

    size_t tailBlocks = (rem + 1 + 8 > 64) ? 2 : 1;
    uint64_t bits = (uint64_t)len * 8ull;
    store32(tail + tailBlocks*64 - 8, (uint32_t)(bits & 0xffffffffull));
    store32(tail + tailBlocks*64 - 4, (uint32_t)((bits >> 32) & 0xffffffffull));

    for (size_t i = 0; i < tailBlocks; i++)
        transform(state, tail + i*64);

    for (int i = 0; i < 4; i++)
        store32(digest + i*4, state[i]);
}

std::string md5hex(const void* data, size_t len)
{
    unsigned char digest[MD5_DIGEST_SIZE];
    md5(data, len, digest);

    char out[MD5_DIGEST_SIZE*2 + 1];
    for (int i = 0; i < MD5_DIGEST_SIZE; i++)
        snprintf(out + i*2, 3, "%02x", digest[i]);

    return std::string(out, MD5_DIGEST_SIZE*2);
}

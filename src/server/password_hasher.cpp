#include "server/password_hasher.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace kfc {

namespace {

constexpr int kSaltBytes = 16;
constexpr int kHashBytes = 32;

struct Sha256Context {
    std::array<std::uint32_t, 8> state{};
    std::uint64_t bit_count = 0;
    std::array<std::uint8_t, 64> buffer{};
    std::size_t buffer_size = 0;
};

constexpr std::array<std::uint32_t, 64> kSha256Constants = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
    0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
    0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
    0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
    0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2};

std::uint32_t rotr(std::uint32_t value, std::uint32_t bits) {
    return (value >> bits) | (value << (32U - bits));
}

void sha256_transform(Sha256Context& context, const std::uint8_t block[64]) {
    std::array<std::uint32_t, 64> words{};
    for (std::size_t i = 0; i < 16; ++i) {
        words[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24) |
                   (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
                   static_cast<std::uint32_t>(block[i * 4 + 3]);
    }
    for (std::size_t i = 16; i < 64; ++i) {
        const std::uint32_t s0 = rotr(words[i - 15], 7) ^ rotr(words[i - 15], 18) ^
                                 (words[i - 15] >> 3);
        const std::uint32_t s1 = rotr(words[i - 2], 17) ^ rotr(words[i - 2], 19) ^
                                 (words[i - 2] >> 10);
        words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }

    std::uint32_t a = context.state[0];
    std::uint32_t b = context.state[1];
    std::uint32_t c = context.state[2];
    std::uint32_t d = context.state[3];
    std::uint32_t e = context.state[4];
    std::uint32_t f = context.state[5];
    std::uint32_t g = context.state[6];
    std::uint32_t h = context.state[7];

    for (std::size_t i = 0; i < 64; ++i) {
        const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const std::uint32_t ch = (e & f) ^ ((~e) & g);
        const std::uint32_t temp1 = h + s1 + ch + kSha256Constants[i] + words[i];
        const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temp2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    context.state[0] += a;
    context.state[1] += b;
    context.state[2] += c;
    context.state[3] += d;
    context.state[4] += e;
    context.state[5] += f;
    context.state[6] += g;
    context.state[7] += h;
}

void sha256_init(Sha256Context& context) {
    context.state = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    context.bit_count = 0;
    context.buffer_size = 0;
}

void sha256_update(Sha256Context& context, const std::uint8_t* data, std::size_t length) {
    for (std::size_t i = 0; i < length; ++i) {
        context.buffer[context.buffer_size++] = data[i];
        if (context.buffer_size == 64) {
            sha256_transform(context, context.buffer.data());
            context.bit_count += 512;
            context.buffer_size = 0;
        }
    }
}

void sha256_final(Sha256Context& context, std::array<std::uint8_t, 32>& digest) {
    context.bit_count += context.buffer_size * 8;

    context.buffer[context.buffer_size++] = 0x80;
    if (context.buffer_size > 56) {
        while (context.buffer_size < 64) {
            context.buffer[context.buffer_size++] = 0;
        }
        sha256_transform(context, context.buffer.data());
        context.buffer_size = 0;
    }

    while (context.buffer_size < 56) {
        context.buffer[context.buffer_size++] = 0;
    }

    for (int i = 7; i >= 0; --i) {
        context.buffer[context.buffer_size++] = static_cast<std::uint8_t>((context.bit_count >> (i * 8)) & 0xff);
    }
    sha256_transform(context, context.buffer.data());

    for (std::size_t i = 0; i < 8; ++i) {
        digest[i * 4] = static_cast<std::uint8_t>((context.state[i] >> 24) & 0xff);
        digest[i * 4 + 1] = static_cast<std::uint8_t>((context.state[i] >> 16) & 0xff);
        digest[i * 4 + 2] = static_cast<std::uint8_t>((context.state[i] >> 8) & 0xff);
        digest[i * 4 + 3] = static_cast<std::uint8_t>(context.state[i] & 0xff);
    }
}

std::array<std::uint8_t, 32> sha256(const std::uint8_t* data, std::size_t length) {
    Sha256Context context;
    sha256_init(context);
    sha256_update(context, data, length);
    std::array<std::uint8_t, 32> digest{};
    sha256_final(context, digest);
    return digest;
}

std::array<std::uint8_t, 32> hmac_sha256(const std::uint8_t* key, std::size_t key_len,
                                           const std::uint8_t* data, std::size_t data_len) {
    std::array<std::uint8_t, 64> key_block{};
    if (key_len > 64) {
        const auto hashed_key = sha256(key, key_len);
        for (std::size_t i = 0; i < 32; ++i) {
            key_block[i] = hashed_key[i];
        }
    } else {
        for (std::size_t i = 0; i < key_len; ++i) {
            key_block[i] = key[i];
        }
    }

    std::array<std::uint8_t, 64> inner_pad{};
    std::array<std::uint8_t, 64> outer_pad{};
    for (std::size_t i = 0; i < 64; ++i) {
        inner_pad[i] = static_cast<std::uint8_t>(key_block[i] ^ 0x36);
        outer_pad[i] = static_cast<std::uint8_t>(key_block[i] ^ 0x5c);
    }

    Sha256Context inner_context;
    sha256_init(inner_context);
    sha256_update(inner_context, inner_pad.data(), inner_pad.size());
    sha256_update(inner_context, data, data_len);
    std::array<std::uint8_t, 32> inner_digest{};
    sha256_final(inner_context, inner_digest);

    Sha256Context outer_context;
    sha256_init(outer_context);
    sha256_update(outer_context, outer_pad.data(), outer_pad.size());
    sha256_update(outer_context, inner_digest.data(), inner_digest.size());
    std::array<std::uint8_t, 32> outer_digest{};
    sha256_final(outer_context, outer_digest);
    return outer_digest;
}

std::vector<std::uint8_t> pbkdf2_sha256(const std::string& password, const std::uint8_t* salt,
                                          std::size_t salt_len, int iterations, std::size_t dk_len) {
    const int blocks = static_cast<int>((dk_len + 31) / 32);
    std::vector<std::uint8_t> derived(dk_len);
    std::size_t offset = 0;

    for (int block = 1; block <= blocks; ++block) {
        std::array<std::uint8_t, 4> block_index = {
            static_cast<std::uint8_t>((block >> 24) & 0xff),
            static_cast<std::uint8_t>((block >> 16) & 0xff),
            static_cast<std::uint8_t>((block >> 8) & 0xff),
            static_cast<std::uint8_t>(block & 0xff),
        };

        std::vector<std::uint8_t> message(salt_len + 4);
        for (std::size_t i = 0; i < salt_len; ++i) {
            message[i] = salt[i];
        }
        for (std::size_t i = 0; i < 4; ++i) {
            message[salt_len + i] = block_index[i];
        }

        auto u = hmac_sha256(reinterpret_cast<const std::uint8_t*>(password.data()),
                             password.size(), message.data(), message.size());
        std::array<std::uint8_t, 32> block_result = u;

        for (int iter = 1; iter < iterations; ++iter) {
            u = hmac_sha256(reinterpret_cast<const std::uint8_t*>(password.data()), password.size(),
                            u.data(), u.size());
            for (std::size_t i = 0; i < 32; ++i) {
                block_result[i] ^= u[i];
            }
        }

        const std::size_t copy_len = std::min<std::size_t>(32, dk_len - offset);
        for (std::size_t i = 0; i < copy_len; ++i) {
            derived[offset + i] = block_result[i];
        }
        offset += copy_len;
    }

    return derived;
}

std::string to_hex(const std::uint8_t* data, std::size_t length) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < length; ++i) {
        stream << std::setw(2) << static_cast<int>(data[i]);
    }
    return stream.str();
}

bool from_hex(const std::string& hex, std::vector<std::uint8_t>& out) {
    if (hex.size() % 2 != 0) {
        return false;
    }

    out.clear();
    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const std::string byte = hex.substr(i, 2);
        try {
            out.push_back(static_cast<std::uint8_t>(std::stoi(byte, nullptr, 16)));
        } catch (const std::exception&) {
            return false;
        }
    }
    return true;
}

bool constant_time_equals(const std::string& left, const std::string& right) {
    if (left.size() != right.size()) {
        return false;
    }

    unsigned char result = 0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        result |= static_cast<unsigned char>(left[i] ^ right[i]);
    }
    return result == 0;
}

std::array<std::uint8_t, kSaltBytes> random_salt() {
    std::array<std::uint8_t, kSaltBytes> salt{};
    std::random_device device;
    for (auto& byte : salt) {
        byte = static_cast<std::uint8_t>(device());
    }
    return salt;
}

}  // namespace

std::string PasswordHasher::hash_password(const std::string& password, int iterations) {
    const auto salt = random_salt();
    const auto derived =
        pbkdf2_sha256(password, salt.data(), salt.size(), iterations, kHashBytes);

    return "pbkdf2_sha256$" + std::to_string(iterations) + "$" + to_hex(salt.data(), salt.size()) +
           "$" + to_hex(derived.data(), derived.size());
}

bool PasswordHasher::verify_password(const std::string& password, const std::string& stored_hash) {
    const std::size_t first = stored_hash.find('$');
    const std::size_t second = stored_hash.find('$', first + 1);
    const std::size_t third = stored_hash.find('$', second + 1);
    if (first == std::string::npos || second == std::string::npos || third == std::string::npos) {
        return false;
    }

    if (stored_hash.substr(0, first) != "pbkdf2_sha256") {
        return false;
    }

    int iterations = 0;
    try {
        iterations = std::stoi(stored_hash.substr(first + 1, second - first - 1));
    } catch (const std::exception&) {
        return false;
    }

    std::vector<std::uint8_t> salt;
    if (!from_hex(stored_hash.substr(second + 1, third - second - 1), salt)) {
        return false;
    }

    std::vector<std::uint8_t> expected_hash;
    if (!from_hex(stored_hash.substr(third + 1), expected_hash)) {
        return false;
    }

    const auto derived =
        pbkdf2_sha256(password, salt.data(), salt.size(), iterations, expected_hash.size());
    return constant_time_equals(to_hex(derived.data(), derived.size()),
                                stored_hash.substr(third + 1));
}

}  // namespace kfc

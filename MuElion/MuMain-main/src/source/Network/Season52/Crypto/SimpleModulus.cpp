#include "SimpleModulus.hpp"

#include <algorithm>
#include <cstring>

namespace mu::net::s52 {
namespace {

constexpr std::array<std::uint32_t, 4> kSaveLoadXor{
    0x3F08A79B, 0xE25CC287, 0x93D27AB9, 0x20DEA7BF
};

std::uint16_t ReadU16LE(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0])
         | (static_cast<std::uint16_t>(p[1]) << 8u);
}

std::uint32_t ReadU32LE(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0])
         | (static_cast<std::uint32_t>(p[1]) << 8u)
         | (static_cast<std::uint32_t>(p[2]) << 16u)
         | (static_cast<std::uint32_t>(p[3]) << 24u);
}

void WriteU16LE(std::uint8_t* p, std::uint16_t value) noexcept {
    p[0] = static_cast<std::uint8_t>(value & 0xFFu);
    p[1] = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
}

void WriteU32LE(std::uint8_t* p, std::uint32_t value) noexcept {
    p[0] = static_cast<std::uint8_t>(value & 0xFFu);
    p[1] = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
    p[2] = static_cast<std::uint8_t>((value >> 16u) & 0xFFu);
    p[3] = static_cast<std::uint8_t>((value >> 24u) & 0xFFu);
}

} // namespace

bool SimpleModulusKey::IsValid() const noexcept {
    return std::all_of(modulus.begin(), modulus.end(),
                       [](std::uint32_t value) { return value != 0; })
        && std::all_of(key.begin(), key.end(),
                       [](std::uint32_t value) { return value != 0; });
}

bool SimpleModulus::LoadKeyFile(const std::uint8_t* bytes,
                                std::size_t size,
                                SimpleModulusKey& out) noexcept {
    if (bytes == nullptr || size != KeyFileSize) {
        return false;
    }

    constexpr std::uint16_t chunkId = 0x1112;
    if (ReadU16LE(bytes) != chunkId || ReadU32LE(bytes + 2) != KeyFileSize) {
        return false;
    }

    SimpleModulusKey parsed{};
    std::size_t offset = 6;

    auto readTable = [&](std::array<std::uint32_t, 4>& target) {
        for (std::size_t i = 0; i < target.size(); ++i) {
            target[i] = ReadU32LE(bytes + offset) ^ kSaveLoadXor[i];
            offset += 4;
        }
    };

    readTable(parsed.modulus);
    readTable(parsed.key);
    readTable(parsed.xorKey);

    if (!parsed.IsValid()) {
        return false;
    }

    out = parsed;
    return true;
}

std::size_t SimpleModulus::EncryptedSize(std::size_t plainSize) noexcept {
    if (plainSize == 0) {
        return 0;
    }
    const auto blocks = (plainSize + PlainBlockSize - 1) / PlainBlockSize;
    return blocks * EncryptedBlockSize;
}

bool SimpleModulus::Encrypt(const SimpleModulusKey& key,
                            const std::uint8_t* plain,
                            std::size_t plainSize,
                            std::vector<std::uint8_t>& encrypted) {
    encrypted.clear();
    if (!key.IsValid() || plain == nullptr || plainSize == 0) {
        return false;
    }

    encrypted.resize(EncryptedSize(plainSize));
    std::size_t sourceOffset = 0;
    std::size_t targetOffset = 0;

    while (sourceOffset < plainSize) {
        const auto remaining = plainSize - sourceOffset;
        const auto blockSize = std::min(remaining, PlainBlockSize);
        if (!EncryptBlock(key,
                          plain + sourceOffset,
                          blockSize,
                          encrypted.data() + targetOffset)) {
            encrypted.clear();
            return false;
        }

        sourceOffset += blockSize;
        targetOffset += EncryptedBlockSize;
    }

    return true;
}

bool SimpleModulus::Decrypt(const SimpleModulusKey& key,
                            const std::uint8_t* encrypted,
                            std::size_t encryptedSize,
                            std::vector<std::uint8_t>& plain) {
    plain.clear();
    if (!key.IsValid() || encrypted == nullptr || encryptedSize == 0
        || (encryptedSize % EncryptedBlockSize) != 0) {
        return false;
    }

    plain.reserve((encryptedSize / EncryptedBlockSize) * PlainBlockSize);

    for (std::size_t offset = 0; offset < encryptedSize; offset += EncryptedBlockSize) {
        std::array<std::uint8_t, PlainBlockSize> block{};
        const int decoded = DecryptBlock(key, encrypted + offset, block.data());
        if (decoded < 0 || decoded > static_cast<int>(PlainBlockSize)) {
            plain.clear();
            return false;
        }
        plain.insert(plain.end(), block.begin(), block.begin() + decoded);
    }

    return true;
}

bool SimpleModulus::EncryptBlock(const SimpleModulusKey& key,
                                 const std::uint8_t* source,
                                 std::size_t sourceSize,
                                 std::uint8_t* target) {
    if (source == nullptr || target == nullptr || sourceSize == 0
        || sourceSize > PlainBlockSize) {
        return false;
    }

    std::array<std::uint8_t, PlainBlockSize> padded{};
    std::copy(source, source + sourceSize, padded.begin());

    std::array<std::uint32_t, 4> encoded{};
    std::uint32_t previous = 0;

    for (std::size_t i = 0; i < encoded.size(); ++i) {
        const auto word = ReadU16LE(padded.data() + (i * 2));
        encoded[i] = ((((key.xorKey[i] ^ word) ^ previous) * key.key[i])
                      % key.modulus[i]);
        previous = static_cast<std::uint16_t>(encoded[i]);
    }

    for (std::size_t i = 0; i < 3; ++i) {
        encoded[i] = (encoded[i] ^ key.xorKey[i])
                   ^ static_cast<std::uint16_t>(encoded[i + 1]);
    }

    std::fill(target, target + EncryptedBlockSize, 0);
    std::size_t bit = 0;

    for (const auto value : encoded) {
        std::array<std::uint8_t, 4> packed{};
        WriteU32LE(packed.data(), value);
        bit = CopyBits(target, bit, packed.data(), 0, 16);
        bit = CopyBits(target, bit, packed.data(), 22, 2);
    }

    std::uint8_t checksum = 0xF8;
    for (const auto value : padded) {
        checksum ^= value;
    }

    std::array<std::uint8_t, 2> trailer{
        static_cast<std::uint8_t>((checksum ^ static_cast<std::uint8_t>(sourceSize)) ^ 0x3D),
        checksum
    };
    CopyBits(target, bit, trailer.data(), 0, 16);
    return true;
}

int SimpleModulus::DecryptBlock(const SimpleModulusKey& key,
                                const std::uint8_t* source,
                                std::uint8_t* target) {
    if (source == nullptr || target == nullptr) {
        return -1;
    }

    std::array<std::uint32_t, 4> decoded{};
    std::size_t bit = 0;

    for (auto& value : decoded) {
        std::array<std::uint8_t, 4> packed{};
        CopyBits(packed.data(), 0, source, bit, 16);
        bit += 16;
        CopyBits(packed.data(), 22, source, bit, 2);
        bit += 2;
        value = ReadU32LE(packed.data());
    }

    for (int i = 2; i >= 0; --i) {
        decoded[static_cast<std::size_t>(i)] =
            (decoded[static_cast<std::size_t>(i)] ^ key.xorKey[static_cast<std::size_t>(i)])
            ^ static_cast<std::uint16_t>(decoded[static_cast<std::size_t>(i) + 1]);
    }

    std::fill(target, target + PlainBlockSize, 0);
    std::uint32_t previous = 0;

    for (std::size_t i = 0; i < decoded.size(); ++i) {
        const auto word = static_cast<std::uint16_t>(
            ((((key.key[i] * decoded[i]) % key.modulus[i]) ^ key.xorKey[i])
             ^ previous));
        WriteU16LE(target + (i * 2), word);
        previous = static_cast<std::uint16_t>(decoded[i]);
    }

    std::array<std::uint8_t, 2> trailer{};
    CopyBits(trailer.data(), 0, source, bit, 16);

    const auto plainSize = static_cast<std::uint8_t>(
        (trailer[0] ^ trailer[1]) ^ 0x3D);

    std::uint8_t checksum = 0xF8;
    for (std::size_t i = 0; i < PlainBlockSize; ++i) {
        checksum ^= target[i];
    }

    if (checksum != trailer[1] || plainSize > PlainBlockSize) {
        return -1;
    }

    return plainSize;
}

std::size_t SimpleModulus::CopyBits(std::uint8_t* target,
                                    std::size_t targetBit,
                                    const std::uint8_t* source,
                                    std::size_t sourceBit,
                                    std::size_t bitCount) {
    if (target == nullptr || source == nullptr || bitCount == 0) {
        return targetBit;
    }

    const auto sourceEndBit = sourceBit + bitCount;
    const auto sourceFirstByte = sourceBit >> 3u;
    const auto sourceLastByte = (sourceEndBit - 1u) >> 3u;
    const auto tempBytes = (sourceLastByte - sourceFirstByte) + 1u;

    std::vector<std::uint8_t> temp(tempBytes + 1u, 0);
    std::memcpy(temp.data(), source + sourceFirstByte, tempBytes);

    if ((sourceEndBit % 8u) != 0) {
        temp[tempBytes - 1u] &= static_cast<std::uint8_t>(
            0xFFu << (8u - (sourceEndBit % 8u)));
    }

    const int left = static_cast<int>(sourceBit % 8u);
    const int right = static_cast<int>(targetBit % 8u);

    Shift(temp.data(), tempBytes, -left);
    Shift(temp.data(), tempBytes + 1u, right);

    const auto writeBytes = (right <= left ? 0u : 1u) + tempBytes;
    auto* destination = target + (targetBit >> 3u);

    for (std::size_t i = 0; i < writeBytes; ++i) {
        destination[i] |= temp[i];
    }

    return targetBit + bitCount;
}

void SimpleModulus::Shift(std::uint8_t* bytes,
                          std::size_t size,
                          int amount) noexcept {
    if (bytes == nullptr || size == 0 || amount == 0) {
        return;
    }

    if (amount > 0) {
        const auto shift = static_cast<unsigned>(amount);
        for (std::size_t i = size - 1; i > 0; --i) {
            bytes[i] = static_cast<std::uint8_t>(
                (bytes[i - 1] << (8u - shift)) | (bytes[i] >> shift));
        }
        bytes[0] = static_cast<std::uint8_t>(bytes[0] >> shift);
        return;
    }

    const auto shift = static_cast<unsigned>(-amount);
    for (std::size_t i = 0; i + 1 < size; ++i) {
        bytes[i] = static_cast<std::uint8_t>(
            (bytes[i + 1] >> (8u - shift)) | (bytes[i] << shift));
    }
    bytes[size - 1] = static_cast<std::uint8_t>(bytes[size - 1] << shift);
}

} // namespace mu::net::s52

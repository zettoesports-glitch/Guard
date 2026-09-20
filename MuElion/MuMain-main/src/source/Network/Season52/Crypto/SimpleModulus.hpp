#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mu::net::s52 {

struct SimpleModulusKey {
    std::array<std::uint32_t, 4> modulus{};
    std::array<std::uint32_t, 4> key{};
    std::array<std::uint32_t, 4> xorKey{};

    bool IsValid() const noexcept;
};

class SimpleModulus {
public:
    static constexpr std::size_t PlainBlockSize = 8;
    static constexpr std::size_t EncryptedBlockSize = 11;
    static constexpr std::size_t KeyFileSize = 54;

    static bool LoadKeyFile(const std::uint8_t* bytes,
                            std::size_t size,
                            SimpleModulusKey& out) noexcept;

    static std::size_t EncryptedSize(std::size_t plainSize) noexcept;

    static bool Encrypt(const SimpleModulusKey& key,
                        const std::uint8_t* plain,
                        std::size_t plainSize,
                        std::vector<std::uint8_t>& encrypted);

    static bool Decrypt(const SimpleModulusKey& key,
                        const std::uint8_t* encrypted,
                        std::size_t encryptedSize,
                        std::vector<std::uint8_t>& plain);

private:
    static bool EncryptBlock(const SimpleModulusKey& key,
                             const std::uint8_t* source,
                             std::size_t sourceSize,
                             std::uint8_t* target);

    static int DecryptBlock(const SimpleModulusKey& key,
                            const std::uint8_t* source,
                            std::uint8_t* target);

    static std::size_t CopyBits(std::uint8_t* target,
                                std::size_t targetBit,
                                const std::uint8_t* source,
                                std::size_t sourceBit,
                                std::size_t bitCount);

    static void Shift(std::uint8_t* bytes,
                      std::size_t size,
                      int amount) noexcept;
};

} // namespace mu::net::s52

#pragma once

#include "ISeasonCrypto.hpp"
#include "SimpleModulus.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mu::net::s52 {

class Season52Crypto final : public ISeasonCrypto {
public:
    bool LoadClientToServerKey(const std::uint8_t* bytes, std::size_t size) noexcept;
    bool LoadServerToClientKey(const std::uint8_t* bytes, std::size_t size) noexcept;

    void SetClientToServerKey(const SimpleModulusKey& key) noexcept { clientToServer_ = key; }
    void SetServerToClientKey(const SimpleModulusKey& key) noexcept { serverToClient_ = key; }

    void ResetSerial(std::uint8_t value = 0) noexcept { sendSerial_ = value; }
    std::uint8_t LastReceivedSerial() const noexcept { return lastReceivedSerial_; }

    bool Decode(const std::uint8_t* packet,
                std::size_t size,
                std::vector<std::uint8_t>& plain) override;

    bool Encode(const std::uint8_t* packet,
                std::size_t size,
                std::vector<std::uint8_t>& encrypted) override;

    bool EncodeForcedWide(const std::uint8_t* packet,
                          std::size_t size,
                          std::vector<std::uint8_t>& encrypted);

private:
    SimpleModulusKey clientToServer_{};
    SimpleModulusKey serverToClient_{};
    std::uint8_t sendSerial_{0};
    std::uint8_t lastReceivedSerial_{0};
};

} // namespace mu::net::s52

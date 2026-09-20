#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mu::net {

class ISeasonCrypto {
public:
    virtual ~ISeasonCrypto() = default;

    virtual bool Decode(const std::uint8_t* packet,
                        std::size_t size,
                        std::vector<std::uint8_t>& plain) = 0;

    virtual bool Encode(const std::uint8_t* packet,
                        std::size_t size,
                        std::vector<std::uint8_t>& encrypted) = 0;
};

} // namespace mu::net

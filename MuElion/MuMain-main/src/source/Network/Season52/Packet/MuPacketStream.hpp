#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mu::net {

class MuPacketStream {
public:
    bool Feed(const std::uint8_t* data,
              std::size_t size,
              std::vector<std::vector<std::uint8_t>>& packets);

    void Reset() noexcept { buffer_.clear(); }
    std::size_t BufferedBytes() const noexcept { return buffer_.size(); }

private:
    std::vector<std::uint8_t> buffer_;
};

} // namespace mu::net

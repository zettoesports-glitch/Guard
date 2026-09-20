#include "MuPacketStream.hpp"

namespace mu::net {
namespace {

bool TryPacketSize(const std::vector<std::uint8_t>& buffer,
                   std::size_t& packetSize) noexcept {
    packetSize = 0;
    if (buffer.size() < 2) {
        return true;
    }

    switch (buffer[0]) {
        case 0xC1:
        case 0xC3:
            packetSize = buffer[1];
            return packetSize >= 3;

        case 0xC2:
        case 0xC4:
            if (buffer.size() < 3) {
                return true;
            }
            packetSize = (static_cast<std::size_t>(buffer[1]) << 8u)
                       | static_cast<std::size_t>(buffer[2]);
            return packetSize >= 4;

        default:
            return false;
    }
}

} // namespace

bool MuPacketStream::Feed(const std::uint8_t* data,
                          std::size_t size,
                          std::vector<std::vector<std::uint8_t>>& packets) {
    packets.clear();
    if (data == nullptr && size != 0) {
        return false;
    }

    if (size != 0) {
        buffer_.insert(buffer_.end(), data, data + size);
    }

    while (!buffer_.empty()) {
        std::size_t packetSize = 0;
        if (!TryPacketSize(buffer_, packetSize)) {
            buffer_.clear();
            return false;
        }

        if (packetSize == 0 || buffer_.size() < packetSize) {
            break;
        }

        packets.emplace_back(buffer_.begin(), buffer_.begin() + packetSize);
        buffer_.erase(buffer_.begin(), buffer_.begin() + packetSize);
    }

    return true;
}

} // namespace mu::net

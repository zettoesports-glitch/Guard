#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mu::net::s52 {

inline constexpr std::array<std::uint8_t, 32> kPacketXorFilter{
    0xE7, 0x6D, 0x3A, 0x89, 0xBC, 0xB2, 0x9F, 0x73,
    0x23, 0xA8, 0xFE, 0xB6, 0x49, 0x5D, 0x39, 0x5D,
    0x8A, 0xCB, 0x63, 0x8D, 0xEA, 0x7D, 0x2B, 0x5F,
    0xC3, 0xB1, 0xE9, 0x83, 0x29, 0x51, 0xE8, 0x56
};

inline std::size_t PacketPayloadOffset(std::uint8_t type) noexcept {
    switch (type) {
        case 0xC1:
        case 0xC3:
            return 3;
        case 0xC2:
        case 0xC4:
            return 4;
        default:
            return 0;
    }
}

inline bool EncodePacketXor(std::uint8_t* packet, std::size_t size) noexcept {
    if (packet == nullptr) {
        return false;
    }

    const auto start = PacketPayloadOffset(packet[0]);
    if (start == 0 || size <= start) {
        return start != 0;
    }

    for (std::size_t i = start; i < size; ++i) {
        packet[i] ^= packet[i - 1] ^ kPacketXorFilter[i % kPacketXorFilter.size()];
    }
    return true;
}

inline bool DecodePacketXor(std::uint8_t* packet, std::size_t size) noexcept {
    if (packet == nullptr) {
        return false;
    }

    const auto start = PacketPayloadOffset(packet[0]);
    if (start == 0 || size <= start) {
        return start != 0;
    }

    for (std::size_t i = size - 1; i >= start; --i) {
        packet[i] ^= packet[i - 1] ^ kPacketXorFilter[i % kPacketXorFilter.size()];
        if (i == start) {
            break;
        }
    }
    return true;
}

inline void BuxTransform(std::uint8_t* data, std::size_t size) noexcept {
    static constexpr std::array<std::uint8_t, 3> key{0xFC, 0xCF, 0xAB};
    if (data == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < size; ++i) {
        data[i] ^= key[i % key.size()];
    }
}

} // namespace mu::net::s52

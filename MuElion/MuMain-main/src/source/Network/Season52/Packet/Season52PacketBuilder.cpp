#include "Season52PacketBuilder.hpp"
#include "../Crypto/Season52Crypto.hpp"
#include "../Crypto/Season52Xor.hpp"

#include <algorithm>
#include <array>

namespace mu::net::s52 {
namespace {

void WriteU32LE(std::vector<std::uint8_t>& packet, std::uint32_t value) {
    packet.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    packet.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xFFu));
    packet.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xFFu));
    packet.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xFFu));
}

template <std::size_t N>
std::array<std::uint8_t, N> FixedField(std::string_view value) {
    std::array<std::uint8_t, N> field{};
    const auto count = std::min(value.size(), field.size());
    std::copy_n(reinterpret_cast<const std::uint8_t*>(value.data()), count, field.begin());
    return field;
}

} // namespace

std::vector<std::uint8_t> BuildServerListRequest() {
    return {0xC1, 0x04, 0xF4, 0x06};
}

std::vector<std::uint8_t> BuildServerAddressRequest(std::uint16_t serverId) {
    return {
        0xC1, 0x06, 0xF4, 0x03,
        static_cast<std::uint8_t>(serverId & 0xFFu),
        static_cast<std::uint8_t>((serverId >> 8u) & 0xFFu)
    };
}

std::vector<std::uint8_t> BuildCharacterListRequest(std::uint8_t language) {
    std::vector<std::uint8_t> packet{0xC1, 0x05, 0xF3, 0x00, language};
    EncodePacketXor(packet.data(), packet.size());
    return packet;
}

bool BuildLoginRequest(std::string_view account,
                       std::string_view password,
                       std::uint32_t tickCount,
                       const std::array<std::uint8_t, ProtocolVersionSize>& version,
                       const std::array<std::uint8_t, ProtocolSerialSize>& serial,
                       Season52Crypto& crypto,
                       std::vector<std::uint8_t>& wirePacket) {
    auto accountField = FixedField<AccountSize>(account);
    auto passwordField = FixedField<PasswordSize>(password);

    BuxTransform(accountField.data(), accountField.size());
    BuxTransform(passwordField.data(), passwordField.size());

    // Canonical pre-encryption packet:
    // C1 size F1 01 account[10] password[20] tick[4] version[5] serial[16]
    std::vector<std::uint8_t> plain;
    plain.reserve(4u + AccountSize + PasswordSize + 4u
                  + ProtocolVersionSize + ProtocolSerialSize);

    plain.push_back(0xC1);
    plain.push_back(0); // patched below
    plain.push_back(0xF1);
    plain.push_back(0x01);

    plain.insert(plain.end(), accountField.begin(), accountField.end());
    plain.insert(plain.end(), passwordField.begin(), passwordField.end());
    WriteU32LE(plain, tickCount);

    // Louis/Webzen sends each protocol version byte decremented by its
    // one-based position.
    for (std::size_t i = 0; i < version.size(); ++i) {
        plain.push_back(static_cast<std::uint8_t>(version[i] - (i + 1u)));
    }

    plain.insert(plain.end(), serial.begin(), serial.end());

    if (plain.size() >= 256u) {
        return false;
    }
    plain[1] = static_cast<std::uint8_t>(plain.size());

    return crypto.Encode(plain.data(), plain.size(), wirePacket);
}

} // namespace mu::net::s52

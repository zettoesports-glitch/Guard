#include "Season52Crypto.hpp"
#include "Season52Xor.hpp"

#include <algorithm>

namespace mu::net::s52 {
namespace {

bool ReadPacketSize(const std::uint8_t* packet,
                    std::size_t available,
                    std::size_t& declared) noexcept {
    declared = 0;
    if (packet == nullptr || available < 2) {
        return false;
    }

    switch (packet[0]) {
        case 0xC1:
        case 0xC3:
            declared = packet[1];
            return declared >= 3 && declared <= available;

        case 0xC2:
        case 0xC4:
            if (available < 3) {
                return false;
            }
            declared = (static_cast<std::size_t>(packet[1]) << 8u)
                     | static_cast<std::size_t>(packet[2]);
            return declared >= 4 && declared <= available;

        default:
            return false;
    }
}

} // namespace

bool Season52Crypto::LoadClientToServerKey(const std::uint8_t* bytes,
                                           std::size_t size) noexcept {
    return SimpleModulus::LoadKeyFile(bytes, size, clientToServer_);
}

bool Season52Crypto::LoadServerToClientKey(const std::uint8_t* bytes,
                                           std::size_t size) noexcept {
    return SimpleModulus::LoadKeyFile(bytes, size, serverToClient_);
}

namespace {

bool EncodeSeason52Packet(SimpleModulusKey& clientToServer,
                          std::uint8_t& sendSerial,
                          const std::uint8_t* packet,
                          std::size_t size,
                          std::vector<std::uint8_t>& encrypted,
                          bool forceWide) {
    encrypted.clear();
    if (!clientToServer.IsValid()) {
        return false;
    }

    std::size_t declared = 0;
    if (!ReadPacketSize(packet, size, declared) || declared != size) {
        return false;
    }

    const bool shortPacket = packet[0] == 0xC1;
    const bool widePacket = packet[0] == 0xC2;
    if (!shortPacket && !widePacket) {
        return false;
    }

    std::vector<std::uint8_t> prepared(packet, packet + size);
    if (!EncodePacketXor(prepared.data(), prepared.size())) {
        return false;
    }

    const std::size_t headOffset = shortPacket ? 2u : 3u;
    std::vector<std::uint8_t> cryptoInput;
    cryptoInput.reserve(1u + (size - headOffset));
    cryptoInput.push_back(sendSerial++);
    cryptoInput.insert(cryptoInput.end(),
                       prepared.begin() + static_cast<std::ptrdiff_t>(headOffset),
                       prepared.end());

    std::vector<std::uint8_t> cipher;
    if (!SimpleModulus::Encrypt(clientToServer,
                                cryptoInput.data(),
                                cryptoInput.size(),
                                cipher)) {
        return false;
    }

    const std::size_t c3Size = cipher.size() + 2u;
    if (!forceWide && c3Size < 256u) {
        encrypted.resize(c3Size);
        encrypted[0] = 0xC3;
        encrypted[1] = static_cast<std::uint8_t>(c3Size);
        std::copy(cipher.begin(), cipher.end(), encrypted.begin() + 2);
        return true;
    }

    const std::size_t c4Size = cipher.size() + 3u;
    if (c4Size > 0xFFFFu) {
        return false;
    }

    encrypted.resize(c4Size);
    encrypted[0] = 0xC4;
    encrypted[1] = static_cast<std::uint8_t>((c4Size >> 8u) & 0xFFu);
    encrypted[2] = static_cast<std::uint8_t>(c4Size & 0xFFu);
    std::copy(cipher.begin(), cipher.end(), encrypted.begin() + 3);
    return true;
}

} // namespace

bool Season52Crypto::Encode(const std::uint8_t* packet,
                            std::size_t size,
                            std::vector<std::uint8_t>& encrypted) {
    return EncodeSeason52Packet(
        clientToServer_, sendSerial_, packet, size, encrypted, false);
}

bool Season52Crypto::EncodeForcedWide(
    const std::uint8_t* packet,
    std::size_t size,
    std::vector<std::uint8_t>& encrypted) {
    return EncodeSeason52Packet(
        clientToServer_, sendSerial_, packet, size, encrypted, true);
}

bool Season52Crypto::Decode(const std::uint8_t* packet,
                            std::size_t size,
                            std::vector<std::uint8_t>& plain) {
    plain.clear();
    if (!serverToClient_.IsValid()) {
        return false;
    }

    std::size_t declared = 0;
    if (!ReadPacketSize(packet, size, declared) || declared != size) {
        return false;
    }

    const bool shortPacket = packet[0] == 0xC3;
    const bool widePacket = packet[0] == 0xC4;
    if (!shortPacket && !widePacket) {
        return false;
    }

    const std::size_t encryptedOffset = shortPacket ? 2u : 3u;
    if (size <= encryptedOffset) {
        return false;
    }

    std::vector<std::uint8_t> decoded;
    if (!SimpleModulus::Decrypt(serverToClient_,
                                packet + encryptedOffset,
                                size - encryptedOffset,
                                decoded)
        || decoded.size() < 2u) {
        return false;
    }

    lastReceivedSerial_ = decoded[0];

    if (shortPacket) {
        const std::size_t plainSize = decoded.size() + 1u;
        if (plainSize >= 256u) {
            return false;
        }

        plain.resize(plainSize);
        plain[0] = 0xC1;
        plain[1] = static_cast<std::uint8_t>(plainSize);
        std::copy(decoded.begin() + 1, decoded.end(), plain.begin() + 2);
        return true;
    }

    const std::size_t plainSize = decoded.size() + 2u;
    if (plainSize > 0xFFFFu) {
        return false;
    }

    plain.resize(plainSize);
    plain[0] = 0xC2;
    plain[1] = static_cast<std::uint8_t>((plainSize >> 8u) & 0xFFu);
    plain[2] = static_cast<std::uint8_t>(plainSize & 0xFFu);
    std::copy(decoded.begin() + 1, decoded.end(), plain.begin() + 3);
    return true;
}

} // namespace mu::net::s52

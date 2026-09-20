#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace mu::net::s52 {

class Season52Crypto;

inline constexpr std::size_t AccountSize = 10;
inline constexpr std::size_t PasswordSize = 20;
inline constexpr std::size_t ProtocolVersionSize = 5;
inline constexpr std::size_t ProtocolSerialSize = 16;

std::vector<std::uint8_t> BuildServerListRequest();
std::vector<std::uint8_t> BuildServerAddressRequest(std::uint16_t serverId);
std::vector<std::uint8_t> BuildCharacterListRequest(std::uint8_t language);

bool BuildLoginRequest(std::string_view account,
                       std::string_view password,
                       std::uint32_t tickCount,
                       const std::array<std::uint8_t, ProtocolVersionSize>& version,
                       const std::array<std::uint8_t, ProtocolSerialSize>& serial,
                       Season52Crypto& crypto,
                       std::vector<std::uint8_t>& wirePacket);

} // namespace mu::net::s52

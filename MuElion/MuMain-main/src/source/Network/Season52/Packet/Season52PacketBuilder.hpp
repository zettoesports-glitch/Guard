#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace mu::net::s52 {

class Season52Crypto;

inline constexpr std::size_t AccountSize = 10;
inline constexpr std::size_t CharacterNameSize = 10;
inline constexpr std::size_t PasswordSize = 12;
inline constexpr std::size_t PersonalCodeSize = 10;
inline constexpr std::size_t ProtocolVersionSize = 5;
inline constexpr std::size_t ProtocolSerialSize = 16;

std::vector<std::uint8_t> BuildServerListRequest();
std::vector<std::uint8_t> BuildServerAddressRequest(std::uint16_t serverId);

std::vector<std::uint8_t> BuildCharacterListRequest(std::uint8_t language);
std::vector<std::uint8_t> BuildCreateCharacterRequest(std::string_view name,
                                                       std::uint8_t classId,
                                                       std::uint8_t skin);
std::vector<std::uint8_t> BuildDeleteCharacterRequest(std::string_view name,
                                                       std::string_view personalCode);
std::vector<std::uint8_t> BuildSelectCharacterRequest(std::string_view name);
std::vector<std::uint8_t> BuildFinishLoadingRequest();

bool BuildLoginRequest(std::string_view account,
                       std::string_view password,
                       std::uint32_t tickCount,
                       const std::array<std::uint8_t, ProtocolVersionSize>& version,
                       const std::array<std::uint8_t, ProtocolSerialSize>& serial,
                       Season52Crypto& crypto,
                       std::vector<std::uint8_t>& wirePacket);

} // namespace mu::net::s52

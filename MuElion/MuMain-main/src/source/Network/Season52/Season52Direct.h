#pragma once

#include "Network/Season52/Crypto/Season52Crypto.hpp"
#include "Network/Season52/Packet/MuPacketStream.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

class Connection;

namespace mu::net::s52 {

bool DirectProtocolEnabled() noexcept;

class DirectSession {
public:
    static DirectSession& Instance();

    // Must be called whenever SocketClient is replaced.
    // gameServer=false: ConnectServer, no SimpleModulus is required.
    // gameServer=true: GameServer, Enc1/Dec2 are loaded and serial is reset.
    bool BeginConnection(bool gameServer);
    void Reset();

    bool IsGameServerConnection() const noexcept { return gameServer_; }
    bool KeysLoaded() const noexcept { return keysLoaded_; }

    bool SendServerList(Connection* connection);
    bool SendServerAddress(Connection* connection, std::uint16_t serverId);
    bool SendCharacterList(Connection* connection, std::uint8_t language);
    bool SendCreateCharacter(Connection* connection,
                             const wchar_t* name,
                             std::uint8_t classId,
                             std::uint8_t skin);
    bool SendDeleteCharacter(Connection* connection,
                             const wchar_t* name,
                             const wchar_t* personalCode);
    bool SendSelectCharacter(Connection* connection, const wchar_t* name);
    bool SendFinishLoading(Connection* connection);
    bool SendLogin(Connection* connection,
                   const wchar_t* account,
                   const wchar_t* password,
                   const std::uint8_t* version,
                   const std::uint8_t* serial);

    // Accepts either one complete packet or arbitrary TCP chunks. Output
    // contains complete packets suitable for the existing WSclient dispatcher.
    bool ProcessIncoming(const std::uint8_t* data,
                         std::size_t size,
                         std::vector<std::vector<std::uint8_t>>& packets);

private:
    bool EnsureKeysLoaded();
    bool SendPacket(Connection* connection, const std::vector<std::uint8_t>& packet);

private:
    Season52Crypto crypto_;
    MuPacketStream stream_;
    bool gameServer_{false};
    bool keysLoaded_{false};
    bool keyLoadAttempted_{false};
};

} // namespace mu::net::s52

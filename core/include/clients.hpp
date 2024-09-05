#ifndef HEXANE_IMPLANT_CLIENTS_HPP
#define HEXANE_IMPLANT_CLIENTS_HPP

#include <core/corelib.hpp>

namespace Clients {
    FUNCTION _client* GetClient(uint32_t peer_id);
    FUNCTION BOOL RemoveClient(uint32_t peer_id);
    FUNCTION BOOL AddClient(const wchar_t *pipe_name, uint32_t peer_id);
    FUNCTION BOOL PushClients();
}
#endif //HEXANE_IMPLANT_CLIENTS_HPP

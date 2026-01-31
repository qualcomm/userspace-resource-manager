
// tests/Utils/Include/TestInitReset.hpp
#pragma once
#include "TestUtils.h"          // for MakeAlloc<T>/FreeBlock/etc.
#include "Request.h"
#include "RequestManager.h"
#include "ClientDataManager.h"
// #include "CocoTable.h"       // if you also need to reset this in RequestMap tests

namespace testinit {

// Initialize all pools/components commonly needed by request-related tests.
inline void InitAll() {
    // From your gdb backtraces: Request::Request() needs DLManager
    MakeAlloc<DLManager>(128);

    // Because your tests call GetBlock<Request>() and MPLACED(Resource/ResIterable)
    MakeAlloc<Request>(512);
    MakeAlloc<Resource>(512);
    MakeAlloc<ResIterable>(512);

    // Add more if future gdb traces show other getBlock<T>() throws:
    // MakeAlloc<RequestNode>(512);
    // MakeAlloc<CocoNode>(256);
}

// Reset/clear singletons or global state so each test starts clean.
inline void ResetAll() {
    // Prefer real, public reset/clear APIs if available:
    // RequestManager::getInstance()->clear();
    // ClientDataManager::getInstance()->clearAll();
    // CocoTable::getInstance()->clear();

#ifdef ENABLE_TEST_RESET
    // If production does not expose reset, add TEST_ONLY hooks guarded by this macro.
    // RequestManager::TEST_ONLY_Reset();
    // ClientDataManager::TEST_ONLY_Reset();
    // CocoTable::TEST_ONLY_Reset();
#endif
}

} // namespace testinit


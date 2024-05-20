#include "pcsx2_ipc.h"
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <climits>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define u128 __int128

/* Test case suite for the PCSX2 IPC API
 * You will probably need to set environment variables to be able
 * to boot emulator(s) with some ISO for all of this to run. Refer to
 * utils/default.nix for an example on how to do that.
 */

// a portable sleep function
auto msleep(int sleepMs) -> void {
#ifdef _WIN32
    Sleep(sleepMs);
#else
    usleep(sleepMs * 1000);
#endif
}

auto open_pcsx2() -> void {
    if (const char *env_p = std::getenv("PCSX2_TEST")) {
        char pcsx2_path[4096];
        strcpy(pcsx2_path, env_p);
        strcat(pcsx2_path, " &");
        if (system(pcsx2_path) != 0) {
            printf("PCSX2 failed to execute! Have you set your environment "
                   "variables?\n");
        }
    }
}

#ifdef _WIN32
auto kill_pcsx2() -> int { return system("tskill PCSX2"); }
#else
auto kill_pcsx2() -> int { return system("pkill PCSX2"); }

#endif

SCENARIO("PCSX2 can be interacted with remotely through IPC", "[pcsx2_ipc]") {

    // ensure we have a clean environment
    kill_pcsx2();

    WHEN("PCSX2 is not started") {
        THEN("Errors should happen") {
            PCSX2Ipc *ipc = new PCSX2Ipc();
            REQUIRE_THROWS(ipc->Write<u64>(0x00347D34, 5));
        }
    }

    GIVEN("PCSX2 with IPC started") {

        open_pcsx2();

        // we wait for PCSX2 to be reachable within 10 seconds.
        PCSX2Ipc *check = new PCSX2Ipc();
        int i = 0;
        while (i < 20) {
            try {
                check->Read<u8>(0x00347D34);
                break;
            } catch (...) {
                msleep(500);
                i++;
                // F
                if (i >= 20)
                    REQUIRE(0 == 1);
            }
        }

        WHEN("We want to communicate with PCSX2") {
            THEN("It returns errors on invalid commands") {
                PCSX2Ipc *ipc = new PCSX2Ipc();

                char c_cmd[5];
                c_cmd[4] = PCSX2Ipc::MsgUnimplemented;
                c_cmd[0] = 5;
                char c_ret[5];

                // send unimplement message to server
                REQUIRE_THROWS(
                    ipc->SendCommand(PCSX2Ipc::IPCBuffer{ 5, c_cmd },
                                     PCSX2Ipc::IPCBuffer{ 5, c_ret }));

                // make unimplemented write
                REQUIRE_THROWS(ipc->Write<u128>(0x00347D34, 5));

                // make unimplemented read
                REQUIRE_THROWS(ipc->Read<u128>(0x00347D34));
            }

            THEN("It returns errors when socket issues happen") {
                PCSX2Ipc *ipc = new PCSX2Ipc();

                char c_cmd[5];
                c_cmd[0] = 5;

                // trying to read from socket with INT_MAX and/or pointer from
                // an address space that is not yours will return an errno. We
                // do both to simulate a connection issue.
                REQUIRE_THROWS(ipc->SendCommand(
                    PCSX2Ipc::IPCBuffer{ 5, c_cmd },
                    PCSX2Ipc::IPCBuffer{ INT_MAX, (char *)0x00 }));

                // trying to write to a socket with INT_MAX and/or pointer from
                // an address space that is not yours will return an errno. We
                // do both to simulate a connection issue.
                REQUIRE_THROWS(ipc->SendCommand(
                    PCSX2Ipc::IPCBuffer{ INT_MAX, (char *)0x00 },
                    PCSX2Ipc::IPCBuffer{ 1, c_cmd }));
            }
        }

        WHEN("We want to read/write to the memory") {
            THEN("The read/writes are consistent") {

                // we do a bunch of writes, read back, ensure the result is
                // correct and ensure we didn't threw an error.
                REQUIRE_NOTHROW([&]() {
                    PCSX2Ipc *ipc = new PCSX2Ipc();
                    ipc->Write<u64>(0x00347D34, 5);
                    ipc->Write<u32>(0x00347D44, 6);
                    ipc->Write<u16>(0x00347D54, 7);
                    ipc->Write<u8>(0x00347D64, 8);
                    REQUIRE(ipc->Read<u64>(0x00347D34) == 5);
                    REQUIRE(ipc->Read<u32>(0x00347D44) == 6);
                    REQUIRE(ipc->Read<u16>(0x00347D54) == 7);
                    REQUIRE(ipc->Read<u8>(0x00347D64) == 8);
                }());
            }
        }

        WHEN("We want to know PCSX2 Version") {
            THEN("It returns a correct one") {

                // we do a bunch of writes, read back, ensure the result is
                // correct and ensure we didn't threw an error.
                REQUIRE_NOTHROW([&]() {
                    PCSX2Ipc *ipc = new PCSX2Ipc();
                    char* version = ipc->Version();
                    REQUIRE(strncmp(version, "PCSX2", 5) == 0);
                }());
            }
        }

        WHEN("We want to execute multiple operations in a row") {
            THEN("The operations get executed successfully and consistently") {

                // we do a bunch of writes, read back, ensure the result is
                // correct and ensure we didn't threw an error, in batch mode
                // this time.
                REQUIRE_NOTHROW([&]() {
                    PCSX2Ipc *ipc = new PCSX2Ipc();

                    ipc->InitializeBatch();
                    ipc->Write<u64, true>(0x00347E34, 5);
                    ipc->Write<u32, true>(0x00347E44, 6);
                    ipc->Write<u16, true>(0x00347E54, 7);
                    ipc->Write<u8, true>(0x00347E64, 8);
                    ipc->SendCommand(ipc->FinalizeBatch());

                    msleep(1);

                    ipc->InitializeBatch();
                    ipc->Read<u64, true>(0x00347E34);
                    ipc->Read<u32, true>(0x00347E44);
                    ipc->Read<u16, true>(0x00347E54);
                    ipc->Read<u8, true>(0x00347E64);
                    ipc->Version<true>();
                    auto resr = ipc->FinalizeBatch();
                    ipc->SendCommand(resr);

                    REQUIRE(strncmp(ipc->GetReply<PCSX2Ipc::MsgVersion>(resr, 4), "PCSX2", 5) == 0);
                    REQUIRE(ipc->GetReply<PCSX2Ipc::MsgRead8>(resr, 3) == 8);
                    REQUIRE(ipc->GetReply<PCSX2Ipc::MsgRead16>(resr, 2) == 7);
                    REQUIRE(ipc->GetReply<PCSX2Ipc::MsgRead32>(resr, 1) == 6);
                    REQUIRE(ipc->GetReply<PCSX2Ipc::MsgRead64>(resr, 0) == 5);
                }());
            }

            THEN("We error out when packets are too big") {
                PCSX2Ipc *ipc = new PCSX2Ipc();

                // write packets too big
                REQUIRE_THROWS([&]() {
                    ipc->InitializeBatch();
                    for (int i = 0; i < 60000; i++) {
                        ipc->Write<u64, true>(0x00347E34, 5);
                    }
                    ipc->SendCommand(ipc->FinalizeBatch());
                }());

                // read packets too big
                REQUIRE_THROWS([&]() {
                    ipc->InitializeBatch();
                    for (int i = 0; i < 60000; i++) {
                        ipc->Read<u64, true>(0x00347E34);
                    }
                    ipc->SendCommand(ipc->FinalizeBatch());
                }());
            }
        }

        // probably a good idea to kill it once we're done testing
        kill_pcsx2();
    }
}

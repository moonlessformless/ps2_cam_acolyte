#include "pcsx2_ipc.h"
#include <iostream>
#include <ostream>
#include <stdio.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

// TODO: Remind me when C++20 using-enum is implemented in clang/gcc so i can
// write cleaner code
// using enum PCSX2Ipc::IPCCommand;

// a portable sleep function
auto msleep(int sleepMs) -> void {
#ifdef _WIN32
    Sleep(sleepMs);
#else
    usleep(sleepMs * 1000);
#endif
}

// this function is an infinite loop reading a value in memory, this shows you
// how timer can work
auto read_background(PCSX2Ipc *ipc) -> void {
    while (true) {
        // you can go slower but go higher at your own risk
        msleep(100);

        // we read a 32 bit value from memory address 0x00347D34
        try {
            // those comments calculate a rough approximation of the latency
            // time of socket IPC, in µs, if you want to have an idea.

            // auto t1 = std::chrono::high_resolution_clock::now();
            uint32_t value = ipc->Read<u32>(0x00347D34);
            // auto t2 = std::chrono::high_resolution_clock::now();
            // auto duration =
            //    std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1)
            //        .count();
            // std::cout << "execution time: " << duration << std::endl;
            printf("PCSX2Ipc::Read<uint32_t>(0x00347D34) :  %u\n", value);
        } catch (...) {
            // if the operation failed
            printf("ERROR!!!!!\n");
        }
    }
}

// the main function that is executed at the start of our program
auto main(int argc, char *argv[]) -> int {

    // we instantiate a new PCSX2Ipc object. It should be shared across all your
    // threads.
    PCSX2Ipc *ipc = new PCSX2Ipc();

    // we create a new thread
    std::thread first(read_background, ipc);

    // in this case we wait 5 seconds before writing to our address
    msleep(5000);
    try {
        printf("%s\n", ipc->Version());
        // a normal write can be done this way
        ipc->Write<u8>(0x00347D34, 0x5);

        // if you need to make a lot of IPC requests at once(eg >50/frame @
        // 60fps) it is recommended to build a batch message: you should build
        // this message at the start of your thread once and keep the
        // BatchCommand to avoid wasting time recreating this
        // IPC packet.
        //
        // to create a batch IPC packet you need to initialize it, be sure to
        // enable the batch command in templates(read the documentation, for
        // Read it is Read<T, true>) and finalize it.
        ipc->InitializeBatch();
        ipc->Write<u8, true>(0x00347D34, 0xFF);
        ipc->Write<u8, true>(0x00347D33, 0xEF);
        ipc->Write<u8, true>(0x00347D32, 0xDF);
        auto res = ipc->FinalizeBatch();
        // our batch ipc packet is now saved and ready to be used whenever! When
        // we need it we just fire up a SendCommand:
        ipc->SendCommand(res);

        // let's do it another time, but this time with Read, which returns
        // arguments!
        ipc->InitializeBatch();
        ipc->Read<u8, true>(0x00347D34);
        ipc->Read<u8, true>(0x00347D33);
        ipc->Read<u8, true>(0x00347D32);
        auto resr = ipc->FinalizeBatch();
        // same as before
        ipc->SendCommand(resr);

        // now reading the return value is a little bit more tricky, you'll have
        // to know the type of your function and the number it was executed in.
        // For example, Read(0x00347D32) was our third function, and is a
        // function of type MsgRead8, so we will do:
        //   GetReply<MsgRead8>(resr, 2);
        // 2 since arrays start at 0 in C++, so 3-1 = 2 and resr being our
        // BatchCommand that we saved above!
        // Refer to the documentation of IPCCommand to know all the possible
        // function types
        printf("PCSX2Ipc::Read<uint8_t>(0x00347D32) :  %u\n",
               ipc->GetReply<PCSX2Ipc::MsgRead8>(resr, 2));
    } catch (...) {
        // if the operation failed
        printf("ERROR!!!!!\n");
    }

    // we wait for the thread to finish. in our case it is an infinite loop
    // (while true) so it will never do so.
    first.join();

    return 0;
}

import ctypes
import os
import platform


# we get the correct library extension per os
lib="libpcsx2_ipc_c"
cur_os = platform.system()
if(cur_os == "Linux"):
    lib="libpcsx2_ipc_c.so"
elif(cur_os == "Windows"):
    lib="libpcsx2_ipc_c.dll"
elif(cur_os == "Darwin"):
    lib="libpcsx2_ipc_c.dylib"


# we load the library, this will require it to be in the same folder
# refer to bindings/c to build the library.
libipc = ctypes.CDLL(os.path.join(os.path.dirname(os.path.abspath(__file__)),lib))

# we create a new PCSX2Ipc object
ipc = libipc.pcsx2ipc_new()

# we read an uint8_t from memory location 0x00347D34
print(libipc.pcsx2ipc_read(ipc, 0x00347D34, 0, False))

# we check for errors
print("Error (if any): " + str(libipc.pcsx2ipc_get_error(ipc)))

# we delete the object and free the resources
libipc.pcsx2ipc_delete(ipc)

# for more infos check out the C bindings documentation :D !

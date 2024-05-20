PCSX2 IPC Client-Side Reference API
======

[![build](https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fci.govanify.com%2Fjob%2Fgovanify%2Fjob%2Fpcsx2_ipc%2Fjob%2Fmaster)
![tests](https://img.shields.io/jenkins/tests?compact_message&jobUrl=https%3A%2F%2Fci.govanify.com%2Fjob%2Fgovanify%2Fjob%2Fpcsx2_ipc%2Fjob%2Fmaster%2F)
![coverage](https://img.shields.io/jenkins/coverage/api?jobUrl=https%3A%2F%2Fci.govanify.com%2Fjob%2Fgovanify%2Fjob%2Fpcsx2_ipc%2Fjob%2Fmaster%2F)
](https://ci.govanify.com/blue/organizations/jenkins/govanify%2Fpcsx2_ipc/activity?branch=master)

You'll find [here](https://code.govanify.com/govanify/pcsx2_ipc/)
the reference implementation of PCSX2 IPC Socket client-side C++ API, along with
different language bindings and examples.    

A small C++ client example is provided along with the API. It can be compiled
by executing the command `meson build && cd build && ninja` in the folder
"example" that is included in the releases.  
If you want to run the tests you'll have to do 
`meson build && cd build && meson test`. This will require you to set
environment variables to correctly startup the emulator(s). Refer to `src/tests.cpp`
to see which ones.

Meson and ninja ARE portable across OSes as-is and shouldn't require any tinkering. Please
refer to [the meson documentation](https://mesonbuild.com/Using-with-Visual-Studio.html) 
if you really want to use another generator, say, Visual Studio, instead of ninja.   
Alternatively, loading the "windows-qt.pro" on Windows with Qt Creator will work just fine if you're lazy.  
If you dislike C++
[bindings in popular languages are
available](https://code.govanify.com/govanify/pcsx2_ipc/src/branch/master/bindings/).

On Doxygen you can find the documentation of the C++ API [here](@ref PCSX2Ipc).  
The C API is documented [here](@ref bindings/c/c_ffi.h) and is probably what you
want to read if you use language bindings.

Language bindings will require you to compile the C bindings library for the OS
you target. Please refer to `bindings/c` documentation for building it.

Have fun!  
-Gauvain "GovanifY" Roussel-Tarbouriech, 2020

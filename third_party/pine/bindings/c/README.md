This folder allows you to build a C compatible library, libpcsx2\_ipc\_c, usable
by C programs and much more by using the FFI of other programming languages.  
Refer to the [meson documentation](https://mesonbuild.com/) for how to build
this library on your OS. Alternatively, if your OS is sane, running 
`meson build && cd build && ninja` should build your library just fine.

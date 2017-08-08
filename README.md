# AppMyProduct device stub [![Build Status](https://travis-ci.org/nabto/appmyproduct-device-stub.svg?branch=master)](https://travis-ci.org/nabto/appmyproduct-device-stub)

General [AppMyProduct](https://www.appmyproduct.com) device stub
application to enable testing of AMP sample apps and to use as basis
for actual vendor specific device applications. Currently supports the
[Heat Control](https://github.com/nabto/ionic-starter-nabto) Ionic
starter app.

# Building for desktop testing

The device application can be built on any system with cmake and a toolchain supported by cmake. So the following are prerequisites:

1. install cmake - either through your package manager (e.g. `apt-get install cmake` or `brew install cmake`) or by downloading from https://cmake.org/download/

2. install a C compiler - e.g., on Windows install Visual Studio, on macOS install XCode (Linux (incl. Raspberry Pi) typically comes with one)

3. on Windows, install a git client

When these prerequisites are satisfied, the following steps can be performed:


```
# Fetch the stub source - note the --recursive option to also install the uNabto SDK
git clone --recursive https://github.com/nabto/appmyproduct-device-stub

# got to the main directory
cd appmyproduct-device-stub

# Create a build directory
mkdir build
cd build

# Build the stub
cmake ..
make -j3

# Run the demo with a device ID and key obtained from www.appmyproduct.com 
./amp_device_stub -d kzspcxu3.gygkd.appmyproduct.com -k de6bc4ced691531ee8e5e345e47cb1cd
```

If everything goes well, you will see output resembling the following:

```console
06:36:03:574 unabto_common_main.c(127) Device id: 'kzspcxu3.gygkd.appmyproduct.com'
06:36:03:575 unabto_common_main.c(128) Program Release 123.456
06:36:03:575 unabto_app_adapter.c(698) Application event framework using SYNC model
06:36:03:575 unabto_context.c(55) SECURE ATTACH: 1, DATA: 1
06:36:03:575 unabto_context.c(63) NONCE_SIZE: 32, CLEAR_TEXT: 0
06:36:03:576 unabto_common_main.c(206) Nabto was successfully initialized
06:36:03:576 unabto_main.c(82) AppMyProduct demo stub [kzspcxu3.gygkd.appmyproduct.com] running!
06:36:03:576 unabto_context.c(55) SECURE ATTACH: 1, DATA: 1
06:36:03:576 unabto_context.c(63) NONCE_SIZE: 32, CLEAR_TEXT: 0
06:36:03:576 unabto_attach.c(792) State change from IDLE to WAIT_DNS
06:36:03:576 unabto_attach.c(793) Resolving dns: kzspcxu3.gygkd.appmyproduct.com
06:36:03:587 unabto_attach.c(814) State change from WAIT_DNS to WAIT_BS
06:36:03:632 unabto_attach.c(479) State change from WAIT_BS to WAIT_GSP
06:36:03:644 unabto_attach.c(266) ########    U_INVITE with LARGE nonce sent, version: - URL: -
06:36:03:678 unabto_attach.c(580) State change from WAIT_GSP to ATTACHED
```

The crucial part is the `ATTACHED` message on the last line, telling that the device stub is now registered with the Nabto services and ready to access from remote (note that per default the device is configured to only allow local connections, though - the user must change this (or you must change the defaults in `demo_init()` in `src/unabto_application.c`).

If you don't specify a cryptographic key that matches the device id, the device will not register to allow remote access, but you will still be able to test the device locally.

# Building for an embedded system

## uNabto platform adapter

If you want to run the device application on an actual limited
embedded system, first check if the system is already supported by the
uNabto SDK - a _platform adapter_ supporting the system and IP stack
in question must be defined. Take a look in `unabto/src/platforms`.

If your specific platform is not supported, a new platform adapter
must be implemented - see section 12.1 "Porting uNabto" in [TEN023 "How to
write a uNabto device
application"](https://www.nabto.com/downloads/docs/TEN023%20Writing%20a%20uNabto%20Device%20Application.pdf)
(pdf).

## Cross compilation

If a gcc toolchain exists for the target architecture, CMake can build
for it out of the box: Prior to following the general build steps
above, just export `CC` to point to the cross gcc compiler:

```
mkdir build
cd build
export CC=/opt/toolchains/arm-2014.05/arm-none-linux-gnueabi/bin/gcc
cmake ..
make -j3
```

For platforms with toolchains not supported by cmake, the uNabto SDK
files must be added along with the stub application
(`src/unabto_application.c`) and an adaptation of the stub main file
to the specific platform (for instance, `src/unabto_main.c` assumes
commandline args can be passed to main).

The necessary uNabto SDK files to include in a non-cmake project can
be seen in the `CMakeLists.txt` project definition file, the groups of
files such as `${unabto_src}` and
`{unabto_module_fingerprint_acl_src}` are defined in
`unabto/build/cmake/unabto_files.cmake`. That is, use these
definitions to know which files to include e.g. in your IDE-based project
setup.

# Virtual-BreadBoard
VBB is a Virtual Breadboard / PCB simultation for Prototyping and Educational Purposes.
It is featured in [this journal](https://www.mdpi.com/2079-9268/12/4/52/htm) and was originally in the [riscv-vp](https://github.com/agra-uni-bremen/riscv-vp) repository.
It is however not limited to risc-v specific hardware and thus was moved.
If you are using thethe Virtual Breadboard GUI in a scientific paper, please cite this journal: https://www.mdpi.com/2079-9268/12/4/52.

It features a TCP protocol that connects GPIO-Modules of virtual Processors to this interactive GUI.
Devices can be modelled in either C++ or Lua for the speed/flexibility tradeoff.


#### 1) HowTo Use

This repo contains some example environments (`.json` configuration files) and loads an OLED screen with some buttons per default.
For a complete list of configuration files and available devices, run `vp-breadboard -h`.

#### 2) Available Demos

Currently, there is a CLI tool that mocks a GPIO module in `lib/protocol/test`, and the fully featured riscv-vp in its *Sifive HiFive1* target.
For building the riscv-vp, please refer to https://github.com/agra-uni-bremen/riscv-vp.
Some example programs, such as a snake game, are built around the Sifive Hifive1 board and can be found in this repo: https://github.com/agra-uni-bremen/sifive-hifive1.

#### 3) Build requirements

Mainly the usual build tools (and lua) are required:

On Ubuntu 22, install these:
```bash
sudo apt-get install autoconf automake autotools-dev curl libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo libgoogle-perftools-dev libtool patchutils bc zlib1g-dev libexpat-dev qt5-default liblua5.4-dev 
```

On Fedora, following actions are required:
```bash
sudo dnf groupinstall "C Development Tools and Libraries"
sudo dnf install autoconf automake curl libmpc-devel mpfr-devel gmp-devel gawk bison flex texinfo gperf libtool patchutils bc zlib-devel expat-devel cmake qt5-qtbase qt5-qtbase-devel lua-devel
#optional debuginfo
sudo dnf debuginfo-install boost-iostreams boost-program-options boost-regex bzip2-libs glibc libgcc libicu libstdc++ zlib
```
Then, just build it in CMake style: `mkdir build && cd build && cmake .. && make`.

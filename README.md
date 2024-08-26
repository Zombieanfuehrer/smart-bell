# atmega328p-cxx-template
This project template uses Conan and CMake to provide a preconfigured project environment.

![Latest Release](https://img.shields.io/github/v/release/Zombieanfuehrer/atmega328p-cxx-template)
![Build Status](https://github.com/Zombieanfuehrer/atmega328p-cxx-template/actions/workflows/build.yml/badge.svg)
![License](https://img.shields.io/github/license/Zombieanfuehrer/atmega328p-cxx-template)
![GitHub issues](https://img.shields.io/github/issues/Zombieanfuehrer/atmega328p-cxx-template)
![GitHub stars](https://img.shields.io/github/stars/Zombieanfuehrer/atmega328p-cxx-template)

## Project Description

This project is a GitHub template for the AVR-ATmega microcontroller family.
It implements DevOps fundamentals and supports easy integration of libraries into other projects through Conan.
CMake is used as the build tool, which supports easy build configuration via Conan in the form of CMake presets.
This project aims to provide a basic foundation for both beginner and advanced programmers to engage in modern software development in C/C++.

## Using this Template

To create a new project based on this template, follow these steps:

1. Go to the [repository page](https://github.com/Zombieanfuehrer/atmega328p-cxx-template).
2. Click on the "Use this template" button.
3. Enter the name and description for your new repository.
4. Click "Create repository from template".

Your new project will be created with the same files and structure as this template repository.

## Prerequisites

To use this template, the following prerequisites must be met:
- Linux, WSL, or a corresponding container.
- Installed __avr-gcc toolchain__ [download avr-gcc](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers "avr-gcc toolchain downloads from microchip.com")

```sh
# Example installation of the 8-bit toolchain, e.g., for ATmega328p

# Download with wget
cd /opt/ && wget https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/avr8-gnu-toolchain-3.7.0.1796-linux.any.x86_64.tar.gz
# Alternatively, you can use curl
cd /opt/ && curl -O https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/avr8-gnu-toolchain-3.7.0.1796-linux.any.x86_64.tar.gz
# Extract the archive with tar
tar -C ./ -xf ./avr8-gnu-toolchain-3.7.0.1796-linux.any.x86_64.tar.gz
# Set the appropriate ownership
chown -R root:root /opt/avr8-gnu-toolchain-linux_x86_64
```

- __CMake__ min. Version 3.20
```sh
# Installation of CMake
sudo apt-get install -y cmake
```
- __Python3__ including venv and pip
```sh
# Installation of Python3, venv, and pip
sudo apt-get install -y python3 python3-venv python3-pip
```
- __Doxygen__  (optional) with Graphviz
```sh
# Installation of Doxygen and Graphviz
sudo apt-get install -y doxygen graphviz
```
- __Conan__ >= Version 2.0
```sh
# Conan is automatically added via the CMake project through ./requirements.txt
# If you want to use Conan manually outside of this project, it is recommended to install Conan in a virtual Python environment
python3 -m venv <name_of_virtual_env> <target_path>
source <name_of_virtual_env>/bin/activate
pip install conan
# Alternatively, install Conan directly
pip install conan
```
- __Conan profiles__  Background -> [conan introduction to profiles](https://docs.conan.io/2/reference/config_files/profiles.html "conan 2 profile documentation")

After installing Conan, we need specific Conan profiles for the cross-compilation process via Conan -> CMake -> avr-gcc toolchain to set and pass the environment. To use this Conan feature, we need to create a default profile:
```sh
# Create default Conan profile with default name
conan profile detect
```
> __Note:__ The created profile is usually located at: *~/.conan2/profiles/default*

To use the specific Conan profiles for the ATmega328p, you can download them from [this repo](https://github.com/Zombieanfuehrer/conan-profiles-linux "conan 2 Zombieanfuehrer/conan-profiles-linux")
and copy them to the folder: */.conan2/profiles/*

-__avr-mega328p__: This profile is used to create a release build.

-__avr-mega328p_g__: This profile is used to create a debug build.

## Build Instructions

The easiest way to build the project and use it as a basis for developing our own projects is to build it via Conan install.

```sh
# Run conan install in the project directory or in reference to the conanfile.py
conan install . --build=missing -pr:h=avr-mega328p_g
# Set the shell environment with the corresponding variables defined by Conan
. build/Debug/generators/conanbuild.sh
# Set the CMake preset, definition from conanfile.py
# to pass the build environment and compiler/linker flags to CMake
cmake --preset conan-generated-avr-debug
# Start the build process via CMake (build/Debug sets the target directory for binaries and artifacts)
cmake --build build/Debug
```

For a release build, the Conan profile, preset, and directory names should be changed accordingly.
Afterwards, the project can be edited using an IDE of your choice, such as VSCode.
With a CMake plugin, the other targets can be executed in the IDE, but this can also be done via the shell.

Alternatively, the project can also be configured directly via CMake, but this bypasses the idea of using Conan as a build interface.

```sh
# Create a build directory and switch to it
mkdir build
cd build
# Run CMake configure with ./CMakeLists.txt
cmake ..
# Load the CMake preset
cmake --preset conan-generated-avr-debug
# Start the build process via CMake (build/Debug sets the target directory for binaries and artifacts)
cmake --build build/Debug
```

## License

This project is licensed under the MIT License. For more information, see the [LICENSE](LICENSE "MIT") file.
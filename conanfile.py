import os
import subprocess
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy

class Atmega328TemplateRecipe(ConanFile):
    name = "atmega328_smart_bell_fw"
    version = "1.0.0"

    # Optional metadata
    license = "MIT"
    author = "David A. Haufe"
    url = "https://github.com/Zombieanfuehrer/atmega328p-cxx-template"
    description = "Smart Bell firmware for ATmega328P"
    topics = ("avr")

    # Binary configuration
    settings = "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False], "platform": ["avr", "linux"], "tests": [True, False]}
    default_options = {"shared": False, "fPIC": False, "platform": "avr", "tests": False}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "*.cmake", "app/*", "src/*", "public/*", "private/*", "tests/*", "style/*", "docs/Doxyfile", "configure/*", ".github/workflows/*", "cmake/*", ".gitignore", "LICENSE", "README.md", "requirements.txt", "conanfile.py"

    def requirements(self):
        if self.options.platform == "linux" and self.options.tests:
            self.requires("gtest/1.16.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ENABLE_UNIT_TESTS"] = "ON" if self.options.tests else "OFF"
        tc.presets_prefix = "conan-generated-" + str(self.options.platform)
        deps = CMakeDeps(self)
        deps.generate()
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        if self.options.platform == "linux":
            cmake.build(target="run_unit_tests_fw_logic")
            self.perform_ctest()
        else:
            cmake.build(target="ATmega328_WTD")
            cmake.build(target="ATmega328_PIN_INT")
            cmake.build(target="ATmega328_TIMER_INT")
            cmake.build(target="ATmega328_USART")
            cmake.build(target="ATmega328_SPI")
            cmake.build(target="ATmega328_UTILS")

            cmake.build(target="ATmega328_UART_EXAMPLE_FW")
            cmake.build(target="ATmega328_UART_EXAMPLE_FW_hex")

    def package(self):
        cmake = CMake(self)
        cmake.install()
        # Copy the app folder to the package folder
        package_app_dir = os.path.join(self.package_folder, "app")
        copy(self, "ATmega328_SMART_BELL_FW.hex", os.path.join(self.build_folder, "app"), package_app_dir)
        # Copy libs to the package folder
        package_lib_dir = os.path.join(self.package_folder, "lib")
        copy(self, "*.a", os.path.join(self.build_folder, "lib"), package_lib_dir)
        package_public_dir = os.path.join(self.package_folder, "public")
        copy(self, "*.h", os.path.join(self.build_folder, "public"), package_public_dir)

    def package_info(self):
        self.cpp_info.libs = [
            "ATmega328_WTD",
            "ATmega328_PIN_INT",
            "ATmega328_TIMER_INT",
            "ATmega328_USART",
            "ATmega328_SPI",
            "ATmega328_UTILS"
        ]
        self.cpp_info.includedirs = ["public"]
        self.cpp_info.libdirs = ['lib']
        self.cpp_info.bindirs = ['app']

    def perform_ctest(self):
        self.output.info("Running unit tests with CTest")
        result = subprocess.run(["ctest", "--output-on-failure"], cwd=self.build_folder)
        if result.returncode != 0:
            raise Exception("Unit tests failed")
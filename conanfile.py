import os
import subprocess
import shutil
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy, download, unzip, check_sha1
from conan.tools.scm import Git

class Atmega328TemplateRecipe(ConanFile):
    name = "atmega328_smart_bell_fw"
    version = "1.0.0"

    # Optional metadata
    license = "MIT"
    author = "David A. Haufe"
    url = "https://github.com/Zombieanfuehrer/atmega328p-cxx-template"
    description = "Smart Bell firmware for ATmega328P"
    topics = ("avr")

    # Configuration
    settings = "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "platform": ["avr", "linux"],
        "tests": [True, False],
        "W5500_support": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": False,
        "platform": "avr",
        "tests": False,
        "W5500_support": False
    }
    ioLibrary_Driver_Version = "v3.2.0"
    exports_sources = "CMakeLists.txt", "*.cmake", "app/*", "src/*", "public/*", "private/*", "tests/*", "style/*", "docs/Doxyfile", "configure/*", ".github/workflows/*", "cmake/*",".gitignore", "LICENSE", "README.md", "requirements.txt", "conanfile.py"
    
    def requirements(self):
        if self.options.platform == "linux" and self.options.tests:
            self.requires("gtest/1.16.0")

    def layout(self):
        arch = str(self.settings.arch)
        cmake_layout(self, build_folder=f"build/{arch}")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ENABLE_UNIT_TESTS"] = "ON" if self.options.tests else "OFF"
        tc.presets_prefix = "conan-generated-" + str(self.options.platform)
        deps = CMakeDeps(self)
        deps.generate()
        tc.generate()

    def source(self):
        ioLibrary_Driver_zip = f"ioLibrary_Driver-{self.ioLibrary_Driver_Version}.zip"
        download(self, f"https://github.com/Wiznet/ioLibrary_Driver/archive/refs/tags/{self.ioLibrary_Driver_Version}.zip", ioLibrary_Driver_zip)
        check_sha1(self, ioLibrary_Driver_zip, "661eca703e2852027bf6c4c849b463d413c6afa0")
        unzip(self, ioLibrary_Driver_zip)
        extracted_folder = f"ioLibrary_Driver-{self.ioLibrary_Driver_Version.lstrip('v')}"
        os.makedirs("extern/ioLibrary_Driver", exist_ok=True)
        shutil.move(extracted_folder, "extern/ioLibrary_Driver")
        os.remove(ioLibrary_Driver_zip)
            
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        if self.options.platform == "linux":
            cmake.build(target="run_gTest")
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
            
            if self.options.W5500_support:
                cmake.build(target="ATmega328_W5500_ETHERNET_WIZNET_IOLIBRARY")
                cmake.build(target="ATmega328_W5500_ETHERNET")
                cmake.build(target="ATmega328_TCP_SERVER_EXAMPLE_FW")
                cmake.build(target="ATmega328_TCP_SERVER_EXAMPLE_FW_hex")

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

        if self.options.W5500_support:
            self.cpp_info.libs.extend([
                "ATmega328_W5500_ETHERNET_WIZNET_IOLIBRARY",
                "ATmega328_W5500_ETHERNET",
                "ATmega328_TCP_SERVER_EXAMPLE_FW"
            ])

        self.cpp_info.includedirs = ["public"]
        self.cpp_info.libdirs = ['lib']
        self.cpp_info.bindirs = ['app']

    def perform_ctest(self):
        self.output.info("Running unit tests with CTest")
        result = subprocess.run(["ctest", "--output-on-failure"], cwd=self.build_folder)
        if result.returncode != 0:
            raise Exception("Unit tests failed")
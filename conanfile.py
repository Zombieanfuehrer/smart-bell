import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import copy

class Atmega328TemplateRecipe(ConanFile):
    name = "atmega328_template"
    version = "1.0.0"

    # Optional metadata
    license = "MIT"
    author = "David A. Haufe"
    url = "https://github.com/Zombieanfuehrer/atmega328p-cxx-template"
    description = "Just a simple conan + cmake project to support the atmega328p"
    topics = ("avr")

    # Binary configuration
    settings = "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False], "platform": ["avr", "linux"]}
    default_options = {"shared": False, "fPIC": False, "platform": "avr"}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "*.cmake", "app/*", "src/*", "public/*", "private/*", "style/*", "docs/Doxyfile", "configure/*", ".github/workflows/*", "cmake/*", ".gitignore", "LICENSE", "README.md", "requirements.txt", "conanfile.py"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.presets_prefix = "conan-generated-" + str(self.options.platform)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()

        cmake.build(target="ATmega328__T_LIB")
        cmake.build(target="ATmega328__T_")

    def package(self):
        cmake = CMake(self)
        cmake.install()

        package_bin_dir = os.path.join(self.package_folder, "bin")
        os.makedirs(package_bin_dir, exist_ok=True)
        copy(self, "*", os.path.join(self.build_folder, "bin"), package_bin_dir)

    def package_info(self):
        self.cpp_info.libs = ["ATmega328__T_LIB"]

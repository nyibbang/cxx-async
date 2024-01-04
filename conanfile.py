from conan import ConanFile, tools
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class CxxAsyncRecipe(ConanFile):
    name = "cxx-async"
    version = "0.1"
    package_type = "application"

    # Optional metadata
    license = "BSD-3"
    author = "vincent.palancher@aldebaran.com"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*"

    def layout(self):
        cmake_layout(self)

    def validate(self):
        # Require at least C++20
        if self.settings.compiler.cppstd:
            tools.build.check_min_cppstd(self, "20")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

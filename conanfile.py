from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy
import os


class PiBaseConan(ConanFile):
    name = "pibase"
    version = "0.1.0"
    license = "MIT"
    author = "Aeroscis"
    url = "https://gitee.com/Aeroscis/pibase"
    description = "The PI family base layer: dependency-free C vocabulary shared by every PI library"
    topics = ("base", "c", "abi", "ffi", "header-only")

    settings = "os", "compiler", "build_type", "arch"

    # Header-only: no binary is produced, so no settings may enter the package id
    # (see package_id below) - a consumer must be able to share one package
    # between build types and architectures.
    package_type = "header-library"

    options = {
        "PI_BASE_BUILD_TESTS": [True, False],
    }
    default_options = {
        "PI_BASE_BUILD_TESTS": False,
    }

    exports_sources = "include/*", "cmake/*", "tests/*", "CMakeLists.txt"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        # Same name as the CMake option, forwarded verbatim - one switch, not two
        # vocabularies for the same thing.
        tc.cache_variables["PI_BASE_BUILD_TESTS"] = bool(self.options.PI_BASE_BUILD_TESTS)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "*.h",
             src=os.path.join(self.source_folder, "include"),
             dst=os.path.join(self.package_folder, "include"))
        copy(self, "LICENSE",
             src=self.source_folder,
             dst=os.path.join(self.package_folder, "licenses"))

    def package_id(self):
        # header-library already implies this; stated explicitly so the intent
        # survives a reader who does not know that rule.
        self.info.clear()

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

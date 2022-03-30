from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration
import shutil
import os
class IconConan(ConanFile):
    name = "icon"
    version = "0.1"
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Icon here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {
      "shared": [True, False],
      "fPIC": [True, False],
      "enable_testing": [True, False]
    }
    default_options = {
      "shared": False,
      "fPIC": True,
      "enable_testing" : False
    }
    generators = [
      "cmake",
      "cmake_find_package",
      "cmake_paths"
    ]
    exports_sources = [
      "conan/CMakeLists.txt",
      "cmake/*",
      "icon/*",
      "CMakeLists.txt",
      "tests/*"
    ]
    requires = [
      "boost/1.78.0",
      "cppzmq/4.8.1",
      "protobuf/3.19.1",
      "spdlog/1.9.2",
    ]

    def requirements(self):
      if self.options.enable_testing:
        self.requires("catch2/3.0.0@bhome/testing")

    def source(self):
      shutil.move("CMakeLists.txt", "CMakeListsOriginal.txt")
      shutil.move(os.path.join("conan", "CMakeLists.txt"), "CMakeLists.txt")

    def config_options(self):
      if self.settings.os == "Windows":
          del self.options.fPIC

    def validate(self):
        if self.settings.os == "Windows":
            raise ConanInvalidConfiguration("Windows not supported")

        if self.settings.compiler == "gcc" and tools.Version(self.settings.compiler.version) < "10":
            raise ConanInvalidConfiguration("GCC version must be >= 10")

        if self.settings.compiler == "clang" and tools.Version(self.settings.compiler.version) <= "13":
            raise ConanInvalidConfiguration("Clang version must be >= 13")

    def build(self):
        cmake = CMake(self)
        cmake.definitions["ENABLE_TESTING"] = self.options.enable_testing
        cmake.configure()
        cmake.build()

        if (self.options.enable_testing):
          cmake.test();

    def package(self):
        self.copy("*.hpp", src="icon", dst="include")
        self.copy("*.proto", src="icon", dst="include")
        self.copy("*.pb.h", src="icon/icon", dst="include")
        self.copy("*.a", dst="lib")

    def package_info(self):
        self.cpp_info.libs = ["icon"]

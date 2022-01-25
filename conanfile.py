from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration

class IconConan(ConanFile):
    name = "icon"
    version = "0.1"
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Icon here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False], }
    default_options = {"shared": False, "fPIC": True}
    generators = ["cmake", "cmake_find_package"]
    exports_sources = ["cmake/*", "src/*", "CMakeLists.txt"]
    requires = ["boost/1.78.0", "cppzmq/4.8.1", "protobuf/3.19.2", "spdlog/1.9.2", "catch2/2.13.8"]

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
   
    def validate(self):
        print (self.settings.compiler)
        print (tools.Version(self.settings.compiler.version))

        if self.settings.os == "Windows":
            raise ConanInvalidConfiguration("Windows not supported")
        
        if self.settings.compiler == "gcc" and tools.Version(self.settings.compiler.version) < "10":
            raise ConanInvalidConfiguration("GCC must be >= 10")

        if self.settings.compiler == "clang" and tools.Version(self.settings.compiler.version) <= "13":
            raise ConanInvalidConfiguration("Clang musg be >= 13")

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder=".")
        cmake.build()

        # Explicit way:
        # self.run('cmake %s/hello %s'
        #          % (self.source_folder, cmake.command_line))
        # self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("*.h", dst="include", src="src")
        self.copy("*.hpp", dst="include", src="src")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["icon"]

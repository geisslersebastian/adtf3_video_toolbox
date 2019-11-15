from conans import ConanFile, tools, CMake
import os

class ADTF3VideoToolbox(ConanFile):
    name = "ADTF3VideoToolbox"
    version = "0.1.0"
    
    settings = "os", "compiler", "build_type", "arch"
    description = "%s" % (name)
    generators = "cmake", "txt", "virtualenv"
    short_paths = True
    keep_imports = True
    no_copy_source = True
    enable_multiconfig_package = False

    def build_requirements(self):
        self.build_requires("cmake_installer/3.15.5@conan/stable")
        self.build_requires("ADTF/3.6.2@dw/stable")

    def source(self):
        self.run("git clone https://https://github.com/geisslersebastian/adtf3_video_toolbox.git")

    def build(self):
        cmake = CMake(self)
        # same as cmake.configure(source_folder=self.source_folder, build_folder=self.build_folder)
        cmake.configure()
        cmake.build()
        #cmake.test() # Build the "RUN_TESTS" or "test" target
        # Build the "install" target, defining CMAKE_INSTALL_PREFIX to self.package_folder
        cmake.install()
    


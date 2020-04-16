from conans import ConanFile, tools, CMake
import os

class ADTF3VideoToolbox(ConanFile):
    name = "adtf_video_toolbox"
    version = "0.1.0"
    
    settings = "os", "compiler", "build_type", "arch"
    description = "%s" % (name)
    generators = "cmake", "txt", "virtualenv"
    short_paths = True
    keep_imports = True
    no_copy_source = True
    enable_multiconfig_package = False

    def build_requirements(self):
        self.build_requires("opencv/4.1.0@test/test")
        self.build_requires("ADTF/3.7.0@dw/integration")

    def source(self):
        self.run("git clone https://github.com/geisslersebastian/adtf3_video_toolbox.git source")

    def build(self):
        cmake = CMake(self)
        # same as cmake.configure(source_folder=self.source_folder, build_folder=self.build_folder)
        cmake.configure(source_folder="source")
        cmake.build()
        #cmake.test() # Build the "RUN_TESTS" or "test" target
        # Build the "install" target, defining CMAKE_INSTALL_PREFIX to self.package_folder
        cmake.install()
    


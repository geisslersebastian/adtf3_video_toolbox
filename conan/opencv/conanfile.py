from conans import ConanFile, tools, CMake
import os

class OpenCV(ConanFile):
    version = "4.1.0"
    name = "opencv"
    settings = "os", "compiler", "build_type", "arch"
    description = "%s" % (name)
    generators = "cmake", "txt", "virtualenv"
    short_paths = True

    def source(self):
        self.run("git clone https://github.com/opencv/opencv")

    def build(self):
        cmake = CMake(self)
        # same as cmake.configure(source_folder=self.source_folder, build_folder=self.build_folder)
        cmake.configure(source_folder="opencv")
        cmake.build()
        #cmake.test() # Build the "RUN_TESTS" or "test" target
        # Build the "install" target, defining CMAKE_INSTALL_PREFIX to self.package_folder
        cmake.install()
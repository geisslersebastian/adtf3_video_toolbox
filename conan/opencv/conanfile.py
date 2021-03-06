from conans import ConanFile, tools, CMake
import os

class OpenCV(ConanFile):
    version = "4.1.0"
    name = "opencv"
    description = "%s" % (name)
    short_paths = True
    license = "MPL 2.0"
    generators = "cmake"
    settings = {"os": ["Windows", "Linux"], "arch": ["x86_64"], "compiler": ["Visual Studio", "gcc"], "build_type": ["Debug", "Release"]}
    
    options = {"cuda": [True, False]}
    default_options = {"cuda": True}
    
    no_copy_source = True
    cmake = None
    
    #scm = {
    #    "type": "git",
    #    "url": "https://github.com/opencv/opencv",
    #    "revision": "master"
    #}   

    def source(self):
        self.run("git clone https://github.com/opencv/opencv.git "+self.source_folder+"/source")
        self.run("git clone https://github.com/opencv/opencv_contrib.git "+self.source_folder+"/opencv_contrib")
    
    def build(self):
        cmake = CMake(self)
        # same as cmake.configure(source_folder=self.source_folder, build_folder=self.build_folder)
        
        if self.options.cuda:
            cmake.definitions["WITH_CUDA"] = "ON"
            cmake.definitions["WITH_CUDNN"] = "ON"
            cmake.definitions["OPENCV_DNN_CUDE"] = "ON"
            cmake.definitions["OPENCV_EXTRA_MODULES_PATH"] = self.source_folder + "/opencv_contrib/modules"
        cmake.configure(source_folder=self.source_folder+"/source")
        cmake.build()
        #cmake.test() # Build the "RUN_TESTS" or "test" target
        # Build the "install" target, defining CMAKE_INSTALL_PREFIX to self.package_folder
        cmake.install()
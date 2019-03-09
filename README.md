# Summary
Compile Tensorflow 1.12 C++ API from source on a NVIDIA Jetson Xavier system

# Hardware requirement
- NVIDIA Jetson Xavier Developer Kit
- Host computer running Ubuntu Linux x64 Version 18.04 or 16.04
- Internet connection to both system

# 1. Install Jetpack
If Xavier is freshly out of the box, several developer software need to be installed to it, including CUDA and cuDNN. NVIDIA provided Jetpack as a bundle to the developers. Following the [official installation guide](https://developer.nvidia.com/embedded/downloads#?search=JetPack%204.1.1) provided by NVIDIA to install Jetpack 4.1.1 to it. It requires a host computer along with a internet connection between them to install the whole package.

run `nvcc --version` in a terminal should tell that the current CUDA version is 10.0.

# 2. Build Bazel
Bazel is required to build tensorflow from source. However, there is no official binary installer for Bazel on arm64 CPU architecture. So we have to build it from source. Check [tensorflow website](https://www.tensorflow.org/install/source) for the appropriate Bazel version for each individual tensorflow version. For tensorflow 1.12.0, Bazel 0.15.0 was tested by the tensorflow team. Although personally I tested Bazel 0.19.2 and also succeed.
Get Java
```
sudo apt-get install openjdk-8-jdk
```
Start download and build bazel 0.19.2 on **~/src** folder
```
cd ~/Downloads
wget https://github.com/bazelbuild/bazel/releases/download/0.15.2/bazel-0.19.2-dist.zip
mkdir -p ~/src
cd ~/src
unzip ~/Downloads/bazel-0.19.2-dist.zip -d bazel-0.19.2-dist
cd bazel-0.19.2-dist
./compile.sh
sudo cp output/bazel /usr/local/bin
```
Run `bazel help` to confirm the successful installation of bazel

# 3. Build tensorflow
## 3.1 Get tensorflow
Get tensorflow 1.12.0 from source
```
git clone --recursive https://github.com/tensorflow/tensorflow
cd tensorflow
git checkout v1.12.0-rc1
```
You can copy the source code to **~/src** folder if you feel necessary  
Side note:
Although tensorflow 1.13.0 is the official build that supports CUDA 10.0, but I encountered some difficulties when editing the tool chain of it, so I did not proceed and use 1.12.0-rc1 instead.

## 3.2 Modify toolchain
If you directly starts to build tensorflow as other linux x86_64 machine, you would encounter an [issue](https://github.com/tensorflow/tensorflow/issues/21852) with missing toolchain for "aarch64". 
Open **./third_party/gpus/crosstool/CROSSTOOL.tpl.** file with text editor (gedit). In tensorflow directory,
```
cd ~/tensorflow/third_party/gpus/crosstool
sudo gedit CROSSTOOL.tpl
```
Add the following content after a series of similar `default_toolchain` blocks and save the file
```
default_toolchain {
  cpu: "aarch64"
  toolchain_identifier: "local_linux"
}
```
In tensorflow 1.13.0, the format of CROSSTOOL.tpl was changed and so far I have no luck on finding out how to achieve the same objective.

## 3.3 Build third-party dependencies
### Addressing protobuf issue
Here comes with another pit-hole, protobuf version mismatch. In `/{tensorflow_root}/tensorflow/workspace.bzl` file, the protobuf version is defined as **v3.6.0**, as seen by 
```
    PROTOBUF_URLS = [
        "https://mirror.bazel.build/github.com/google/protobuf/archive/v3.6.0.tar.gz",
        "https://github.com/google/protobuf/archive/v3.6.0.tar.gz",
    ]
    PROTOBUF_SHA256 = "50a5753995b3142627ac55cfd496cebc418a2e575ca0236e29033c67bd5665f4"
    PROTOBUF_STRIP_PREFIX = "protobuf-3.6.0"
```
However, in the shell file we are going to run, protobuf was downloaded as **v3.5.0** (in **/{tensorflow_root}/tensorflow/contrib/makefile/download_dependencies.sh**) 
```
# Note: The Protobuf source in `tensorflow/workspace.bzl` in TensorFlow
# 1.10 branch does not work. `make distclean` fails and blocks the build
# process. For now we're hardcoding to the version which is used by
# TensorFlow 1.9.
PROTOBUF_URL="https://mirror.bazel.build/github.com/google/protobuf/archive/396336eb961b75f03b25824fe86cf6490fb75e3a.tar.gz"
```
Edit **download_dependencies.sh** file with the correct 3.6.0 link ("https://mirror.bazel.build/github.com/google/protobuf/archive/v3.6.0.tar.gz") should work. But I discovered this later so I built my own protobuf instead. 

### Compile from source
In tensorflow root directory,
```
cd tensorflow/contrib/makefile
./build_all_linux.sh
```
The entire process would take several (~3) hours, and packages like protobuf, eigen, and absl would be built from source

## 3.4 Build tensorflow from root
In tensorflow root directory, configure the build options
```
./configure
```
The following options was used by mine, pay special attention to:
CUDA version: 10.0
cuDNN location: /usr/lib/aarch64-linux-gnu
compute capability: 7.2 for xavier
```
Please specify the location of python. [Default is /usr/bin/python]: /usr/bin/python3
   
   
Found possible Python library paths:
  /usr/local/lib/python3.5/dist-packages
  /usr/lib/python3/dist-packages
Please input the desired Python library path to use.  Default is [/usr/local/lib/python3.5/dist-packages]
   
Do you wish to build TensorFlow with jemalloc as malloc support? [Y/n]: n
   
Do you wish to build TensorFlow with Google Cloud Platform support? [Y/n]: n
   
Do you wish to build TensorFlow with Hadoop File System support? [Y/n]: n
   
Do you wish to build TensorFlow with Amazon S3 File System support? [Y/n]: n
   
Do you wish to build TensorFlow with Apache Kafka Platform support? [Y/n]: n
   
Do you wish to build TensorFlow with XLA JIT support? [y/N]: n
   
Do you wish to build TensorFlow with GDR support? [y/N]: n
   
Do you wish to build TensorFlow with VERBS support? [y/N]: n
   
Do you wish to build TensorFlow with OpenCL SYCL support? [y/N]: n
   
Do you wish to build TensorFlow with CUDA support? [y/N]: y
   
Please specify the CUDA SDK version you want to use, e.g. 7.0. [Leave empty to default to CUDA 9.0]: 10.0
   
Please specify the location where CUDA 9.0 toolkit is installed. Refer to README.md for more details. [Default is /usr/local/cuda]: 
   
Please specify the cuDNN version you want to use. [Leave empty to default to cuDNN 7.0]: 7.3.2
   
Please specify the location where cuDNN 7 library is installed. Refer to README.md for more details. [Default is /usr/local/cuda]:
/usr/lib/aarch64-linux-gnu
   
Do you wish to build TensorFlow with TensorRT support? [y/N]: n
TensorRT support will be enabled for TensorFlow.
   
Please specify the location where TensorRT is installed. [Default is /usr/lib/aarch64-linux-gnu]:

Please specify the NCCL version you want to use. [Leave empty to default to NCCL 1.3]: 1.3
   
   
Please specify a list of comma-separated Cuda compute capabilities you want to build with.
You can find the compute capability of your device at: https://developer.nvidia.com/cuda-gpus.
Please note that each additional compute capability significantly increases your build time and binary size. [Default is: 3.5,5.2] 7.2
   
Do you want to use clang as CUDA compiler? [y/N]: n
nvcc will be used as CUDA compiler.
   
Please specify which gcc should be used by nvcc as the host compiler. [Default is /usr/bin/gcc]: 
 
Do you wish to build TensorFlow with MPI support? [y/N]: n
   
Please specify optimization flags to use during compilation when bazel option "--config=opt" is specified [Default is -march=native]: 
   
Would you like to interactively configure ./WORKSPACE for Android builds? [y/N]: 
Not configuring the WORKSPACE for Android builds.
   
Preconfigured Bazel build configs. You can use any of the below by adding "--config=<>" to your build command. See tools/bazel.rc for more details.
	--config=mkl         	# Build with MKL support.
	--config=monolithic  	# Config for mostly static monolithic build.
Configuration finished
```
### Addressing AWS issue
Yay! Another pit-hole on our road! If you started to build with bazel after configuration, you might have an [issue](https://github.com/tensorflow/serving/issues/832) alert you that `undefined reference to Aws::FileSystem::CreateTempFilePath[abi:cxx11]()'`

The fix is indicated in the link, go to **/home/<user_name>/.cache/bazel/_bazel_<user_name>/<hash>/external/aws/BUILD.bazel**, where <user_name> - user current linux user name, and <hash> is hash like de4a7858eac0c7de37e543fdc903ef12. You may not have this file so far if you have not started the building (I'm not sure). But if you have, in section (cc_library) line 27 replace: `"//conditions:default": []"` with `"//conditions:default": glob(["aws-cpp-sdk-core/source/platform/linux-shared/*.cpp",]),`.

### Start to build tensorflow C++ API
In tensorflow root directory
```
bazel build //tensorflow:libtensorflow_cc.so
```
The while process took 23000s (~8 hours) in my Xavier.
By the way, if you want to use C API, build **tensorflow/libtensorflow.so**, if C++ API, use **tensorflow/libtensorflow_cc.so**.
If you need python wheel, you can build **tensorflow/tools/pip_package:build_pip_package**, but there is already an official one on NVIDIA forum [tensorflow-gpu 1.13.0 for xavier](https://devtalk.nvidia.com/default/topic/1042125/jetson-agx-xavier/official-tensorflow-for-jetson-agx-xavier/).

### Install tensorflow library
make directory
```
sudo mkdir /usr/local/tensorflow
sudo mkdir /usr/local/tensorflow/include
```
copy the header files
```
sudo cp -r tensorflow/contrib/makefile/downloads/eigen/Eigen /usr/local/tensorflow/include/
sudo cp -r tensorflow/contrib/makefile/downloads/eigen/unsupported /usr/local/tensorflow/include/
sudo cp -r tensorflow/contrib/makefile/downloads/absl/absl /usr/local/tensorflow/include/
# if your protobuf is 3.6.0 by changing the download file
sudo cp -r tensorflow/contrib/makefile/gen/protobuf/include/google /usr/local/tensorflow/include/
sudo cp tensorflow/contrib/makefile/downloads/nsync/public/* /usr/local/tensorflow/include/
sudo cp -r bazel-genfiles/tensorflow /usr/local/tensorflow/include/
sudo cp -r tensorflow/cc /usr/local/tensorflow/include/tensorflow
sudo cp -r tensorflow/core /usr/local/tensorflow/include/tensorflow
sudo mkdir /usr/local/tensorflow/include/third_party
sudo cp -r third_party/eigen3 /usr/local/tensorflow/include/third_party/
```
Copy library files
```
sudo mkdir /usr/local/tensorflow/lib
sudo cp bazel-bin/tensorflow/libtensorflow_*.so /usr/local/tensorflow/lib
```
## 4. Testing tensorflow C++ API
Try a sample C++ file like main.cc provided by [source](https://github.com/hemajun815/tutorial/blob/master/tensorflow/training-a-DNN-using-only-tensorflow-cc.md)
```
#include "tensorflow/cc/client/client_session.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/cc/framework/gradients.h"

int main()
{
    // benchmark
    auto benchmark_w = 2.0, benchmark_b = 0.5;

    // data
    auto nof_samples = 100;
    struct Sample 
    {
        float sample;
        float label;
    };
    std::vector<struct Sample> dataset;
    std::srand((unsigned)std::time(NULL));
    for (int i = 0; i < nof_samples; i++)
    {
        float sample = std::rand() / float(RAND_MAX) - 0.5;
        float label = benchmark_w * sample + benchmark_b + std::rand() / float(RAND_MAX) * 0.01;
        dataset.push_back({sample, label});
    }

    // model
    tensorflow::Scope root = tensorflow::Scope::NewRootScope();
    auto x = tensorflow::ops::Placeholder(root, tensorflow::DataType::DT_FLOAT);
    auto y = tensorflow::ops::Placeholder(root, tensorflow::DataType::DT_FLOAT);
    auto w = tensorflow::ops::Variable(root, {1, 1}, tensorflow::DataType::DT_FLOAT);
    auto assign_w = tensorflow::ops::Assign(root, w, tensorflow::ops::RandomNormal(root, {1, 1}, tensorflow::DataType::DT_FLOAT));
    auto b = tensorflow::ops::Variable(root, {1, 1}, tensorflow::DataType::DT_FLOAT);
    auto assign_b = tensorflow::ops::Assign(root, b, {{0.0f}});
    auto y_ = tensorflow::ops::Add(root, tensorflow::ops::MatMul(root, x, w), b);
    auto loss = tensorflow::ops::L2Loss(root, tensorflow::ops::Sub(root, y_, y));
    std::vector<tensorflow::Output> grad_outputs;
    TF_CHECK_OK(AddSymbolicGradients(root, {loss}, {w, b}, &grad_outputs));
    auto learn_rate = tensorflow::ops::Const(root, 0.01f, {});
    auto apply_w = tensorflow::ops::ApplyGradientDescent(root, w, learn_rate, {grad_outputs[0]});
    auto apply_b = tensorflow::ops::ApplyGradientDescent(root, b, learn_rate, {grad_outputs[1]});

    // train
    tensorflow::ClientSession sess(root);
    sess.Run({assign_w, assign_b}, nullptr);
    std::vector<tensorflow::Tensor> outputs;
    timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int epoch = 1; epoch <= 64; epoch++)
    {
        std::random_shuffle(dataset.begin(), dataset.end());
        for (int i = 0; i < nof_samples; i++)
        {
            TF_CHECK_OK(sess.Run({{x, {{dataset[i].sample}}}, {y, {{dataset[i].label}}}}, {w, b, loss, apply_w, apply_b}, &outputs));
        }
        LOG(INFO) << "epoch " << epoch << ": w=" << outputs[0].matrix<float>() << " b=" << outputs[1].matrix<float>() << " loss=" << outputs[2].scalar<float>();
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    LOG(INFO) << "elapsed time： " << t1.tv_sec - t0.tv_sec + (t1.tv_nsec - t0.tv_nsec) * 1.0 / 1000000000 << "s";
    return 0;
}
```
or official example.cc [source](https://www.tensorflow.org/guide/extend/cc)
```
// tensorflow/cc/example/example.cc

#include "tensorflow/cc/client/client_session.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/tensor.h"

int main() {
  using namespace tensorflow;
  using namespace tensorflow::ops;
  Scope root = Scope::NewRootScope();
  // Matrix A = [3 2; -1 0]
  auto A = Const(root, { {3.f, 2.f}, {-1.f, 0.f} });
  // Vector b = [3 5]
  auto b = Const(root, { {3.f, 5.f} });
  // v = Ab^T
  auto v = MatMul(root.WithOpName("v"), A, b, MatMul::TransposeB(true));
  std::vector<Tensor> outputs;
  ClientSession session(root);
  // Run and fetch v
  TF_CHECK_OK(session.Run({v}, &outputs));
  // Expect outputs[0] == [19; -3]
  LOG(INFO) << outputs[0].matrix<float>();
  return 0;
}
```
Both files can be compiled from any locations:
```
g++ -std=c++11 ./main.cc -ltensorflow_framework -ltensorflow_cc -o tfcc \
-I/usr/local/tensorflow/include/ \
-L/usr/local/tensorflow/lib/ \
-Wl,-rpath=/usr/local/tensorflow/lib/
```

Here is my example output of aforementioned **main.cc**
```
nvidia@jetson-0423418010444:~/Documents/cpp_project$ ./tfcc
2019-03-07 16:41:17.469494: I tensorflow/stream_executor/cuda/cuda_gpu_executor.cc:931] ARM64 does not support NUMA - returning NUMA node zero
2019-03-07 16:41:17.469989: I tensorflow/core/common_runtime/gpu/gpu_device.cc:1432] Found device 0 with properties: 
name: Xavier major: 7 minor: 2 memoryClockRate(GHz): 1.5
pciBusID: 0000:00:00.0
totalMemory: 15.45GiB freeMemory: 1.10GiB
2019-03-07 16:41:17.470150: I tensorflow/core/common_runtime/gpu/gpu_device.cc:1511] Adding visible gpu devices: 0
2019-03-07 16:41:19.059567: I tensorflow/core/common_runtime/gpu/gpu_device.cc:982] Device interconnect StreamExecutor with strength 1 edge matrix:
2019-03-07 16:41:19.059842: I tensorflow/core/common_runtime/gpu/gpu_device.cc:988]      0 
2019-03-07 16:41:19.059982: I tensorflow/core/common_runtime/gpu/gpu_device.cc:1001] 0:   N 
2019-03-07 16:41:19.060585: I tensorflow/core/common_runtime/gpu/gpu_device.cc:1115] Created TensorFlow device (/job:localhost/replica:0/task:0/device:GPU:0 with 589 MB memory) -> physical GPU (device: 0, name: Xavier, pci bus id: 0000:00:00.0, compute capability: 7.2)
2019-03-07 16:41:21.223866: I ./main.cc:55] epoch 1: w=0.751655 b=0.29391 loss=0.0274053
2019-03-07 16:41:21.542139: I ./main.cc:55] epoch 2: w=0.846608 b=0.406805 loss=0.164987
2019-03-07 16:41:21.826643: I ./main.cc:55] epoch 3: w=0.941089 b=0.448278 loss=0.00171172
2019-03-07 16:41:22.117194: I ./main.cc:55] epoch 4: w=1.02611 b=0.466717 loss=0.00212118
2019-03-07 16:41:22.421440: I ./main.cc:55] epoch 5: w=1.10209 b=0.488073 loss=0.0786828
2019-03-07 16:41:22.725282: I ./main.cc:55] epoch 6: w=1.17624 b=0.479675 loss=0.00696869
2019-03-07 16:41:23.001284: I ./main.cc:55] epoch 7: w=1.24259 b=0.485361 loss=0.00326194
2019-03-07 16:41:23.289471: I ./main.cc:55] epoch 8: w=1.303 b=0.484753 loss=0.0274161
2019-03-07 16:41:23.565903: I ./main.cc:55] epoch 9: w=1.35979 b=0.486861 loss=0.00443319
2019-03-07 16:41:23.839075: I ./main.cc:55] epoch 10: w=1.41042 b=0.487121 loss=0.0335449
2019-03-07 16:41:24.112628: I ./main.cc:55] epoch 11: w=1.45836 b=0.493528 loss=0.0132444
2019-03-07 16:41:24.392869: I ./main.cc:55] epoch 12: w=1.50249 b=0.493722 loss=0.000421004
2019-03-07 16:41:24.673248: I ./main.cc:55] epoch 13: w=1.54214 b=0.492729 loss=0.0124819
2019-03-07 16:41:24.943316: I ./main.cc:55] epoch 14: w=1.57859 b=0.492936 loss=0.0205041
2019-03-07 16:41:25.215525: I ./main.cc:55] epoch 15: w=1.61338 b=0.494677 loss=0.00140292
2019-03-07 16:41:25.486765: I ./main.cc:55] epoch 16: w=1.6444 b=0.495493 loss=0.00356482
2019-03-07 16:41:25.746055: I ./main.cc:55] epoch 17: w=1.67282 b=0.494353 loss=0.00804859
2019-03-07 16:41:26.002922: I ./main.cc:55] epoch 18: w=1.69897 b=0.49576 loss=0.00925522
2019-03-07 16:41:26.231166: I ./main.cc:55] epoch 19: w=1.72344 b=0.497813 loss=0.00412729
2019-03-07 16:41:26.467576: I ./main.cc:55] epoch 20: w=1.74556 b=0.496992 loss=0.00545393
2019-03-07 16:41:26.694135: I ./main.cc:55] epoch 21: w=1.76627 b=0.500005 loss=0.000584862
2019-03-07 16:41:26.922207: I ./main.cc:55] epoch 22: w=1.78507 b=0.499904 loss=0.000940883
2019-03-07 16:41:27.145010: I ./main.cc:55] epoch 23: w=1.80216 b=0.499362 loss=0.00290919
2019-03-07 16:41:27.385359: I ./main.cc:55] epoch 24: w=1.81826 b=0.500923 loss=0.000320149
2019-03-07 16:41:27.620054: I ./main.cc:55] epoch 25: w=1.83263 b=0.499837 loss=0.00290858
2019-03-07 16:41:27.834869: I ./main.cc:55] epoch 26: w=1.8463 b=0.501314 loss=0.00032206
2019-03-07 16:41:28.046506: I ./main.cc:55] epoch 27: w=1.85856 b=0.501068 loss=0.00116968
2019-03-07 16:41:28.277158: I ./main.cc:55] epoch 28: w=1.86988 b=0.501059 loss=0.00112716
2019-03-07 16:41:28.498097: I ./main.cc:55] epoch 29: w=1.88047 b=0.500762 loss=0.00014708
2019-03-07 16:41:28.709691: I ./main.cc:55] epoch 30: w=1.88993 b=0.501637 loss=0.000818347
2019-03-07 16:41:28.928918: I ./main.cc:55] epoch 31: w=1.89886 b=0.50171 loss=0.000140877
2019-03-07 16:41:29.147404: I ./main.cc:55] epoch 32: w=1.9067 b=0.502913 loss=0.00112039
2019-03-07 16:41:29.367084: I ./main.cc:55] epoch 33: w=1.91438 b=0.502272 loss=8.76888e-05
2019-03-07 16:41:29.581509: I ./main.cc:55] epoch 34: w=1.92122 b=0.503096 loss=9.92497e-06
2019-03-07 16:41:29.783177: I ./main.cc:55] epoch 35: w=1.9275 b=0.502563 loss=6.96661e-05
2019-03-07 16:41:29.999185: I ./main.cc:55] epoch 36: w=1.93322 b=0.503206 loss=0.000259633
2019-03-07 16:41:30.208383: I ./main.cc:55] epoch 37: w=1.93856 b=0.504136 loss=8.23007e-05
2019-03-07 16:41:30.422683: I ./main.cc:55] epoch 38: w=1.94346 b=0.503883 loss=6.48337e-05
2019-03-07 16:41:30.630648: I ./main.cc:55] epoch 39: w=1.94798 b=0.503872 loss=2.32864e-05
2019-03-07 16:41:30.839528: I ./main.cc:55] epoch 40: w=1.95209 b=0.503324 loss=8.82509e-05
2019-03-07 16:41:31.046315: I ./main.cc:55] epoch 41: w=1.95593 b=0.503888 loss=6.81071e-08
2019-03-07 16:41:31.259884: I ./main.cc:55] epoch 42: w=1.95942 b=0.503865 loss=1.0062e-08
2019-03-07 16:41:31.463522: I ./main.cc:55] epoch 43: w=1.96256 b=0.504159 loss=0.000160442
2019-03-07 16:41:31.664602: I ./main.cc:55] epoch 44: w=1.96555 b=0.504346 loss=7.69988e-05
2019-03-07 16:41:31.870408: I ./main.cc:55] epoch 45: w=1.96821 b=0.50445 loss=0.000138752
2019-03-07 16:41:32.063447: I ./main.cc:55] epoch 46: w=1.97076 b=0.504565 loss=2.90031e-05
2019-03-07 16:41:32.254081: I ./main.cc:55] epoch 47: w=1.97305 b=0.504098 loss=4.98831e-05
2019-03-07 16:41:32.472025: I ./main.cc:55] epoch 48: w=1.97515 b=0.504518 loss=4.20674e-05
2019-03-07 16:41:32.671546: I ./main.cc:55] epoch 49: w=1.9771 b=0.504679 loss=2.87651e-05
2019-03-07 16:41:32.860057: I ./main.cc:55] epoch 50: w=1.97888 b=0.504727 loss=1.25812e-05
2019-03-07 16:41:33.066668: I ./main.cc:55] epoch 51: w=1.98052 b=0.504632 loss=4.54919e-06
2019-03-07 16:41:33.265434: I ./main.cc:55] epoch 52: w=1.98199 b=0.504659 loss=5.72449e-05
2019-03-07 16:41:33.464912: I ./main.cc:55] epoch 53: w=1.98341 b=0.504865 loss=1.27067e-06
2019-03-07 16:41:33.668524: I ./main.cc:55] epoch 54: w=1.98469 b=0.50482 loss=3.59992e-08
2019-03-07 16:41:33.863386: I ./main.cc:55] epoch 55: w=1.98587 b=0.504924 loss=1.81591e-06
2019-03-07 16:41:34.048780: I ./main.cc:55] epoch 56: w=1.98691 b=0.504967 loss=3.36097e-05
2019-03-07 16:41:34.239706: I ./main.cc:55] epoch 57: w=1.98793 b=0.50514 loss=5.60373e-06
2019-03-07 16:41:34.437763: I ./main.cc:55] epoch 58: w=1.98882 b=0.50514 loss=2.03703e-05
2019-03-07 16:41:34.630575: I ./main.cc:55] epoch 59: w=1.98966 b=0.505059 loss=1.30769e-05
2019-03-07 16:41:34.821846: I ./main.cc:55] epoch 60: w=1.99043 b=0.505072 loss=1.63617e-05
2019-03-07 16:41:35.018024: I ./main.cc:55] epoch 61: w=1.99115 b=0.505079 loss=6.36616e-07
2019-03-07 16:41:35.201758: I ./main.cc:55] epoch 62: w=1.99177 b=0.505054 loss=1.19506e-05
2019-03-07 16:41:35.404581: I ./main.cc:55] epoch 63: w=1.99239 b=0.505126 loss=6.05516e-06
2019-03-07 16:41:35.594773: I ./main.cc:55] epoch 64: w=1.99294 b=0.505213 loss=4.29734e-06
2019-03-07 16:41:35.595074: I ./main.cc:58] elapsed time： 16.2057s
nvidia@jetson-0423418010444:~/Documents/cpp_project$ ^C
nvidia@jetson-0423418010444:~/Documents/cpp_project$ 
```

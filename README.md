# MapReduceFramework
A C++ framework implementing the MapReduce programming model with multi-threading support,
designed for efficient parallel data processing.

# 📘 Overview
This project was developed by [**Noam Kimhi**](https://github.com/noam-kimhi) and [**Or Forshmit**](https://github.com/OrF8) as part of the course
[**67808 - Operating Systems**](https://shnaton.huji.ac.il/index.php/NewSyl/67808/2/2025/) at
The Hebrew University of Jerusalem ([HUJI](https://en.huji.ac.il/)).\
The framework enables the distribution of data processing tasks across multiple threads,
allowing for scalable and efficient computation of large datasets.
 
# 🧾 Table of Contents
- [📘 Overview](https://github.com/OrF8/MapReduceFramework?tab=readme-ov-file#-Overview)
- [🧾 Table of Contents](https://github.com/OrF8/MapReduceFramework?tab=readme-ov-file#-Table-of-Contents)
- [⚙️ Features](https://github.com/OrF8/MapReduceFramework?tab=readme-ov-file#%EF%B8%8F-features)
- [🛠️ Requirements](https://github.com/OrF8/MapReduceFramework?tab=readme-ov-file#%EF%B8%8F-requirements)
- [📦 Installation](https://github.com/OrF8/MapReduceFramework?tab=readme-ov-file#-installation)
- [🚀 Usage](https://github.com/OrF8/MapReduceFramework?tab=readme-ov-file#-usage)
- [🗂️ Project Structure](https://github.com/OrF8/MapReduceFramework?tab=readme-ov-file#%EF%B8%8F-project-structure)
- [📄 License](https://github.com/OrF8/MapReduceFramework?tab=readme-ov-file#-license)

# ⚙️ Features
- **Multithreaded Execution**: Processes data in parallel, leveraging multiple CPU cores.
- **MapReduce Paradigm**: Supports the standard `Map` and `Reduce` functions for data transformation and aggregation.
- **Modular Design**: Easily extendable and adaptable to various data processing tasks.

# 🛠️ Requirements
- C++20 or higher
- CMake 3.10 or higher (if using the provided `CMakeLists.txt`)
- GNU Make (if you use the provided `Makefile`)
- A C++ compiler supporting C++20 (e.g., GCC, Clang, MSVC)

# 📦 Installation
1. Clone the repository:
    ```
    git clone https://github.com/OrF8/MapReduceFramework.git
   ```
2. Navigate to the project directory:
   ```
   cd MapReduceFramework
   ```
3. Create a build directory:
    ```
    mkdir build
    ```
4. Navigate to the build directory:
    ```
    cd build
    ```
5. Build the project:
   - Using Cmake:
     ```
     cmake ..
     make
     ```
   - Or using GNU Makefile:
     ```
     make
     ```
   This will create a library file `libMapReduceFramework.a` in the `build/` directory.
6. Additional Make and CMake targets are available, such as `tar`.

# 🚀 Usage
To use the MapReduce framework, follow these steps:
1. Implement your own `Map` and `Reduce` functors by inheriting from the provided interfaces in `include/`.
2. Call the `startMapReduceJob` function with your data.
3. An example of how to use the framework can be found in the `examples/` directory.
   - To run the example with CMake:
     ```
     mkdir build
     cd build
     cmake ..
     make
     ./SampleClient (or ./SampleClient.exe on Windows)
     ```
   - Or using GNU Makefile:
     ```
     make runSampleClient
     ```
     
# 🗂️ Project Structure
  ```
  .
  ├── example/              # Sample jobs (e.g. char count)
  │   └── SampleClient.cpp
  ├── include/              # Public headers (MapReduceFramework API)
  │   ├── Barrier.h
  │   ├── JobStateManager.h
  │   ├── MapReduceClient.h
  │   └── MapReduceFramework.h
  ├── src/                  # Framework implementation
  │   ├── Barrier.cpp
  │   ├── JobStateManager.cpp
  │   └── MapeduceFramework.cpp
  ├── CMakeLists.txt        # CMake build script
  ├── Makefile              # Alternative Makefile build
  ├── LICENSE               # The license file
  └── README.md             # ← you are here
  ```

# 📄 License
This project is licensed under the MIT License-see the [**LICENSE**](https://github.com/OrF8/MapReduceFramework/blob/main/LICENSE) file for details.

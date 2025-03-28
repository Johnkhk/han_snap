# han_snap

## Build Instructions

### First Time Build Instructions

1. Ensure you have CMake installed (version 3.10 or higher).
2. Ensure you have wxWidgets installed.
3. Clone the repository:
   ```
   git clone https://github.com/yourusername/han_snap.git
   ```
4. Navigate to the project directory:
   ```
   cd han_snap
   ```
5. Create a build directory and navigate into it:
   ```
   mkdir build && cd build
   ```
6. Run CMake to configure the project:
   ```
   cmake ..
   ```
7. Build the project:
   ```
   cmake --build .
   ```

### After First Time Build Instructions

1. Navigate to the build directory:
   ```
   cd han_snap/build
   ```
2. Build the project:

   For Linux/MacOS:
   ```
   cmake --build . && ./han_snap_app
   ```

   For Windows:
   ```
   cmake --build . && .\Debug\han_snap_app.exe
   ```



### Just running LLM.cpp
```
g++ -o myapp src/llm.cpp -std=c++17 -I/opt/homebrew/include -lcurl
```

<!-- ## Testing

```
cmake -S . -B build && cmake --build build && ctest --test-dir build
``` -->
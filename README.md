# pylibcaption
Python wrapper for libcaption

## Building a wheel

Use `build_wheel.sh` to compile a platform wheel for a given Python version.

**Prerequisites:** `uv`, `cmake`, `make`, a C compiler (`cc`/`gcc`)

```bash
# Build for a specific Python version
./build_wheel.sh 3.12
./build_wheel.sh 3.8

# Build for the current Python
./build_wheel.sh
```

Output wheel will be under under `dist/`

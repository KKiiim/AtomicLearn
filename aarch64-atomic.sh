echo ===== armv8 =====
echo  answer
aarch64-linux-gnu-g++-11 -march=armv8-a -O0 src/test-std-atomic.cpp -o armv8Atomic
qemu-aarch64 -L /usr/aarch64-linux-gnu ./armv8Atomic

echo ...disassembly
/usr/aarch64-linux-gnu/bin/objdump -d ./armv8Atomic > armv8Atomic.log
rm armv8Atomic

echo ===== armv7 =====
echo  answer
arm-linux-gnueabihf-g++ -march=armv7-a -mfloat-abi=soft -O0 src/test-std-atomic.cpp -o armv7Atomic
qemu-arm -L /usr/arm-linux-gnueabihf ./armv7Atomic

echo ...disassembly
arm-linux-gnueabihf-objdump -d ./armv7Atomic > armv7Atomic.log
rm armv7Atomic
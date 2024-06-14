g++ -S src/barrier/compiler-barrier/complier-barrier.cpp -o compiler-barrier.asm
g++ -S -O1 src/barrier/compiler-barrier/complier-barrier.cpp -o O1-compiler-barrier.asm
g++ -S -O2 src/barrier/compiler-barrier/complier-barrier.cpp -o O2-compiler-barrier.asm
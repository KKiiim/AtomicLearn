echo =======-O0=======
g++ -std=c++20 -O0 src/x86-atomic.cpp -o x86Atomic && ./x86Atomic && rm x86Atomic
echo =======-O1=======
g++ -std=c++20 -O1 src/x86-atomic.cpp -o x86Atomic && ./x86Atomic && rm x86Atomic
echo =======-O2=======
g++ -std=c++20 -O2 src/x86-atomic.cpp -o x86Atomic && ./x86Atomic && rm x86Atomic
echo =======-O3=======
g++ -std=c++20 -O3 src/x86-atomic.cpp -o x86Atomic && ./x86Atomic && rm x86Atomic
echo =======-Os=======
g++ -std=c++20 -Os src/x86-atomic.cpp -o x86Atomic && ./x86Atomic && rm x86Atomic

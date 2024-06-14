#define barrier() __asm__ __volatile__("" ::: "memory")
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

int x, y, r;
void f() {
  x = r;
  __asm__ __volatile__("" ::: "memory");
  y = 1;
}

volatile int x1, y1;
int r1;
void f1() {
  x1 = r1;
  y1 = 1;
}

int x2, y2, r2;
void f2() {
  ACCESS_ONCE(x2) = r2;
  ACCESS_ONCE(y2) = 1;
}
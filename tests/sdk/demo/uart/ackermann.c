#define LOG MyPrintf

int ackermann(int m, int n) {
  if (m == 0) {
    return n + 1;
  } else if (m > 0 && n == 0) {
    return ackermann(m - 1, 1);
  } else {
    return ackermann(m - 1, ackermann(m, n - 1));
  }
}

void ackermann_test() {
for (int i = 0; i <= 3; ++i) {
    for (int j = 0; j <= 4; ++j) {
      int result = ackermann(i, j);
      LOG("Ackermann(%d, %d)=%d \n", i, j, result);
    }
  }
}
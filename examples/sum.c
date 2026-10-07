/* Sum 1 through 5, on a stack machine made of DOS batch files. */
int main(void) {
    int sum = 0;
    int i = 1;
    while (i <= 5) {
        sum = sum + i;
        i = i + 1;
    }
    if (sum == 15) {
        return sum;
    } else {
        return 255;
    }
}

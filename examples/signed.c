/* Signed arithmetic and short-circuit conditions: returns 9400. */
int main(void) {
    int total = 10000;
    int delta = -300;
    while (delta < 0 && total > 0) {
        total = total + delta;
        delta = delta + 100;
    }
    return total;
}

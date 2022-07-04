#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100

int DP[MAX + 1];
int fib(int n) {
	if (n > MAX)
		exit(1);
	if (n <= 2) {
		DP[n] = 1;
		return 1;
	}
	if (DP[n] != -1)
		return DP[n];
	DP[n] = fib(n - 1) + fib(n - 2);
	return DP[n];
}

int main() {
	memset(DP, -1, sizeof(DP));
	
	for (int i = 1; i <= 10; i++)
		printf("fib(%d) = %d\n", i, fib(i));

	return 0;
}

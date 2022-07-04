#include <stdio.h>
#include <math.h>

int solve(int door_count) {
	return (int)floor(sqrt((double)door_count));
}

int main() {
	for (int i = 1; i <= 100; i++)
		printf("solve(%d) = %d\n", i, solve(i));

	return 0;
}

#include<omp.h>
#include<stdio.h>
#include<unistd.h>

int main() {
	int x = 0;
    #pragma omp parallel for private(x)
	for (int i = 0; i < 12; i++) {
		x++;
		printf("thread %d: i = %d\n",
				omp_get_thread_num(), i);
		sleep(1);
	}
	printf("x = %d\n", x);
}
#include <stdio.h>
#include "apilib.h"

#define MAX	    1000

void HariMain(void) {
    char flag[MAX], s[8];
    int i, j;
    // 先将数组都初始化为0
    for (i = 0; i < MAX; i++) {
	flag[i] = 0;
    }
    for (i = 2; i < MAX; i++) {
	if (flag[i] == 0) {
	    // 没有标记的为素数
	    sprintf(s, "%d ", i);
	    api_putstr0(s);
	    for (j = i * 2; j < MAX; j += i) {
		flag[j] = 1;	// 给i的倍数作标记
	    }
	}
    }

    api_end();
}
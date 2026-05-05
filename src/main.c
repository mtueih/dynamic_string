#include <stdio.h>
#include <stdlib.h>
#include "dynamic_string.h"

int main(void) {
	dstr_adt *dstr = NULL;

	dstr = dstr_create("Hello, World!");

	// 检查是否创建成功并打印
	if (dstr != NULL) {
		printf("Created string: %s\n", dstr_cstr_const(dstr));

		// 销毁字符串，释放内存
		dstr_destroy(dstr);
	}
	// dstr_destroy(NULL);

	return EXIT_SUCCESS;
}

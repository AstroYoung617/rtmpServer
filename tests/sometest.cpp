#include <iostream>
#include <string>

void testCopyArray() {
	uint8_t a[3] = {0x47, 0x48, 0x49};
	uint8_t b[10] = { 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, };
	memcpy(b, a, sizeof(a));
	for (int i = 0; i < 10; i++) {
		std::cout << b[i] << " ";
	}
	std::cout << std::endl;
}

int main() {
	testCopyArray();
	return 0;
}
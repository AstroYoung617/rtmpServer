#include <iostream>
#include <string>
using namespace std;

int main() {
	int port[5];
	int i = 0;
	int* p = (int*)malloc(sizeof(int) * 5);
	while (cin >> port[i]) {
		p[i] = port[i];
		cout << p[i] << " ";
		i++;
	}
	cout << sizeof(port) / 4 << endl;
	
	return 0;
}
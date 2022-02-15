#include <iostream>
#include <vector>
using namespace std;

int main() {
  vector<int> test;
  int k;
	while (cin >> k)
	{
		test.push_back(k);
		if (cin.get() == '\n') break;
	}

	for (auto x : test)
		cout << x << " ";
}
           
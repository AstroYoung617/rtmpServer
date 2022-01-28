#include <iostream>
#include <fstream>

using namespace std;

void main(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++)
	{
		cout << "argument[" << i << "] is: " << argv[i] << endl;
	}
	system("pause");
}
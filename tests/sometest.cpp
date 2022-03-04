#include <iostream>
#include <string>
#include <vector>
#include <typeinfo>

using namespace std;

int main() {
	
	const vector<string> scores = { "F","D","C","B","A","A+" };
	unsigned grade = 0;
	string lettergrade;
	while (cin >> grade) {
		/*if (grade < 60)
			lettergrade = scores[0];
		else {
			lettergrade = scores[(grade - 50) / 10];
			if ((grade % 10) > 7)
				lettergrade += '+';
			else if ((grade % 10) < 3)
				lettergrade += '-';
		}*/
		(grade < 60) ? lettergrade = scores[0]
			: (((grade % 10) > 7) ? (lettergrade = scores[(grade - 50) / 10], lettergrade += '+')
				: (((grade % 10) < 3) ? (lettergrade = scores[(grade - 50) / 10], lettergrade += '-') : lettergrade = scores[(grade - 50) / 10]));
		
		cout << lettergrade;
	}
	cout << endl;
	return 0;
}
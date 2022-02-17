#include <iostream>
#include <thread>
#include <vector>
#include <functional>
using namespace std;

struct video {
	video(){	}

	void count(int n) {
		for(int i = 0; i < 5; i++){
			cout << n << endl;
		}
	}
};

int main() {
	video *test = new video();
	int n = 5;
	thread testvideo(&video::count, test, n);
	testvideo.join();
}
           
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <cstdlib>
using namespace std;

int main() {
	std::vector<char> v(100 * 1024 * 1024);
	while (1) {
		int s = rand() % 5;
		sleep(s);
		int size = rand() % 3;
		size++;
		v = std::vector<char>(size * 100 * 1024 * 1024);
	}
}

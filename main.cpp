#include <iostream>
#include "multi_vector.h"

int main() {
	using namespace mvec;

	multi_vector<int> a(1);
	a.reserve_push(0);
	a.push_back(0, 10);

	a.reserve_totalling(0);

	a.totalling(0);

	return 0;
}

#include <iostream>
#include "multi_vector.h"


void hoge(mvec::multi_vector<int>& vec, size_t ind) {
	
	for (int i = 0; i < 1000; i++)
	{
		vec.push_back(ind, i);
	}
	vec.reserve_totalling(ind);
	vec.totalling(ind);

}

int main() {
	using namespace mvec;

	multi_vector<int> a(5);
	a.reserve_push(5000);

	std::vector<std::thread> threads(5);
	
	for (size_t i = 0; i < 5; i++)
	{
		threads[i] = std::thread(&hoge, std::reference_wrapper(a), i);
	}

	for (auto&i : threads)
	{
		i.join();
	}

	a.totalling_wait();

	return 0;
}

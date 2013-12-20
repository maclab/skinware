#include <iostream>
#include <cstdlib>
#include <ctime>
#include "scaler.h"
#include "filter.h"
#include "amplifier.h"

using namespace std;

uint8_t amp(uint8_t x)
{
	if (x > 127)
		return 255-x;
	return x;
}

#define N 5
scaler s(500);
filter f(5);
amplifier al(2, -50);
amplifier aq(-255, -255, 255);
amplifier ac(amp);

void print(const skin_sensor_response *responses)
{
	cout << "Responses:";
	for (int i = 0; i < N; ++i)
		cout << " " << (int)responses[i];
	cout << endl;
	vector<uint8_t> scaled = s.scale(responses, N);
	cout << "Scaled to:";
	for (int i = 0; i < N; ++i)
		cout << " " << (int)scaled[i];
	cout << endl;
	f.new_responses(scaled);
	vector<uint8_t> filtered = f.get_responses();
	cout << "Filtered to:";
	for (int i = 0; i < N; ++i)
		cout << " " << (int)filtered[i];
	cout << endl;
	vector<uint8_t> amplified_l = al.amplify(filtered);
	vector<uint8_t> amplified_q = aq.amplify(filtered);
	vector<uint8_t> amplified_c = ac.amplify(filtered);
	cout << "Amplified to (linear):";
	for (int i = 0; i < N; ++i)
		cout << " " << (int)amplified_l[i];
	cout << endl;
	cout << "Amplified to (quadratic):";
	for (int i = 0; i < N; ++i)
		cout << " " << (int)amplified_q[i];
	cout << endl;
	cout << "Amplified to (custom):";
	for (int i = 0; i < N; ++i)
		cout << " " << (int)amplified_c[i];
	cout << endl;
}

int main()
{
	char t;
	skin_sensor_response responses[N];
	srand(time(NULL));
	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < N; ++j)
			responses[j] = (i + 1) * ((j + 1) * 255 / N);
		print(responses);
	}
	cin >> t;
	for (int i = 255; i >= 0; --i)
	{
		for (int j = 0; j < N; ++j)
			responses[j] = (i + 1) * ((j + 1) * 255 / N);
		print(responses);
	}
	cin >> t;
	for (int i = 0; i < 300; ++i)
	{
		for (int j = 0; j < N; ++j)
			responses[j] = rand() % 1000 + 8000;
		print(responses);
	}
	cin >> t;
	for (int i = 0; i < 300; ++i)
	{
		s.dampen(30);
		for (int j = 0; j < N; ++j)
			responses[j] = rand() % 1000 + 8000;
		print(responses);
	}
	cin >> t;
	s.reset();
	for (int i = 0; i < 300; ++i)
	{
		for (int j = 0; j < N; ++j)
			responses[j] = rand() % 1000 + 8000;
		print(responses);
	}
	return 0;
}

// MilionerzyKonwerter.cpp 
// Author: Mateusz ¯arkowski
// Konwerter pytañ .dat do formatu zgodnego z Quizem EMYSa.

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;

static std::vector<char> ReadAllBytes(char const* filename) {
	ifstream ifs(filename, ios::binary | ios::ate);
	ifstream::pos_type pos = ifs.tellg();

	std::vector<char>  result(pos);

	ifs.seekg(0, ios::beg);
	ifs.read(&result[0], pos);

	return result;
}

string readStringAtPos(std::vector<char>& data, unsigned int& index) {

	//sometimes after question there are not-nul symbols
	//skip them if thats the case
	if (data[index + 1] == '\0') {
		index++;
		while (data[index] == '\0') index++;

	}

	//read how many 
	int readCount = (int)data[index];
	string out = string(data.begin() + index + 1,
		data.begin() + index + 1 + readCount);
	index += 1 + readCount;
	//skip until end chars (nul)
	while (data[index] != '\0') index++;
	//skip all nuls
	while (data[index] == '\0') index++;

	return out;
}

vector<string> readEntry(std::vector<char>& data, unsigned int& index) {
	index += 8;
	vector<string> entry;
	for (int i = 0; i < 5; i++)
		entry.push_back(readStringAtPos(data, index));


	int ans = data[index];
	//below works because ans is 1-4
	iter_swap(entry.begin() + 1, entry.begin() + ans);

	//skip last 16
	index += 16;
	return entry;

}


int main() {

	std::vector<char> data = ReadAllBytes("in.dat");
	unsigned int index = 0;

	ofstream out("out.txt");
	int cnt = 0;
	while (index < data.size()) {
		out << "#" << setfill('0') << setw(4) << cnt++ << endl;
		vector<string> el = readEntry(data, index);
		for (string& s : el) {
			out << s << endl;
		}
	}

	//cin.get();
    return 0;
}


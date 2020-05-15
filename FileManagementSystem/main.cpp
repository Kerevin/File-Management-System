#include "BootSector.h"
#include "RDET.h"
#include <Windows.h>
#include <filesystem>
#include <unordered_map>
using namespace std;
namespace fs = std::experimental::filesystem;
void importFile(fstream& f, char* filename) {
	fstream output;
	output.open(filename, ios::binary | ios::out);
	char buffer[1024];
	f.seekg(0, ios::end);
	long long size = f.tellg();
	f.seekg(0, ios::beg);
	/*
	while(of.read(buffer, sizeof(buffer)))
		{

			output.write(buffer, of.gcount());
		}
	output.write(buffer, of.gcount());
	*/
	output.close();
}
void exportFile(fstream& f, char* filename) {
	fstream output;
	output.open(filename, ios::binary | ios::out);
	char buffer[1024];
	while (f.read(buffer, sizeof(buffer))) {
		output.write(buffer, f.gcount());
	}
	output.write(buffer, f.gcount());
	output.close();
}

void createBootSector(fstream& f, long volSize)
{
	long totalSectors = volSize * (1024 * 1024) / 512;

	for (int i = 0; i < 20; ++i) {
		int zero = 12;
		f.write((char*)&zero, sizeof(char));
	}
}

void initializeFile(fstream& f, int volumeSize)
{
	int zero = 0;
	for (long i = 0; i <= volumeSize * 1024 * 1024 / 4; ++i)
	{
		f.write((char*)&zero, 4);
	}
}

//int main()
//{
//	WIN32_FIND_DATA fd;
//	string search_path = "D:/Coding/HeDieuHanh/FileManagementSystem/FileManagementSystem";
//	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
//
//}

int main() {
	long volumeSize = 4;
	BootSector bs(volumeSize);

	fstream f("test.re", ios::binary | ios::out | ios::in);
	initializeFile(f, volumeSize);
	bs.createBootSector(f);
	//bs.readBootSector(f);

	RDET rd(bs);
	vector<FAT> nFat;
	for (int i = 0; i < bs.getNumFats(); i++)
	{
		FAT f(bs, i);
		nFat.push_back(f);
	}
	rd.addItem(f, "D:/Coding/HeDieuHanh/FileManagementSystem/FileManagementSystem/Debug", true, nFat);

	f.close();

}
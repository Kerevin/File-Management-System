#include "RDET.h"
#include <Windows.h>
#include <unordered_map>
using namespace std;


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

	fstream f("test.re", ios::binary | ios::in | ios::out);
	if (f.fail())
	{
		cout << "FAILED TO OPEN FILE " << endl;
		f.close();
		return 0;
	}
	initializeFile(f, volumeSize);
	bs.createBootSector(f);
	//bs.printBootSector();

	RDET rd(bs);
	FAT fat(bs);

	rd.addItem(f, "D:/Stuff/testvolume", true, fat, true);

	f.close();

}
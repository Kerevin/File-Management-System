#include "BootSector.h"
#include "RDET.h"
#include <Windows.h>
#include <filesystem>
#include <sys/stat.h>

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
void readCluster(int numCluster, int bootSectorSize, int fatSize, int numFat, int numSectorsOfCluster) {
	int clusterPosition = bootSectorSize + fatSize * numFat + (numCluster - 2) * numSectorsOfCluster;
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

int main()
{
	RDET rd;
	rd.get_all_files_names_within_folder("D:/Coding/HeDieuHanh/FileManagementSystem/FileManagementSystem");

}

//int main() {
//    long volumeSize = 4;
//    BootSector bs(volumeSize);
//    RDET rd;
//    fstream f("test.re", ios::binary | ios::in | ios::out);
//   initializeFile(f, volumeSize);
//   bs.createBootSector(f);
//    
//    bs.readBootSector(f);
//   bs.printBootSector();
//    f.close();
//
//}
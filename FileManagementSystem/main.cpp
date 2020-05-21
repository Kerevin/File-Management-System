#include "RDET.h"
#include "FileManagement.h"
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

void createNewVolume(fstream& f, long volumeSize, BootSector& bs)
{

	initializeFile(f, volumeSize);
	bs.createBootSector(f);
}
int main()
{
	int ch;
	cout << "1. Open volume" << endl;
	cout << "2. Create new volume" << endl;
	do {
		cout << ">> ";
		cin >> ch;
	} while (ch > 2 || ch < 0);
	
	cin.ignore(1);
	string path;
	bool validPath = false;
	do{
		
		if (ch == 1)
		{
			cout << "Link of volume: ";
			getline(cin, path);
			fstream test(path, ios::in | ios::binary);
			if (!test.fail())
				{validPath = true;}
			else {
				cout << "Path error" << endl;
			}
		}
		else{
			cout << "Enter path to save: "; 
			getline(cin, path);
			DWORD attribs = ::GetFileAttributesA(path.c_str());
			if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
				validPath = true;
				string name;
				cout <<"Enter name of volume: ";
				getline(cin, name);
				path+= "/" + name;
			}
		}
		
	} while(!validPath);
	

	system("cls");

	if (ch == 2)
	{
		long volumeSize;
		cout << "Enter size of volume(MB): ";
		cin >> volumeSize;
		cin.ignore(1);
		FileManagement fm(volumeSize, path);
		fm.showMenu();
	}
	else {
		FileManagement fm(path);	
		fm.showMenu();
	}

}
//int main() {
//	long volumeSize = 4;
//	BootSector bs(4);
//
//	fstream f("test.re", ios::binary | ios::in | ios::out);
//	if (f.fail())
//	{
//		cout << "FAILED TO OPEN FILE " << endl;
//		f.close();
//		return 0;
//	}
//	//bs.readBootSector(f);
//	//createNewVolume(f, volumeSize, bs);
//	//bs.printBootSector();
//	RDET rd(bs);
//	FAT fat(bs);
//
//	//rd.addItem(f, "D:/Stuff/testvolume", true, fat, false);
//	rd.showFolder(f, fat);
//	cout << endl;
//
//
//
//	f.close();


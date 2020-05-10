#pragma once
#include <fstream>
#include <string>
#include <string.h>
#include <iostream>
#include <vector>
using namespace std;

class BootSector
{
private:
	long int volSize; // Volume size: kích thước của volume, đơn vị MB
	int volSector; // Số sector của volume
	int sectorSize; // Size của mỗi sector, mặc định sẽ là 512 byte
	int clusterSize;  //  mặc định sẽ là 4096 (dạng 2 ^ (10 + n)) 
	int clusterSectors; // Sc: Số sector của mỗi cluster, clusterSize / sectorSize
	int numCluster; // Số cluster của volume
	int bootSize;	// Sb, mặc định = 1 sector
	int fatSize; // SF: số sector của bảng FAT, dựa trên số cluster hiện có
	int numFat; // Số bảng FAT
	int entrySizeRDET; // SR: số entry của RDET, mặc định là 512 (Mỗi entry có 32 bytes)

	// Vị trí của thông số quan trọng để ghi vào boot sector
	vector <int> writePosition = {0xB, 0x3, 0xD, 0xE, 0x10, 0x11, 0x16, 0x20, 0x36, 0x1FE};
	vector <int> numBytesWritten = {2,   8,   1,   2,    1,    2,    2,    4,    8,     2};	// số byte yêu cầu để ghi
	vector <long long> writeContent;

public:
	
	BootSector()
	{
		volSize = 1;	
		sectorSize = 512;
		volSector = volSize * 1024 * 1024 / sectorSize;
		clusterSize = 4096;
		clusterSectors = clusterSize / sectorSize;
		bootSize = 1;
		entrySizeRDET = 512;
		numFat = 2;
		fatSize = int((this->volSector - this->bootSize) / (256 * this->clusterSectors + this->numFat)) + 1;
	}
	BootSector(long volSize)
	{
		this->volSize = volSize;
		sectorSize = 512;
		clusterSize = 4096;
		volSector = volSize * 1024 * 1024 / sectorSize;
		clusterSectors = clusterSize / sectorSize;
		bootSize = 1;
		entrySizeRDET = 512;
		numFat = 2;
		fatSize = int((this->volSector - this->bootSize) / (256 * this->clusterSectors + this->numFat)) + 1;
	}


	void createBootSector(fstream &f)
	{
		writeContent = { sectorSize, 'TSER',clusterSectors, bootSize, numFat, entrySizeRDET, fatSize, volSector, 2314885625596363078 /*Fat 16 */, 43605 /*kết thúc boot sector*/};
		for (int i = 0; i<writePosition.size(); i++)
		{
			f.seekg(writePosition[i], ios::beg);
			f.write((char*) &writeContent[i], this->numBytesWritten[i]);
		}
	}

	void readBootSector(fstream& f)
	{
		int writePosition[] = { 0xB, 0xD, 0xE, 0x10, 0x11, 0x16, 0x20};
		int numBytesWritten []= { 2,   1,   2,    1,    2,    2,    4};	// số byte yêu cầu để ghi
		
		vector<int*> content = { &sectorSize, &clusterSectors, &bootSize, &numFat, &entrySizeRDET, &fatSize, &volSector};
		for (int i = 0; i < content.size(); i++)
		{
			f.seekg(writePosition[i], ios::beg);
			f.read((char*)&content[i], this->numBytesWritten[i]);
		}
	}

	void printBootSector()
	{
		cout << "Volume size: " << volSize << endl;
		cout << "Sector size: " << sectorSize << endl;
		cout << "Cluster size: " << clusterSize << endl;
		cout << "Number sectors of Volume: " << volSector << endl;
		cout << "Number sectors of Cluster: " << clusterSectors << endl;
		cout << "Number sectors of boot sector: " << bootSize << endl;
		cout << "Number entries of RDET: " << entrySizeRDET << endl;
		cout << "Number of FAT Table: " << numFat << endl;
		cout << "Fat size: " << fatSize << endl;
	}
};
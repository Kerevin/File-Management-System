#pragma once
#include <fstream>
#include <string>
#include <string.h>
#include <iostream>
#include <vector>
#include <cmath>
using namespace std;

class BootSector
{
private:
	int volSize; // Volume size: kích thước của volume, đơn vị MB
	int volSector; // Số sector của volume
	int sectorSize; // Size của mỗi sector, mặc định sẽ là 512 byte
	int clusterSize;  //  mặc định sẽ là 4096 (dạng 2 ^ (10 + n)) 
	int clusterSectors; // Sc: Số sector của mỗi cluster, clusterSize / sectorSize
	int bootSize;	// Sb, mặc định = 1 sector
	int fatSize; // SF: số sector của bảng FAT, dựa trên số cluster hiện có
	int entrySizeRDET; // SR: số entry của RDET, mặc định là 512 entry (Mỗi entry có 32 bytes)
	int currentVolSector; // Số sector còn lại của volume;
	// Vị trí của thông số quan trọng để ghi vào boot sector
	vector <int> offset = { 0xB, 0x3, 0xD, 0xE, 0x11, 0x16, 0x20, 0x36, 0x40, 0x1FE };
	vector <int> numBytesWritten = { 2, 8, 1, 2, 1, 2, 4, 8, 4, 2 };	// số byte yêu cầu để ghi
	vector <long long> writeContent;

public:
	BootSector()
	{

	}
	BootSector(int volSize)
	{
		this->volSize = volSize;
		sectorSize = 512;
		clusterSize = 4096;
		volSector = volSize * 1024 * 1024 / sectorSize;
		clusterSectors = clusterSize / sectorSize;
		bootSize = 1;
		entrySizeRDET = 512;
		fatSize = round(float(this->volSector - this->bootSize - ((entrySizeRDET * 32) / sectorSize)) / (256 * this->clusterSectors + 1));
		currentVolSector = volSector - fatSize - bootSize - ((entrySizeRDET * 32) / sectorSize);

	}


	void createBootSector(fstream& f)
	{
		writeContent = { sectorSize, 'TSER',clusterSectors, bootSize, entrySizeRDET, fatSize, volSector, 2314885625596363078 /*Fat 16 */, currentVolSector, 43605 /*kết thúc boot sector*/ };
		for (int i = 0; i < offset.size(); i++)
		{
			f.seekg(offset[i], ios::beg);	// Seek đến offset quan trọng được lưu ở writePosition
			f.write((char*)&writeContent[i], this->numBytesWritten[i]);	// Lưu giá trị quan trọng
		}
	}

	void readBootSector(fstream& f)
	{
		int writePosition[] = { 0xB, 0xD, 0x10, 0x11, 0x16, 0x20, 0x40 };
		int numBytesWritten[] = { 2,   1,    1,    2,    2,    4,    4 };	// số byte yêu cầu để ghi

		vector<int*> content = { &sectorSize, &clusterSectors, &bootSize, &entrySizeRDET, &fatSize, &volSector, &currentVolSector };
		for (int i = 0; i < content.size(); i++)
		{
			f.seekg(writePosition[i], ios::beg);
			f.read((char*)&content[i], this->numBytesWritten[i]);
		}
		volSize = volSector * sectorSize;
	}
	int getVolumeSize()
	{
		return this->volSector;
	}
	int getClusterSector()
	{
		return this->clusterSectors;
	}
	int getRDETOffset()
	{
		return this->bootSize + fatSize;
	}

	int getRDETSize()
	{
		return ((entrySizeRDET * 32) / sectorSize);
	}

	int getFATOffset()
	{
		return bootSize;
	}


	int getSectorSize()
	{
		return sectorSize;
	}

	int getFATSize()
	{
		return fatSize;
	}

	int getCurrentSize()
	{
		return currentVolSector;
	}
	void printBootSector()
	{
		cout << "Volume size: " << volSize << " MB" << endl;
		cout << "Sector size: " << sectorSize << endl;
		cout << "Cluster size: " << clusterSize << endl;
		cout << "Number sectors of Volume: " << volSector << endl;
		cout << "Number sectors of Cluster: " << clusterSectors << endl;
		cout << "Number sectors of boot sector: " << bootSize << endl;
		cout << "Number entries of RDET: " << entrySizeRDET << endl;

		cout << "Fat size: " << fatSize << endl;
	}
};
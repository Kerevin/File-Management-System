#pragma once
#include "BootSector.h"


class FAT
{
private:
	int size; // SF, Size của bảng FAT, đơn vị sector
	int offset; // Vị trí của FAT trong volume
	int sectorSize; // Size của mỗi sector
	int clusterSize; // Số sector của cluster
	int totalEmptyCluster; // Tổng số cluster trống


public:
	FAT(BootSector& bs, int num)
	{
		this->size = bs.getFATSize();
		this->offset = bs.getFATOffset() + num * size;  // Num là số thứ tự của FAT ( nếu có 1 bảng thì num = 0, đếm dần lên)
		this->sectorSize = bs.getSectorSize(); // Size của mỗi sector
		this->clusterSize = bs.getClusterSector(); // Số sector của cluster
		this->totalEmptyCluster = bs.getVolumeSize() - size - offset * sectorSize; // Cluster trống = sector của volume - size RDET - sector trước RDET
	}

	int getCluster(int k) {

		return this->offset + this->size + (k - 2) * clusterSize;
	}


	vector<int> findEmptyOffsets(fstream& f, WIN32_FIND_DATA file)
	{
		int currentOffset = this->offset * sectorSize + 2;
		bool isFull = false;
		vector<int> emptyOffset;
		cout << "FILE SIZE: " << (file.nFileSizeHigh * MAXDWORD) + file.nFileSizeLow << endl;
		while (!isFull && (currentOffset - this->offset * sectorSize) < (size * sectorSize))
		{
			f.seekg(currentOffset, ios::beg);
			short temp;

			f.read((char*)&temp, 2);
			if (temp == 0)
			{
				emptyOffset.push_back(currentOffset - this->offset * sectorSize); // Lưu vị trí trong bảng FAT dưới số thứ tự cluster

				if ((file.nFileSizeHigh * MAXDWORD) + file.nFileSizeLow <= emptyOffset.size() * clusterSize * sectorSize)
				{
					isFull = true;
				}
			}

			currentOffset += 2;
		}
		cout << "CLUSTER BAT DAU: " << emptyOffset[0] << endl;
		return emptyOffset;
	}
	void writeFAT(fstream& f, vector<int> offsets)
	{
		int currentOffset = this->offset * sectorSize + offsets[0]; // Vị trí hiện tại để theo dõi giá trị trong bảng FAT


		if (offsets.size() > 1)
			for (int i = 1; i < offsets.size() - 1; ++i)
			{
				f.seekg(currentOffset, ios::beg);
				f.write((char*)&offsets[i], 2);
				currentOffset = this->offset * sectorSize + offsets[i];

			}
		f.seekg(currentOffset, ios::beg);
		char eof[5] = "FFFF";
		f.write(eof, 4);

	}

};
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
	FAT(BootSector& bs)
	{
		this->size = bs.getFATSize();
		this->offset = bs.getFATOffset();  // Num là số thứ tự của FAT ( nếu có 1 bảng thì num = 0, đếm dần lên)
		this->sectorSize = bs.getSectorSize(); // Size của mỗi sector
		this->clusterSize = bs.getClusterSector(); // Số sector của cluster
		this->totalEmptyCluster = bs.getVolumeSize() - size - offset * sectorSize; // Cluster trống = sector của volume - size RDET - sector trước RDET
	}

	int getCluster(int k) {

		return this->offset + this->size + (k - 2) * clusterSize;
	}


	vector<int> findEmptyOffsets(fstream& f, WIN32_FIND_DATA file, long sizeFize)
	{
		int currentOffset = this->offset * sectorSize + 2;
		bool isFull = false;
		vector<int> emptyOffset;
		cout << "FILE SIZE: " << sizeFize << endl;
		while (!isFull && (currentOffset - this->offset * sectorSize) < (size * sectorSize))
		{
			f.seekp(currentOffset, ios::beg);
			short temp;

			f.read((char*)&temp, 2);
			if (temp == 0)
			{
				emptyOffset.push_back((currentOffset - this->offset * sectorSize) / 2 + 1); // Lưu vị trí trong bảng FAT dưới số thứ tự cluster

				if (sizeFize <= emptyOffset.size() * clusterSize * sectorSize)
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
		int currentOffset = this->offset * sectorSize + (offsets[0] - 1) * 2; // Vị trí hiện tại để theo dõi giá trị trong bảng FAT
		cout << currentOffset << endl;

		if (offsets.size() > 1)
			for (int i = 1; i < offsets.size() - 1; ++i)
			{
				f.seekg(currentOffset, ios::beg);
				f.write((char*)&offsets[i], 2);
				currentOffset = this->offset * sectorSize + (offsets[i] - 1) * 2;

			}
		f.seekg(currentOffset, ios::beg);
		char eof[3] = "FF";
		f.write(eof, 2);

	}

};
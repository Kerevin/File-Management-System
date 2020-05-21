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
		int currentOffset = this->offset * sectorSize + 2;	// Offset bắt đầu của bảng FAT, tính bằng bytes
		bool isFull = false;
		vector<int> emptyOffset;	// Lưu lại tất cả offset có thể của item truyền vào

		while (!isFull && (currentOffset - this->offset * sectorSize) < (size * sectorSize))
		{
			f.seekp(currentOffset, ios::beg);
			short temp;

			f.read((char*)&temp, 2);
			if (temp == 0)
			{
				emptyOffset.push_back((currentOffset - this->offset * sectorSize) / 2 + 1); // Lưu vị trí trong bảng FAT dưới số thứ tự cluster

				if (sizeFize <= emptyOffset.size() * clusterSize * sectorSize)	// Kiểm tra xem chứa đủ item chưa, nếu chưa thì kiếm tiếp 1 cluster, đủ thì thôi
				{
					isFull = true;
				}
			}

			currentOffset += 2;
		}

		return emptyOffset;
	}
	void writeFAT(fstream& f, vector<int> clusters)
	{
		int currentOffset = this->offset * sectorSize + (clusters[0] - 1) * 2; // Vị trí hiện tại để theo dõi giá trị trong bảng FAT


		if (clusters.size() > 1)
			for (int i = 1; i < clusters.size(); ++i)
			{
				f.seekg(currentOffset, ios::beg);
				clusters[i] = (clusters[i] - 1) * 2;	// offset trong bảng FAT của cluster k = (cluster k - 1 ) * 2
				f.write((char*)&clusters[i], 2);
				currentOffset = this->offset * sectorSize + clusters[i];

			}
		f.seekg(currentOffset, ios::beg);
		short eof = 255;
		f.write((char*)&eof, 2);

	}
	vector<int> getItemClusters(fstream& f, int clusterK)
	{

		// Hàm để lấy tất cả vị trí cluster của một item có cluster bắt đầu là cluster K
		int currentOffset = this->offset * sectorSize + (clusterK - 1) * 2; // Suy từ cluster K sang offset trong FAT
		f.seekg(currentOffset, ios::beg);
		bool isFull = false;
		vector<int> cluster;
		cluster.push_back(clusterK);


		while (!isFull)
		{
			short value;

			f.read((char*)&value, 2);
			if (value == 255)
			{
				isFull = true;
				if (cluster.size() > 1)
				{
					cluster.push_back((currentOffset - this->offset * sectorSize) / 2 + 1);
				}
			}
			else {
				cluster.push_back(value / 2 + 1);
				currentOffset = this->offset * sectorSize + value;
				f.seekp(currentOffset, ios::beg);
			}

		}

		return cluster;
	}
	void deleteItem(fstream& f, int clusterK)
	{
		vector<int> allClusters = getItemClusters(f, clusterK);
		int currentOffset;
		int zero = 0;
		for (auto cluster : allClusters)
		{
			currentOffset = (cluster - 1) * 2 + (this->offset * sectorSize);	// Chuyển từ cluster thứ K sang offset trong FAT

			f.seekg(currentOffset, ios::beg);
			f.write((char*)&zero, 2);
		}

	}
};

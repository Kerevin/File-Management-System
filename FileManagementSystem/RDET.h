#pragma once
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <unordered_map> 
#include "BootSector.h"
using namespace std;
struct Entry
{
	string fileName; // Tên tập tin, offset 0x0
	string fileExtension; // Tên mở rộng, offset 0x8
	string fileAttribute; // Thuộc tính tập tin, , offset 0xB (11)
	char empty[14] = { "0" };	// Thông tin không quan trọng, một dãy 0 từ offset 0xC -> 0x10
	short firstCluster; // Cluster bắt đầu, offset 0x1A (26)
	int fileSize; // Kích thước tập tin, offset 0x1C (28)
};
struct SubEntry
{
	// Entry phụ lưu tên dài //

	unsigned char orderNumber; // Thứ tự của entry (bắt đầu từ 1) - Kết thúc thường là 0xA (nếu chỉ có 1 entry phụ) hoặc 0xB (Nếu có hơn 2 entry phụ)
	char fileName[8]; // Tên tập tin, offset 0x0
	char fileExtension[3]; // Tên mở rộng, offset 0x8
	unsigned char fileAttribute; // Thuộc tính tập tin, , offset 0xB (11)
	char empty[14] = { "0" };	// Thông tin không quan trọng, một dãy 0 từ offset 0xC -> 0x10
	short firstCluster; // Cluster bắt đầu, offset 0x1A (26)
	int fileSize; // Kích thước tập tin, offset 0x1C (28)
};
class RDET
{
private:
	int size;		 // Size của RDET, đơn vị sector
	int offset;		// Offset của RDET 
	int sectorSize; // Size của mỗi sector
	int clusterSize; // Số sector của cluster
	int totalEmptyCluster; // Tổng số cluster trống
public:

	RDET(int size, int offset, int sectorSize)
	{
		this->size = size;
		this->offset = offset;
		this->sectorSize = sectorSize;
	}
	RDET(BootSector& bs)
	{
		this->size = bs.getRDETSize();
		this->offset = bs.getRDETOffset();
		this->sectorSize = bs.getSectorSize();
		this->clusterSize = bs.getClusterSector();
		this->totalEmptyCluster = bs.getVolumeSize() - size - offset * sectorSize; // Cluster trống = sector của volume - size RDET - sector trước RDET 


	}
	string getFileExtension(string file)
	{
		transform(file.begin(), file.end(), file.begin(), ::toupper); // In hoa chữ cái
		return file.substr(file.rfind('.') + 1);
	}
	string readFileInfo(WIN32_FIND_DATA file, string extention)
	{
		if (extention.size() > 3)
		{
			extention = extention.substr(0, 3);
		}

		while (extention.size() < 3)
		{
			extention += ' ';
		}
		string f = ""; // Cấu trúc đủ 32 bytes;
		string attribute = (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? "1" : "0";
		f += string(file.cFileName) + extention + attribute;

		for (int i = 0; i < 14; i++)
		{
			f += ".";
		}

		return f;
	}
	string getShortName(WIN32_FIND_DATA fileName)
	{

		// Xử lý để chuyển thành tên ngắn
		int fileExtentionPosition = string(fileName.cFileName).rfind('.');	// Tìm xem có dính file đuôi hay không
		string newName = string(fileName.cFileName).substr(0, 8);	// Lấy 8 ký tự 
		if (fileExtentionPosition != -1 && fileExtentionPosition < newName.size() && (fileName.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			newName = newName.substr(0, fileExtentionPosition);

		}
		while (newName.size() < 8)
		{
			newName += ' ';
		}
		transform(newName.begin(), newName.end(), newName.begin(), ::toupper); // In hoa chữ cái
		return newName;
	}
	vector<WIN32_FIND_DATA> processShortName(vector<WIN32_FIND_DATA> fileName)
	{
		for (int i = 0; i < fileName.size(); ++i)
		{
			strcpy_s(fileName[i].cFileName, getShortName(fileName[i]).c_str());
		}
		return fileName;
	}


	vector<string> get_all_files_names_within_folder(fstream& f, string folder, int pivotOffset)
	{

		vector<string> folderName;
		vector<WIN32_FIND_DATA> fileName;
		vector<WIN32_FIND_DATA> folderNameTemp; // Lưu trữ tên folder để làm tên ngắn
		string search_path = folder + "/*.*";
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);


		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				{
					//cout << "Name: " << fd.cFileName << ", size: " << (fd.nFileSizeHigh * MAXDWORD) + fd.nFileSizeLow << endl;;
					if (strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
						if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							folderName.push_back(fd.cFileName);
							folderNameTemp.push_back(fd);
						}
						else
						{
							fileName.push_back(fd);
						}

				}
			} while (::FindNextFile(hFind, &fd));
			fileName.insert(fileName.end(), folderNameTemp.begin(), folderNameTemp.end());	// Gộp 2 mảng tên dài lại để tính toán tên ngắn cho tập tin và thư mục.
			vector<WIN32_FIND_DATA> shortName = processShortName(fileName);
			for (auto x : shortName)
			{
				cout << x.cFileName << endl;
			}

			for (int i = 0; i < shortName.size() - folderName.size(); i++)
			{
				// Ghi nội dung của file
				addFile(f, fileName[i], readFileInfo(shortName[i], getFileExtension((string)fileName[i].cFileName)), pivotOffset);
				//cout << readFileInfo(shortName[i], getFileExtension((string)fileName[i].cFileName)) << endl;

			}
			for (int i = 0; i < folderName.size(); i++)
			{
				// Ghi nội dung của sub folders
				addFolder(f, folder + "/" + folderName[i], readFileInfo(shortName[i + fileName.size() - 1], ""));
				//get_all_files_names_within_folder(f, folder + "/" + string(folderName[i]), 0);
			}
			::FindClose(hFind);
		}
		return folderName;
	}


	void addFile(fstream& f, WIN32_FIND_DATA file, string shortNameInfo, int pivotOffset)
	{
		/*Nếu là file của một sub folder thì có pivotOffset
		pivotOffset = cluster bắt đầu của sub folder đó
		*/
		int limitSize = pivotOffset == 0 ? (this->size + this->offset) * sectorSize : totalEmptyCluster * clusterSize * sectorSize;
		int currentOffset;
		if (pivotOffset == 0)
			currentOffset = this->offset * this->sectorSize; // Offset hiện tại
		else
			currentOffset = getCluster(pivotOffset) * this->sectorSize;
		bool emptyEntry = false;

		// Kiếm chỗ trống để ghi Entry
		while (currentOffset < limitSize && !emptyEntry)
		{
			f.seekg(currentOffset, ios::beg);
			char currentValue[5];
			f.read((char*)&currentValue, 4);
			currentValue[4] = '\0';
			if (string(currentValue) == "")
			{
				emptyEntry = true;
			}
			else
				currentOffset += 32;
		}
		cout << "Current offset: " << currentOffset << ", empty: " << emptyEntry << endl;
		vector<int> allAvailableCluster; // Cluster trống để lưu nội dung file
		if (emptyEntry)
		{
			f.seekg(currentOffset, ios::beg);
			// Đi tìm cluster trống để ghi nội dung file
			for (int k = 2; k <= totalEmptyCluster; k++)
			{
				int firstOffset; // Offset đầu của cluster đó
				f.seekg(this->getCluster(k) * sectorSize);	// Nhảy tới offset của cluster k
				f.read((char*)&firstOffset, 4);
				if (firstOffset == 0 || firstOffset == 0xE5)
				{
					allAvailableCluster.push_back(k);

					// Kiểm tra xem dung lượng file có vượt qua n cluster hay chưa?
					// allAvailableCluster.size() * clusterSize * sectorSize là tính số byte hiện tại đã lưu được của file
					if ((file.nFileSizeHigh * MAXDWORD) + file.nFileSizeLow <= allAvailableCluster.size() * clusterSize * sectorSize)
					{
						break;
					}
				}
			}


		}

		if (allAvailableCluster.size() > 0)
		{
			cout << "GHI VAO OFFSET: " << currentOffset << ", file: " << shortNameInfo << endl;
			// Ghi entry;
			f.seekg(currentOffset, ios::beg);
			//cout << shortNameInfo << endl;
			char temp[27];	// mảng char tạm ghi vào entry
			strcpy_s(temp, shortNameInfo.c_str());
			//cout << temp << endl;
			f.write((char*)&temp, 26);
			//cout << f.tellg() << endl;
			// Ghi cluster bắt đầu
			f.write((char*)&allAvailableCluster[0], 2);
			//cout << allAvailableCluster[0] << endl;
			// Ghi file size
			int fileSize = getFileSize(file);
			f.write((char*)&fileSize, 4);


		}
	}
	void addFolder(fstream& f, string folder, string shortName)
	{
		WIN32_FIND_DATA fd;

		HANDLE hFind = ::FindFirstFile(folder.c_str(), &fd);
		strcpy_s(fd.cFileName, string(fd.cFileName).substr(0, 5).c_str());
		addFile(f, fd, shortName, 0);
		cout << "Hello, folder: " << f.tellg() << endl;
		f.seekg(-6, ios::cur);
		cout << ", offset" << f.tellg() << endl;
		short firstClusterOfFoler;
		f.read((char*)&firstClusterOfFoler, 2);
		cout << "Cluter bd: " << int(firstClusterOfFoler) << ", offset" << f.tellg() << endl;
		get_all_files_names_within_folder(f, folder, firstClusterOfFoler);
	}

	void addItem(fstream& f, string item, bool isFolder)
	{
		/*Hàm thêm một item vào vol
		isFolder: True nếu item dc thêm vào là 1 folder, ngược lại là thêm vào 1 file
		*/
		if (isFolder)
		{
			get_all_files_names_within_folder(f, item, 0);
		}
		else {

		}
	}


	int getCluster(int k) {

		return this->offset + this->size + (k - 2) * clusterSize;
	}

	int getFileSize(WIN32_FIND_DATA fd)
	{
		return (fd.nFileSizeHigh * MAXDWORD) + fd.nFileSizeLow;
	}

};


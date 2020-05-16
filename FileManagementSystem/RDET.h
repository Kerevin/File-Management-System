#pragma once
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <unordered_map> 
#include "FAT.h"
using namespace std;

class RDET
{
private:
	int size;		 // Size của RDET, đơn vị sector
	int offset;		// Vị trí của RDET trong Volume
	int sectorSize; // Size của mỗi sector
	int clusterSize; // Số sector của cluster
	int totalEmptyCluster; // Tổng số cluster trống
	int remaningSectors; // Số sector trống còn lại
public:
	RDET(BootSector& bs)
	{
		this->size = bs.getRDETSize();
		this->offset = bs.getRDETOffset();
		this->sectorSize = bs.getSectorSize();
		this->clusterSize = bs.getClusterSector();
		this->remaningSectors = bs.getCurrentSize();
		this->totalEmptyCluster = (bs.getVolumeSize() - (size + offset)) / 8; // Cluster trống = sector của volume - size RDET - sector trước RDET 

	}

	string getFileExtension(string file)
	{

		transform(file.begin(), file.end(), file.begin(), ::toupper); // In hoa chữ cái
		if (file.rfind('.') >= 0)
			return file.substr(file.rfind('.') + 1);
		else return "";

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

	WIN32_FIND_DATA processShortName(WIN32_FIND_DATA fileName)
	{

		strcpy_s(fileName.cFileName, getShortName(fileName).c_str());
		return fileName;
	}

	string readItemInfo(WIN32_FIND_DATA file, string password)
	{
		string extention = (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ? getFileExtension(string(file.cFileName)) : "  ";	// Kiểm tra nếu folder thì không có file đuôi

		if (extention.size() > 3)
		{
			extention = extention.substr(0, 3);
		}

		while (extention.size() < 3)
		{
			extention += ' ';
		}
		file = processShortName(file);

		string f = ""; // Cấu trúc đủ 32 bytes;
		string attribute = (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? "1" : "0";
		f += string(file.cFileName) + extention + attribute;
		if (password.size() > 0)
		{
			f += password;
		}
		for (int i = password.size(); i < 14; i++)
		{
			f += ".";
		}

		return f;
	}

	string getSubEntry(string fileName) {
		// Entry phụ để thể hiện tên dài
		string n = ". ";
		if (fileName.size() <= 29)
		{
			n += fileName;
			for (int i = n.size(); i < 32; i++) {
				n += " ";
			}
		}
		else
		{
			n += fileName.substr(0, 30);
			string m = fileName.substr(29, fileName.length());
			n = getSubEntry(m) + n;

		}
		return n;
	}

	string handleFileName(WIN32_FIND_DATA fileName)
	{
		// Xử lý tên, loại bỏ file đuôi.
		string newName = string(fileName.cFileName);
		int fileExtentionPosition = string(fileName.cFileName).rfind('.');	// Tìm xem file đuôi ở đâu
		if (fileExtentionPosition != -1 && (fileName.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			newName = newName.substr(0, fileExtentionPosition);
		}

		return newName;
	}

	int getSizeOfFolder(string path)
	{
		int size = 0;
		string search_path = path + "/*.*";
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);

		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				{
					//cout << "Name: " << fd.cFileName << ", size: " << (fd.nFileSizeHigh * MAXDWORD) + fd.nFileSizeLow << endl;;
					if (strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
					{
						if (strlen(fd.cFileName) > 8)
						{
							size += getSubEntry(handleFileName(fd)).size();
						}
						size += 32;
					}


				}
			} while (::FindNextFile(hFind, &fd));

		}
		return size;
	}
	void getAllItemsWithinFolder(fstream& f, string folder, int pivotOffset, FAT& fat, string password)
	{
		// Hàm này lấy hết các file hay sub folder trong một folder
		vector<string> folderName;
		vector<WIN32_FIND_DATA> fileName;

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

						}
						else
						{
							fileName.push_back(fd);
						}

				}
			} while (::FindNextFile(hFind, &fd));


			for (int i = 0; i < fileName.size(); i++)
			{
				// Ghi nội dung của file
				addFile(f, fileName[i], pivotOffset, fat, folder + "/" + string(fileName[i].cFileName), password);

			}
			for (int i = 0; i < folderName.size(); i++)
			{
				// Ghi nội dung của sub folders
				addFolder(f, folder + "/" + folderName[i], pivotOffset, fat, password);

			}
			::FindClose(hFind);
		}

	}

	long getTotalSize(string path)
	{
		// Lấy tổng kích thước của folder


		vector<string> folderName;
		vector<WIN32_FIND_DATA> fileName;
		string search_path = path + "/*.*";
		WIN32_FIND_DATA fd;
		long size = 0;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);


		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				{
					if (strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
						if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							folderName.push_back(fd.cFileName);
						}
						else
						{
							fileName.push_back(fd);
						}

				}
			} while (::FindNextFile(hFind, &fd));

			for (int i = 0; i < fileName.size(); i++)
			{

				size += getFileSize(fileName[i]);

			}
			for (int i = 0; i < folderName.size(); i++)
			{// Ghi nội dung của sub folders
				size += getTotalSize(path + "/" + folderName[i]);
			}
			::FindClose(hFind);
		}
		return size;
	}

	void addFile(fstream& f, WIN32_FIND_DATA file, int pivotCluster, FAT& fat, string path, string password)
	{
		/* Nếu là file của một sub folder thì có pivotCluster
		pivotCluster = cluster bắt đầu nội dung (gồm các file/folder) của sub folder đó   */

		string shortName = readItemInfo(file, password);
		int limitSize = pivotCluster == 0 ? (this->size + this->offset) * sectorSize : totalEmptyCluster * clusterSize * sectorSize;
		int currentOffset;	// Vị trí trong bảng RDET để lưu nội dung của entry
		if (pivotCluster == 0)
			currentOffset = this->offset * this->sectorSize; // Offset hiện tại
		else
			currentOffset = getCluster(pivotCluster) * this->sectorSize;
		bool emptyEntry = false;


		cout << "Dang ghi file: " << shortName << "cluster thứ: " << pivotCluster << endl;

		// Kiếm chỗ trống để ghi Entry
		char currentValue[2]; // Giá trị ở vị trí currentOffset
		while (currentOffset < limitSize && !emptyEntry)
		{
			f.seekg(currentOffset, ios::beg);

			f.read((char*)&currentValue, 1);
			currentValue[1] = '\0';

			if (string(currentValue) == "") // Phát hiện có chỗ trống
			{
				emptyEntry = true;
			}
			else
			{
				currentOffset += 32;

			}

		}
		int sizeItem;
		if (!getFileSize(file))

		{
			cout << "Size của folder: " << this->getSizeOfFolder(path) << endl;
			sizeItem = this->getSizeOfFolder(path);
		}
		else {
			sizeItem = getFileSize(file);
		}
		vector<int> availableClusters = fat.findEmptyOffsets(f, file, sizeItem);	// Các cluster trống phù hợp cho file

		cout << "So cluster phai ghi vao: " << availableClusters.size() << endl;

		this->remaningSectors -= availableClusters.size() * clusterSize;


		// Ghi thông tin file vào Entry
		if (emptyEntry)
		{
			fat.writeFAT(f, availableClusters);

			// Nếu không phải folder thì ghi nội dung của file vào vùng DATA
			if (!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				writeFileContent(f, availableClusters, path);

			cout << "GHI VAO OFFSET: " << currentOffset << ", file: " << file.cFileName << endl;
			// Nhảy tới vị trí thích hợp của entry
			f.seekg(currentOffset, ios::beg);


			string fullName = handleFileName(file); // Lấy tên full (loại bỏ file đuôi) 

			if (fullName.size() > 8) // Nếu tên dài hơn 8 ký tự sẽ phát sinh entry phụ
			{
				string subEntry = getSubEntry(fullName);

				// Ghi entry phụ vào file
				for (int i = 0; i < subEntry.size(); i += 32)
				{
					char temp[33]; // mảng char tạm để chép nội dung string sang char
					strcpy_s(temp, subEntry.substr(i, i + 32).c_str());
					f.write((char*)&temp, 32);
				}

			}

			char temp[27];	 // mảng char tạm để chép nội dung string sang char
			strcpy_s(temp, shortName.c_str());

			f.write((char*)&temp, 26);

			// Ghi cluster bắt đầu
			f.write((char*)&availableClusters[0], 2);

			// Ghi file size vào volume
			f.write((char*)&sizeItem, 4);


		}
		else {
			cout << "Khong con cho de ghi" << endl;
		}
		if (f.bad())
		{
			cout << "BAD WRITING! FILE: " << shortName << endl;
			f.clear();
			return;
		}
		cout << endl;
	}

	void addFolder(fstream& f, string folder, int startCluster, FAT& fat, string password)
	{
		WIN32_FIND_DATA fd;

		HANDLE hFind = ::FindFirstFile(folder.c_str(), &fd);
		cout << "Hello, folder: " << fd.cFileName << "-----------------" << endl;
		addFile(f, fd, startCluster, fat, folder, password);

		f.seekp(-6, ios::cur);	// Nhảy tới vị trí cluster đầu


		short firstClusterOfFoler;
		f.read((char*)&firstClusterOfFoler, 2);
		cout << "Cluter bd: " << firstClusterOfFoler << ", offset" << f.tellg() << endl;


		getAllItemsWithinFolder(f, folder, firstClusterOfFoler, fat, "");
	}

	void addItem(fstream& f, string item, bool isFolder, FAT& fat, bool isPassword)
	{
		/*Hàm thêm một item vào vol
		isFolder: True nếu item dc thêm vào là 1 folder, ngược lại là thêm vào 1 file
		isPassword: True nếu người dùng muốn đặt password cho file, folder

		*/
		string password = "";
		if (isPassword)
		{
			do {
				cout << "Nhap password cho item (do dai toi da la 12): ";
				getline(cin, password);
			} while (password.size() == 0 && password.size() > 12);
		}

		if (isFolder)
		{
			long totalSize = getTotalSize(item);
			if (totalSize / sectorSize < this->remaningSectors)
				addFolder(f, item, 0, fat, password);
			else {
				cout << "Khong du dung luong" << endl;
				return;
			}


		}
		else {
			WIN32_FIND_DATA fd;

			HANDLE hFind = ::FindFirstFile(item.c_str(), &fd);
			long totalSize = this->getFileSize(fd);
			if (totalSize / sectorSize < this->remaningSectors)
			{
				addFile(f, fd, 0, fat, item, password);

			}
			else {
				return;
			}
		}
		cout << "So sector con lai: " << this->remaningSectors << endl;
		f.seekg(0x40, ios::beg);
		f.write((char*)&remaningSectors, 4);
	}

	void writeFileContent(fstream& volume, vector<int> clustersOffset, string path)
	{
		fstream file(path, ios::binary | ios::in);
		cout << "FILE PATH: " << path << endl;
		//fstream output(path + "1", ios::binary | ios::out);
		char temp[1024];
		for (auto clusterK : clustersOffset)
		{
			int offset = getCluster(clusterK) * sectorSize;
			volume.seekg(offset, ios::beg);

			int i = 1;
			while (file.read(temp, 1024) && i * 1024 < clusterSize * sectorSize)
			{
				//output.write(temp, file.gcount());
				volume.write(temp, file.gcount());
				i++;
			}
			volume.write(temp, file.gcount());
			//output.write(temp, file.gcount());

		}
		file.close();

	}
	int getCluster(int k) {

		return this->offset + this->size + (k - 2) * clusterSize;
	}

	int getFileSize(WIN32_FIND_DATA fd)
	{
		return (fd.nFileSizeHigh * MAXDWORD) + fd.nFileSizeLow;
	}

};


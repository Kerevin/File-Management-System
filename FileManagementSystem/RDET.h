#pragma once
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <direct.h>
#include "FAT.h"

using namespace std;
struct File
{
	string name;
	string extension;
	int firstCluster;
	long size;
	string password = "";
	bool att;
	bool isPassword;
};
class RDET
{
private:
	int size;		 // Size của RDET, đơn vị sector
	int offset;		// Vị trí của RDET trong Volume, đơn vị sector
	int sectorSize; // Size của mỗi sector
	int clusterSize; // Số sector của cluster
	int totalEmptyCluster; // Tổng số cluster trống
	int remaningSectors; // Số sector trống còn lại

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


		cout << "Dang ghi file: " << shortName << "cluster thu: " << pivotCluster << ", getCluster: " << getCluster(pivotCluster) << endl;

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
		long sizeItem = getFileSize(file);
		if (!sizeItem) // Nếu là folder
		{
			cout << "Size của folder: " << this->getSizeOfFolder(path) << endl;
			sizeItem = this->getSizeOfFolder(path);
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
				writeFileContent(f, availableClusters, path, sizeItem);

			cout << "GHI VAO OFFSET: " << currentOffset << ", file: " << file.cFileName << endl;
			// Nhảy tới vị trí thích hợp của entry
			f.seekg(currentOffset, ios::beg);


			string fullName = file.cFileName; // Lấy tên full (loại bỏ file đuôi) 

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

	string getFileExtension(string fileName)
	{

		transform(fileName.begin(), fileName.end(), fileName.begin(), ::toupper); // In hoa chữ cái
		if (fileName.rfind('.') >= 0)
			return fileName.substr(fileName.rfind('.') + 1);
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
		return newName;
	}

	WIN32_FIND_DATA processShortName(WIN32_FIND_DATA fileName)
	{

		strcpy_s(fileName.cFileName, getShortName(fileName).c_str());
		return fileName;
	}

	void writeFileContent(fstream& volume, vector<int> clustersOffset, string path, long fileSize)
	{
		fstream file(path, ios::binary | ios::in);
		cout << "FILE PATH: " << path << endl;

		char temp[1024];
		for (int i = 0; i < clustersOffset.size() - 1; i++)
		{
			int offsets = getCluster(clustersOffset[i]) * sectorSize;
			volume.seekg(offsets, ios::beg);

			int k = 1;
			while (file.read(temp, 1024) && k * 1024 < clusterSize * sectorSize)
			{
				//output.write(temp, file.gcount());
				volume.write(temp, file.gcount());
				i++;
			}
			volume.write(temp, file.gcount());
			//output.write(temp, file.gcount());

		}

		int offsets = this->getCluster(clustersOffset[clustersOffset.size() - 1]) * sectorSize;
		volume.seekp(offsets, ios::beg);
		int remainingSize = fileSize - (clustersOffset.size() - 1) * sectorSize * clusterSize;

		while (remainingSize > 0)
		{
			if (remainingSize > 1024)
			{
				file.read(temp, 1024);
				volume.write(temp, 1024);
				remainingSize -= 1024;
			}
			else {
				file.read(temp, remainingSize);
				volume.write(temp, remainingSize);
				remainingSize = 0;
			}

		}
		file.close();

	}

	string handleItemName(File n)
	{
		string newName = n.name;
		while (newName[newName.size() - 1] == ' ')
		{
			newName.pop_back();
		}
		return newName;
	}


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
			f += "1";
			f += password;
		}
		else
		{
			f += "0";
		}
		for (int i = password.size(); i < 13; i++)
		{
			f += "\0";
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

	int getSizeOfFolder(string path)
	{
		// Lấy kích thước của tất cả entry có trong folder //
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
							size += getSubEntry(string(fd.cFileName)).size();
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

	int getCluster(int k) {

		return this->offset + this->size + (k - 2) * clusterSize;
	}

	long getFileSize(WIN32_FIND_DATA fd)
	{
		return (fd.nFileSizeHigh * MAXDWORD) + fd.nFileSizeLow;
	}

	void exportFile(fstream& f, fstream& out, FAT& fat, int clusterK, long fileSize)
	{
		vector<int> allClusters = fat.getItemClusters(f, clusterK);
		int currentOffset;
		char temp[1024];
		for (int i = 0; i < allClusters.size() - 1; i++)	// Chạy tới cluster thứ n - 1
		{
			currentOffset = this->getCluster(allClusters[i]) * sectorSize;
			f.seekp(currentOffset, ios::beg);

			int k = 1;	// Số cluster đã đọc được
			while (f.read(temp, 1024) && k * 1024 < clusterSize * sectorSize)
			{

				out.write(temp, f.gcount());
				k++;
			}
			out.write(temp, f.gcount());
		}
		// Xử lý cluster cuối
		currentOffset = this->getCluster(allClusters[allClusters.size() - 1]) * sectorSize;
		f.seekp(currentOffset, ios::beg);

		int remainingSize = fileSize - (allClusters.size() - 1) * sectorSize * clusterSize;


		while (remainingSize > 0)
		{
			if (remainingSize > 1024)
			{
				f.read(temp, 1024);
				out.write(temp, 1024);
				remainingSize -= 1024;
			}
			else {
				f.read(temp, remainingSize);
				out.write(temp, remainingSize);
				remainingSize = 0;
			}

		}



	}

	void exportFolder(fstream& f, string path, FAT& fat, File folder)
	{
		folder.name = handleItemName(folder);
		string p = path + folder.name;
		_mkdir(p.c_str());
		cout << "PATH: " << p << endl;
		vector<File> items = getSubItems(f, fat, folder.firstCluster);
		for (auto x : items)
		{
			if (x.att == 0)
			{
				cout << "File: " << x.name << endl;
				fstream out(p + "/" + x.name, ios::out | ios::binary);
				exportFile(f, out, fat, x.firstCluster, x.size);
			}
			else {
				exportFolder(f, p + "/", fat, x);
			}
		}
	}

	vector<File> getSubItems(fstream& f, FAT& fat, int clusterK = 0)
	{
		int currentOffset;
		vector<int> of;
		if (clusterK == 0)
			currentOffset = this->offset * sectorSize;
		else {
			of = fat.getItemClusters(f, clusterK);

		}

		string item;
		int sizeF;
		short firstCluster;
		vector<File> allFile;
		if (clusterK == 0)
			while (currentOffset < (size + this->offset) * sectorSize)
			{
				char buffer[27];
				f.seekg(currentOffset, ios::beg);
				f.read((char*)&buffer, 26);
				buffer[26] = '\0';

				string temp = string(buffer);
				//cout << temp << endl;
				if (temp.substr(0, 2) == ". ")
				{
					item += temp.substr(2);
				}
				else if (temp != "") {

					f.read((char*)&firstCluster, 2);
					f.read((char*)&sizeF, 4);
					File file;
					file.name = item;
					file.extension = temp.substr(8, 3);
					file.att = temp[11] == '0' ? 0 : 1;
					file.isPassword = temp[12] == '0' ? 0 : 1;
					file.firstCluster = firstCluster;
					file.size = sizeF;
					if (file.isPassword)
					{
						for (auto x : temp.substr(13, 14))
						{
							if (x != '\1')
							{
								file.password += x;
							}
						}
					}
					item = "";

					allFile.push_back(file);
				}

				currentOffset += 32;
			}
		else {
			for (auto cluster : of)
			{
				currentOffset = this->getCluster(cluster) * sectorSize;
				while (currentOffset < (clusterSize + this->getCluster(cluster)) * sectorSize - 32)
				{
					char buffer[27];
					f.seekg(currentOffset, ios::beg);
					f.read((char*)&buffer, 26);
					buffer[26] = '\0';

					string temp = string(buffer);

					if (temp.substr(0, 2) == ". ")
					{
						item.insert(0, temp.substr(2));
					}
					else if (temp != "") {
						if (item.size() == 0)
							item = temp.substr(0, 8);
						f.read((char*)&firstCluster, 2);
						f.read((char*)&sizeF, 4);
						File file;
						file.name = item;
						file.extension = temp.substr(8, 3);
						file.att = temp[11] == '0' ? 0 : 1;
						file.isPassword = temp[12] == '0' ? 0 : 1;
						file.firstCluster = firstCluster;
						file.size = sizeF;
						if (file.isPassword)
						{
							for (auto x : temp.substr(13, 14))
							{
								if (x != '\1')
								{
									file.password += x;
								}
							}
						}
						item = "";

						allFile.push_back(file);
					}

					currentOffset += 32;
				}
			}
		}
		return allFile;
	}

	int showFolder(fstream& f, FAT& fat, int clusterK = 0)
	{

		bool isBack = false;
		string item;
		int sizeF;
		short firstCluster;
		vector<File> allFile = getSubItems(f, fat, clusterK);
	BACK_POSITION:
		for (int i = 0; i < allFile.size(); i++)
		{
			cout << "Tap tin thu " << i + 1 << ": " << allFile[i].name << endl;
		}
		short ch;
		do {
			cout << "Mo foler (chon 1) hoac chon item de export (chon 2) hoac chon 3 de quay ve: ";
			cin >> ch;
			cin.ignore();
		} while (ch != 1 && ch != 2 && ch != 3);
		if (ch == 1)
		{
			do
			{
				cout << "Chon folder nao de mo (chon -1 de thoat): ";
				cin >> ch;
				cin.ignore(0);
			} while (ch > allFile.size() || (ch < 1 && ch != -1));
			if (ch != -1)
			{
				ch--;
				cout << "Chon folder: " << allFile[ch].name << endl;
				if (allFile[ch].att == 1)
					isBack = showFolder(f, fat, allFile[ch].firstCluster) == 3;
			}
		}
		else if (ch == 2) {
			do
			{
				cout << "Chon item nao de mo (chon -1 de thoat): ";
				cin >> ch;
				cin.ignore(0);
			} while (ch > allFile.size());
			if (ch != -1)
			{
				ch--;
				cin.ignore();
				cout << "Chon file: " << allFile[ch].name << endl;

				cout << "Chon duong dan de export item: ";
				string path;
				getline(cin, path);
				if (allFile[ch].att == 0)
				{
					fstream out(path + "/" + allFile[ch].name, ios::out | ios::binary);
					if (out)
						exportFile(f, out, fat, allFile[ch].firstCluster, allFile[ch].size);
					else {
						cout << "Fail to open directory" << endl;
					}
					out.close();
				}
				else {
					exportFolder(f, path, fat, allFile[ch]);
					cout << "DEV chua code phan nay ... " << endl;
				}
			}

		}
		else {
			return 3;
		}
		if (isBack == true)
		{
			cout << "BACK: " << isBack << endl;
			goto BACK_POSITION;
		}
	}


};


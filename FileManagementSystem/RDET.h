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
public:
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
		if (fileExtentionPosition != -1)
		{
			newName = newName.substr(0, fileExtentionPosition);
		}

		return newName;
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


	vector<string> get_all_files_names_within_folder(fstream& f, string folder, int pivotOffset, vector<FAT>& fats)
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


			for (int i = 0; i < shortName.size() - folderName.size(); i++)
			{
				// Ghi nội dung của file
				addFile(f, fileName[i], readFileInfo(shortName[i], getFileExtension((string)fileName[i].cFileName)), pivotOffset, fats, folder + "/" + string(fileName[i].cFileName));


			}
			for (int i = 0; i < folderName.size(); i++)
			{
				// Ghi nội dung của sub folders
				addFolder(f, folder + "/" + folderName[i], readFileInfo(shortName[i + fileName.size() - 1], ""), pivotOffset, fats);

			}
			::FindClose(hFind);
		}
		return folderName;
	}


	void addFile(fstream& f, WIN32_FIND_DATA file, string shortNameInfo, int pivotCluster, vector<FAT>& fats, string path)
	{
		/*Nếu là file của một sub folder thì có pivotCluster
		pivotCluster = cluster bắt đầu nội dung (gồm các file/folder) của sub folder đó
		*/


		int limitSize = pivotCluster == 0 ? (this->size + this->offset) * sectorSize : totalEmptyCluster * clusterSize * sectorSize;
		int currentOffset;	// Vị trí trong bảng RDET để lưu nội dung của entry
		if (pivotCluster == 0)
			currentOffset = this->offset * this->sectorSize; // Offset hiện tại
		else
			currentOffset = getCluster(pivotCluster) * this->sectorSize;
		bool emptyEntry = false;


		cout << "Dang ghi file: " << shortNameInfo << "cluster thứ: " << pivotCluster << endl;

		// Kiếm chỗ trống để ghi Entry
		char currentValue[2]; // Giá trị ở vị trí currentOffset
		while (currentOffset < limitSize && !emptyEntry)
		{
			f.seekg(currentOffset, ios::beg);

			f.read((char*)&currentValue, 1);
			currentValue[1] = '\0';

			if (string(currentValue) == "") // Phát hiện chỗ trống
			{
				emptyEntry = true;
			}
			else
			{
				currentOffset += 32;
				//cout << "Value: " << string(currentValue) << ", offset: " << currentOffset << endl;
			}

		}

		//cout << "Current offset: " << currentOffset << ", empty: " << emptyEntry << ", current value: " << string(currentValue) << endl;

		vector<int> availableClusters = fats[0].findEmptyOffsets(f, file);	// Các cluster trống phù hợp cho file

		cout << "So cluster phai ghi vao: " << availableClusters.size() << endl;
		// Ghi thông tin file vào Entry
		if (emptyEntry)
		{
			fats[0].writeFAT(f, availableClusters);
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
			strcpy_s(temp, shortNameInfo.c_str());

			f.write((char*)&temp, 26);

			// Ghi cluster bắt đầu
			f.write((char*)&availableClusters[0], 2);

			// Ghi file size vào volume
			int fileSize = getFileSize(file);
			f.write((char*)&fileSize, 4);


		}
		if (f.bad())
		{
			cout << "BAD WRITING! FILE: " << shortNameInfo << endl;
			f.clear();
			return;
		}
		cout << endl;
	}

	void addFolder(fstream& f, string folder, string shortName, int startCluster, vector<FAT>& fats)
	{
		WIN32_FIND_DATA fd;

		HANDLE hFind = ::FindFirstFile(folder.c_str(), &fd);

		addFile(f, fd, shortName, startCluster, fats, folder);
		cout << "Hello, folder: " << fd.cFileName << "-----------------" << endl;
		f.seekp(-6, ios::cur);	// Nhảy tới vị trí cluster đầu


		short firstClusterOfFoler;
		f.read((char*)&firstClusterOfFoler, 2);
		cout << "Cluter bd: " << firstClusterOfFoler << ", offset" << f.tellg() << endl;


		get_all_files_names_within_folder(f, folder, firstClusterOfFoler, fats);
	}

	void addItem(fstream& f, string item, bool isFolder, vector<FAT>& fats)
	{
		/*Hàm thêm một item vào vol
		isFolder: True nếu item dc thêm vào là 1 folder, ngược lại là thêm vào 1 file   */

		if (isFolder)
		{
			get_all_files_names_within_folder(f, item, 0, fats);
		}
		else {

		}
	}

	void writeFileContent(fstream& f, vector<int> clustersOffset, string path)
	{
		fstream file(path, ios::binary | ios::in);
		cout << "FILE PATH: " << path << endl;
		char* buffer = new char[clusterSize * sectorSize];
		for (auto x : clustersOffset)
		{
			int offset = getCluster(x);
			f.seekg(offset, ios::beg);
			file.read(buffer, int(clusterSize * sectorSize));
			char temp[1025];
			cout << "ND file: " << buffer << endl;
			int i = 0;
			while (i < strlen(buffer))
			{
				if (strlen(buffer) - 1024 > i)
				{
					strcpy_s(temp, string(buffer).substr(i, i + 1024).c_str());
				}
				else {
					strcpy_s(temp, string(buffer).substr(i).c_str());
				}
				f.write(temp, 1024);
				i += 1024;
			}
		}
		f.close();

	}
	int getCluster(int k) {

		return this->offset + this->size + (k - 2) * clusterSize;
	}

	int getFileSize(WIN32_FIND_DATA fd)
	{
		return (fd.nFileSizeHigh * MAXDWORD) + fd.nFileSizeLow;
	}

};


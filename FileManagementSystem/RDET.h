#pragma once
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <unordered_map> 
using namespace std;
struct Entry
{
	char fileName[8]; // Tên tập tin, offset 0x0
	char fileExtension[3]; // Tên mở rộng, offset 0x8
	unsigned char fileAttribute; // Thuộc tính tập tin, , offset 0xB (11)
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
	int size, offset, sectorSize;


public:

	RDET(int size, int offset, int sectorSize)
	{
		this->size = size;
		this->offset = offset;
		this->sectorSize = sectorSize;
	}
	
	Entry readFileInfo(WIN32_FIND_DATA file)
	{
		Entry entry;
		string name = string(file.cFileName).substr(0, 6);
		std::for_each(name.begin(), name.end(), [](char& c) {
			c = ::toupper(c);
			});
		strcpy_s(entry.fileName, name.c_str());
		entry.fileAttribute = (char)file.dwFileAttributes;
		entry.fileSize = (file.nFileSizeHigh * MAXDWORD) + file.nFileSizeLow;

		return entry;
	}
	vector<WIN32_FIND_DATA> processShortName(vector<WIN32_FIND_DATA> fileName)
	{
		unordered_map<string, vector<int>> uniqueName;
		vector<WIN32_FIND_DATA> shortName;
		vector<string> tempName;
		for (int i = 0; i < fileName.size(); i++)
		{
			uniqueName[string(fileName[i].cFileName).substr(0, 5)].push_back(i);
			shortName.push_back(fileName[i]);
		}
		for (auto x : uniqueName)
		{
			if (x.second.size() > 1)
				for (int i = 0; i < x.second.size(); i++)
				{
					strcpy_s(shortName[x.second[i]].cFileName,string(x.first + "~" + to_string(i + 1)).c_str());
				}
		}
		return shortName;
	}
	vector<string> get_all_files_names_within_folder(fstream &f, string folder)
	{
		
		vector<string> folderName;
		vector<WIN32_FIND_DATA> fileName;
		string search_path = folder + "/*.*";
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);


		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				{
					//cout << "Name: " << fd.cFileName << ", size: " << (fd.nFileSizeHigh * MAXDWORD) + fd.nFileSizeLow << endl;;

					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
					{
						folderName.push_back(fd.cFileName);
					}
					else
					{
						fileName.push_back(fd);
					}

				}
			} while (::FindNextFile(hFind, &fd));
			
			vector<WIN32_FIND_DATA> shortName = processShortName(fileName);
			cout << "Vao day di con di~" << endl;
			for (auto x : shortName)
			{
				cout << x.cFileName << endl;
			}
			
			for (int i = 0; i < fileName.size(); i++)
			{

			
				Entry entry = readFileInfo(fileName[i]);
				cout << "NAME: " << entry.fileName << endl;
				cout << "File attribute: " << int(entry.fileAttribute) << endl;
				cout << "File size: " << entry.fileSize << endl;
				addNewItem(f, entry);

			}
			for (int i = 0; i < folderName.size(); i++)
			{

				get_all_files_names_within_folder(f, folder + "/" + string(folderName[i]));
			}
			::FindClose(hFind);
		}
		return folderName;
	}
	void addNewItem(fstream &f, Entry& entry)
	{
		int currentOffset = this->offset * this->sectorSize; // Offset hiện tại
		bool emptyEntry = false;
		
		
		while (currentOffset < (this->size * this->sectorSize + this->offset) && !emptyEntry)
		{
			f.seekg(currentOffset, ios::beg);
			int currentValue;
			f.read((char*)&currentValue, 4);
			
			if (currentValue == 0 || currentValue == 0xE5)
			{
				emptyEntry = true;			
			}
			else
				currentOffset += 32;
		}
		cout << currentOffset << ", empty: "<< emptyEntry <<  endl;
		if (emptyEntry)
		{
			f.seekg(currentOffset, ios::beg);
			entry.firstCluster = currentOffset;
			f.write((char*)&entry, 32);
		}
	}
	void addItem(fstream& f)
	{

	}

};


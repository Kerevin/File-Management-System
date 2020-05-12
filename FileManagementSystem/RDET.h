#pragma once
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <Windows.h>
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
	int size, offset;
	Entry entryController;
public:
	RDET()
	{
		size = 512;
		offset = 1;
	}
	RDET(int size, int offset)
	{
		this->size = size;
		this->offset = offset;
	}
	vector<string> get_all_files_names_within_folder(string folder)
	{

		vector<string> folderName;
		vector<string> fileName;
		string search_path = folder + "/*.*";
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);


		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				{
					cout << "Name: " << fd.cFileName << ", size: " << (fd.nFileSizeHigh * MAXDWORD) + fd.nFileSizeLow << endl;;

					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
					{
						folderName.push_back(fd.cFileName);
					}
					else
					{
						fileName.push_back(fd.cFileName);
					}

				}
			} while (::FindNextFile(hFind, &fd));
			for (int i = 0; i < folderName.size(); i++)
			{

				vector<string> b = get_all_files_names_within_folder(folder + "/" + string(folderName[i]));

			}
			::FindClose(hFind);
		}
		return folderName;
	}
	void addNewItem()
	{
		int currentOffset = this->offset; // Offset hiện tại
		bool emptyEntry = false;
		while (currentOffset < this->offset && !emptyEntry)
		{

		}
	}
};


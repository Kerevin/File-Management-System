#pragma once

#include "RDET.h"
#include <iomanip>
class FileManagement
{
private:
	BootSector* bs;
	RDET* rd;
	FAT* fat;
	fstream f;
	void initializeFile(int volumeSize)
	{
		int zero[1024] = { 0 };
		for (int i = 0; i < volumeSize * 1024 / 4; ++i)
		{
			f.write((char*)&zero, sizeof(zero));
		}
	}
	void updateFolderSize(int containingFolderCuster, int clusterK, int addingSize)
	{

		int currentOffset;
		int limit = containingFolderCuster == 0 ? (bs->getRDETOffset() + bs->getRDETSize()) * bs->getSectorSize() : (rd->getCluster(containingFolderCuster) + bs->getClusterSector()) * bs->getSectorSize();
		if (containingFolderCuster == 0)
		{

			currentOffset = bs->getRDETOffset() * bs->getSectorSize();
		}
		else {
			currentOffset = rd->getCluster(containingFolderCuster) * bs->getSectorSize();
		}

		vector<int> clusterOfSubEntry;
		while (currentOffset < limit - 32)
		{
			f.seekp(currentOffset, ios::beg); // Nhảy tới cluster đầu của các items trong folder

			char buffer[27];
			f.read(buffer, 26);

			if (string(buffer) != "" && string(buffer) != ". ")
			{
				short firstCluster;

				f.read((char*)&firstCluster, 2);

				if (firstCluster == clusterK)	// Kiểm tra xem phải item mình tìm hay không
				{

					int size;
					f.read((char*)&size, 4);
					size += addingSize;
					f.seekg(-4, ios::cur);
					f.write((char*)&size, 4);
					break;
				}
				clusterOfSubEntry.clear();
			}

			currentOffset += 32;

		}
	}
public:
	FileManagement(long volSize, string path)
	{
		// Contrustor tạo volume mới

		f.open(path, ios::binary | ios::out);
		bs = new BootSector(volSize);
		this->initializeFile(volSize);
		bs->createBootSector(f);
		rd = new RDET(*bs);
		fat = new FAT(*bs);
		f.close();
		f.open(path, ios::binary | ios::out | ios::in);

	}
	FileManagement(string path)
	{
		// Constructor cho đã có sẵn volume
		f.open(path, ios::binary | ios::out | ios::in);

		if (f.fail())
		{
			cout << "Failed to open" << endl;
			cout << "Automatically create new volume";
			f.close();
			f.open(path, ios::binary | ios::out);
			this->initializeFile(1);
			bs->createBootSector(f);
			f.close();
			f.open(path, ios::binary | ios::out | ios::in);
		}
		else {
			bs = new BootSector();
			bs->readBootSector(f);

		}
		rd = new RDET(*bs);
		fat = new FAT(*bs);
	}
	~FileManagement()
	{
		delete bs;
		delete rd;
		delete fat;
		f.close();

	}
	bool checkFileExists(string dirName)
	{
		DWORD attribs = ::GetFileAttributesA(dirName.c_str());
		if (attribs == INVALID_FILE_ATTRIBUTES) {
			return false;
		}
		return ((attribs & FILE_ATTRIBUTE_DIRECTORY) == 0);
	}
	bool checkDirectoryExists(string dirName) {
		DWORD attribs = ::GetFileAttributesA(dirName.c_str());
		if (attribs == INVALID_FILE_ATTRIBUTES) {
			return false;
		}
		return (attribs & FILE_ATTRIBUTE_DIRECTORY);
	}
	void addItem(int containingFolderCluster, int clusterK)
	{
		cout << "IMPORT ITEM" << endl;

		int ch;
		do {
			cout << "Chon import file hay folder (1: file, 2: folder, 0: quay lai): ";
			cin >> ch;

		} while (ch > 2 || ch < 0);

		string path;
		bool isPassword;
		cin.ignore(1);
		if (ch == 0)
		{
			return;
		}
		cout << "Chon duong dan: ";
		getline(cin, path);

		cout << "Co dat password khong?(1: Co, 0: Khong): ";
		cin >> isPassword;
		cin.ignore(1);
		int addingSize;
		if (ch == 1)	// Nếu là file
		{
			if (!checkFileExists(path))
			{
				cout << "Duong dan loi! Khong import file vao duoc..." << endl;
				cout << "An nut bat ky de tiep tuc" << endl;
				getchar();
				return;
			}
			WIN32_FIND_DATA fd;

			HANDLE hFind = ::FindFirstFile(path.c_str(), &fd);
			rd->addItem(f, path, clusterK, false, *fat, isPassword);
			addingSize = rd->getSubEntry(string(fd.cFileName)).size() + 32;
		}
		else {	// Nếu là folder

			if (checkDirectoryExists(path))
				addingSize = rd->getSizeOfFolder(path);
			else {
				cout << "Duong dan loi! Khong import folder vao duoc..." << endl;
				cout << "An nut bat ky de tiep tuc" << endl;
				getchar();
				return;
			}
			rd->addItem(f, path, clusterK, true, *fat, isPassword);
		}
		updateFolderSize(containingFolderCluster, clusterK, addingSize);
	}
	void showFolder(vector<File> allItems)
	{

		if (allItems.size() == 0)
		{
			return;
		}

		for (int i = 0; i < allItems.size(); i++)
		{

			string display = to_string(i + 1) + ". " + rd->handleItemName(allItems[i]);
			for (int i = display.size(); i < 70; i++)
			{
				display += " ";
			}


			if (!allItems[i].att)
			{
				display += allItems[i].extension;
			}
			else {
				display += "Folder";
			}

			for (int i = display.size(); i < 95; i++)
			{
				display += " ";
			}
			if (!allItems[i].att)
				display += to_string(allItems[i].size);
			cout << display << endl;

		}


	}
	void exportItem(File item, string path_out)
	{
		cout << "EXPORT ITEM" << endl;
		if (item.att)
		{
			rd->exportFolder(f, path_out, *fat, item);
		}
		else {
			fstream out(path_out + "/" + item.name, ios::out | ios::binary);

			if (out.fail())
			{
				cout << "Loi export file: " << item.name << endl;
				out.close();
				return;
			}
			try {
				rd->exportFile(f, out, *fat, item.firstCluster, item.size);
			}
			catch (char* e)
			{
				cout << "Khong the export: " << item.name << endl;
			}
			out.close();
		}
	}
	void deleteItem(File item, int containingFolderCluster)
	{

		if (item.att)
		{
			vector<File> subItems = rd->getSubItems(f, *fat, item.firstCluster);
			for (auto sub : subItems)
			{
				deleteItem(sub, item.firstCluster);
			}
			rd->deleteItem(f, *fat, item.firstCluster, containingFolderCluster);
		}
		else {
			rd->deleteItemContent(f, *fat, item.firstCluster);
			rd->deleteItem(f, *fat, item.firstCluster, containingFolderCluster);

		}
	}

	void showMenu()
	{

		int choice;
		int item = 0;
		int currentCluster = 0;	// Cluster của folder hiện tại đang truy vấn
		vector<int> oldCluster;	// Những cluster của folder trước để BACK
		string path;
		string accessingFolderName; // Tên folder đang truy cập
		while (true)
		{
			vector<File> allItems = rd->getSubItems(f, *fat, currentCluster);
			if (currentCluster == 0)
			{
				accessingFolderName = "Folder goc";
			}
			cout << "Dang truy cap: " << accessingFolderName << endl << endl;

			cout << "Name" << setw(70) << "Type" << setw(33) << "Size (Bytes)" << endl;
			showFolder(allItems);
			cout << endl << "-----------------------------" << endl;
			cout << "1. Truy cap folder" << endl;
			cout << "2. Import file/folder vao " << accessingFolderName << endl;
			cout << "3. Export file/folder" << endl;
			cout << "4. Xoa item" << endl;
			cout << "5. Quay ve folder truoc" << endl;
			do {
				cout << ">> ";
				cin >> choice;
				cin.ignore();

			} while (choice > 5 || choice < 0);


			switch (choice)
			{
			case(1):
				if (allItems.size() < 1)
				{
					break;
				}
				do {
					cout << "Chon folder muon truy cap (0 de quay lai): ";
					cin >> item;
					cin.ignore(1);
				} while (item > allItems.size() || item < 0);
				if (item == 0)
				{
					system("cls");
					break;
				}
				if (allItems[item - 1].att == 0)	// Kiểm tra xem item vừa chọn có phải là folder hay không
				{
					cout << "Khong phai la folder. Xin vui long chon folder khac!" << endl;
					getchar();
					system("cls");
					break;
				}

				if (allItems[item - 1].isPassword)
				{

					string password;
					cout << "Nhap password cua item: ";
					getline(cin, password);
					if (password != allItems[item - 1].password)
					{
						cout << "Sai password!" << endl;
						getchar();
						system("cls");
						break;
					}
				}
				oldCluster.push_back(currentCluster);
				accessingFolderName = allItems[item - 1].name;		// Ghi lại tên của folder đang truy cập
				currentCluster = allItems[item - 1].firstCluster;
				system("cls");
				break;

			case(2):
				if (oldCluster.size() > 1)
					addItem(oldCluster[oldCluster.size() - 1], currentCluster);
				else
				{
					addItem(0, currentCluster);
				}
				system("cls");
				break;

			case(3):
				do {
					cout << "Chon item muon export: ";
					cin >> item;
					cin.ignore(1);
				} while (item > allItems.size() || item < 0);


				cout << "Nhap duong dan de export: ";
				getline(cin, path);
				if (!checkDirectoryExists(path))	// Kiểm tra xem folder hợp lệ hay không
				{
					_mkdir(path.c_str());
				}
				if (allItems[item - 1].isPassword)
				{
					string password;
					cout << "Nhap password cua item: ";
					getline(cin, password);
					if (password != allItems[item - 1].password)
					{
						cout << "Sai password!" << endl;
						system("cls");
						break;
					}
				}
				exportItem(allItems[item - 1], path);
				cout << "Nhan phim bat ky de tiep tuc..." << endl;
				getchar();
				system("cls");
				break;
			case(4):
				cout << "Chon item muon xoa: ";
				cin >> item;
				cin.ignore(1);
				if (allItems[item - 1].isPassword)
				{
					string password;
					cout << "Nhap password cua item: ";
					getline(cin, password);
					if (password != allItems[item - 1].password)
					{
						cout << "Sai password!" << endl;
						system("cls");
						break;
					}
				}
				deleteItem(allItems[item - 1], currentCluster);
				system("cls");
				break;
			case(5):
				if (oldCluster.size() > 0)
				{
					currentCluster = oldCluster[oldCluster.size() - 1];
					oldCluster.pop_back();
				}
				system("cls");
				break;

			}
		}

	}
};

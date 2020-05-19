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
	void initializeFile(fstream& f, int volumeSize)
	{
		int zero = 0;
		for (long i = 0; i <= volumeSize * 1024 * 1024 / 4; ++i)
		{
			f.write((char*)&zero, 4);
		}
	}

public:
	FileManagement(long volSize, string path)
	{
		// Contrustor tạo sẵn volume

		f.open(path, ios::binary | ios::in | ios::out);
		if (f.fail())
		{
			cout << "Fail to open" << endl;
		}
		bs = new BootSector(volSize);
		this->initializeFile(f, volSize);
		bs->createBootSector(f);
		rd = new RDET(*bs);
		fat = new FAT(*bs);

	}
	FileManagement(string path)
	{
		// Constructor cho đã có sẵn volume
		f.open(path, ios::binary | ios::in | ios::out);
		if (f.fail())
		{
			cout << "Fail to open" << endl;
		}
		bs = new BootSector();
		bs->readBootSector(f);

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
	void addItem()
	{
		cout << "IMPORT ITEM" << endl;
		int ch;
		do {
			cout << "Chon import file hay folder(1: file, 2: folder): ";
			cin >> ch;

		} while (ch != 1 && ch != 2);
		string path;
		bool isPassword;

		cin.ignore(1);
		cout << "Chon duong dan: ";
		getline(cin, path);

		cout << "Co dat password khong?(1: Co, 0: Khong): ";
		cin >> isPassword;
		cin.ignore(1);
		if (ch == 1)
			rd->addItem(f, path, false, *fat, isPassword);
		else {
			rd->addItem(f, path, true, *fat, isPassword);
		}
	}
	void showFolder(vector<File> allItems)
	{

		if (allItems.size() == 0)
		{
			cout << "Khong co tap tin" << endl;
			return;
		}
		for (int i = 0; i < allItems.size(); i++)
		{
			cout << i + 1 << ". " << rd->handleItemName(allItems[i]);
			if (!allItems[i].att)
			{
				cout << setw(80 - rd->handleItemName(allItems[i]).size()) << allItems[i].size;

			}
			cout << endl;
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
			fstream out(path_out, ios::out | ios::binary);
			rd->exportFile(f, out, *fat, item.firstCluster, item.size);
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
		int cluster = 0;
		vector<int> oldCluster;
		string path;
		while (true)
		{
			vector<File> allItems = rd->getSubItems(f, *fat, cluster);
			showFolder(allItems);
			cout << endl << "-----------------------------" << endl;
			cout << "1. Truy cap folder" << endl;
			cout << "2. Import file/folder vao volume" << endl;
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
				cout << "Chon folder muon truy cap: ";
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
						break;
					}
				}
				oldCluster.push_back(cluster);
				cluster = allItems[item - 1].firstCluster;
				system("cls");
				break;

			case(2):
				addItem();
				system("cls");
				break;

			case(3):
				cout << "Chon item muon export: ";
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
						break;
					}
				}

				cout << "Nhap duong dan de export: ";
				getline(cin, path);
				exportItem(allItems[item - 1], path);
				cout << "Export thanh cong! Nhan phim bat ky de tiep tuc..." << endl;
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
						break;
					}
				}
				deleteItem(allItems[item - 1], cluster);
				system("cls");
				break;
			case(5):
				cluster = oldCluster[oldCluster.size() - 1];
				if (oldCluster.size() > 0)
					oldCluster.pop_back();
				system("cls");
				break;

			}
		}

	}
};
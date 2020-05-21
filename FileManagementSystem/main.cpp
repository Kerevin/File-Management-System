#include "FileManagement.h"
using namespace std;

int main()
{
	int ch;
	cout << "1. Open volume" << endl;
	cout << "2. Create new volume" << endl;
	do {
		cout << ">> ";
		cin >> ch;
	} while (ch > 2 || ch < 0);

	cin.ignore(1);
	string path;
	bool validPath = false;
	do {

		if (ch == 1)
		{
			cout << "Link of volume (*.re file): ";
			getline(cin, path);
			fstream test(path, ios::in | ios::binary);
			if (!test.fail())
			{
				if (path.find(".re") != string::npos)
					validPath = true;
				else {
					cout << "Volume file must be *.re" << endl;
					getchar();
				}
			}
			else {
				cout << "Path error" << endl;
			}
		}
		else {
			cout << "Enter path to save: ";
			getline(cin, path);
			DWORD attribs = ::GetFileAttributesA(path.c_str());
			if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
				validPath = true;
				string name;
				cout << "Enter name of volume: ";
				getline(cin, name);
				path += "/" + name + ".re";
			}
		}

	} while (!validPath);


	system("cls");

	if (ch == 2)
	{
		long volumeSize;
		cout << "Enter size of volume(MB): ";
		cin >> volumeSize;
		cin.ignore(1);
		FileManagement fm(volumeSize, path);
		fm.showMenu();
	}
	else {
		FileManagement fm(path);
		fm.showMenu();
	}

}


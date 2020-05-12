#pragma once
#include "BootSector.h"
#include "RDET.h"

class FileManagement
{
private:
	BootSector* bs;
	RDET* rd;
	int rdOffset; // vị trí của bảng rdet;
	int fatOffset; // vị trí của bảng fat;

public:
	FileManagement(long volSize)
	{
		bs = new BootSector(volSize);
		rdOffset = bs->getRDETOffset();
		fatOffset = bs->getFATOffset();
		rd = new RDET(bs->getRDETSize ,rdOffset);

	}

};
#ifndef	DRVLIST_H
#define DRVLIST_H
extern char SelectedDriverString[];
void	DrvList_PickDriver(HANDLE hInstance, HWND hwndParent, geEngine *Engine, geDriver **Driver, geDriver_Mode **Mode);

#endif


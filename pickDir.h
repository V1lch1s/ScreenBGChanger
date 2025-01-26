#ifndef PICKDIR_H
#define PICKDIR_H

#include <windows.h>
#include <commdlg.h> // GetOpenFileName
#include "resource.h" // Archivo de recursos

int pickDirStandar(char* rutaSeleccionada, HWND hwnd, HMENU hMenu, HANDLE hStopEvent, NOTIFYICONDATA nid, HANDLE hThread);

#endif // PICKDIR_H
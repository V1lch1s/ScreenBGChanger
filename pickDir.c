#include "pickDir.h"
#include "cleanNclose.h"

// Función para abrir un cuadro de diálogo y seleccionar los archivos
int pickDirStandar(char* rutaSeleccionada, HWND hwnd, HMENU hMenu, HANDLE hStopEvent, NOTIFYICONDATA nid, HANDLE hThread) {
    OPENFILENAME ofn; // Estructura para el cuadro de diálogo
    char archivo[260] = ""; // Buffer para la ruta (260 caracteres)

    // Antes de llamar a GetOpenFileName()
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON)));
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON)));

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd; // Ventana Propietaria (NULL si no hay una)
    ofn.lpstrFile = archivo; // Apunta al buffer para almacenar la ruta
    ofn.nMaxFile = sizeof(archivo);
    ofn.lpstrFilter = "Archivos de Imagen\0*.BMP;*.JPEG;*.JPG;*.PNG;*.GIF\0";
    ofn.nFilterIndex = 1; // Índice del filtro predeterminado
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        strcpy(rutaSeleccionada, ofn.lpstrFile); // Copiar la ruta seleccionada
        return 0; // Completado (El archivo puede ser inválido)
    } else {
        // Si el usuario cancela, advertir antes de cerrar la aplición
        if (CommDlgExtendedError() == 0) { // Si no hay error, significa que se canceló
            //cleanNclose(hwnd, hMenu, hStopEvent, nid, hThread);
            return 1; // Cancelado o Error
        }
    }
}
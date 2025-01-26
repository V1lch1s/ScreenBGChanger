#include "cleanNclose.h"

// Función para cerrar la aplicación
void cleanNclose(HWND hwnd, HMENU hMenu, HANDLE hStopEvent, NOTIFYICONDATA nid, HANDLE hThread) {
    MessageBoxW(hwnd, L"El cambiador de fondo se cerrará a continuación.", L"Se cerrará el programa", MB_OK | MB_ICONWARNING);
    PostQuitMessage(0); // Cierra la aplicación
    
    // Limpiamos la memoria RAM
    // Eliminamos el ícono de la bandeja
    Shell_NotifyIcon(NIM_DELETE, &nid);
    DestroyIcon(nid.hIcon); // Liberar el ícono
    
    // Liberamos al menú
    if (hMenu) {
        DestroyMenu(hMenu);
        DestroyMenu(hMenu);
        hMenu = NULL;
    }

    SetEvent(hStopEvent); // Notifica al hilo que debe terminar
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hStopEvent);
}
// Agregamos el ícono: windres resources.rc -o resources.o
// Compilación: gcc BgChanger.c pickDir.c cleanNclose.c resources.o -o BgChanger.exe -mwindows

//#include <unistd.h> // sleep(segundos) 
#include <windows.h>  // Funciones del Sistema Operativo (Windows API). Ej.: Sleep(milisegundos)
#include <tlhelp32.h>  // Tool Help 32. Para obtener información sobre procesos en ejecución (saber si se está ejecutando un proceso).
#include <stdio.h>  // Librería estándar de entrada y salida. Operaciones básicas de archivo y consola.
#include <commdlg.h>  // Maneja diálogos comunes de Windows. GetOpenFileName
#include <string.h> // Funciones de manejo de strings. Ej.: strrchr, strcat, strcmp.
#include "resource.h" // Incluimos el archivo de recursos del ícono
#include "pickDir.h"
#include "cleanNclose.h"

#define WM_TRAYICON (WM_USER + 1)
#define ID_MENU_ITEM1 1
#define ID_MENU_ITEM2 2
#define ID_MENU_SET_DIR1 101
#define ID_MENU_SET_DIR2 102

char dirFondo1[MAX_PATH] = "";
char dirFondo2[MAX_PATH] = "";
char configFile[MAX_PATH] = ""; // Ruta al archivo de configuración .ini

NOTIFYICONDATA nid;
HANDLE hStopEvent; // Evento de Detención
HANDLE hThread; // hMenu
HMENU hMenu;   // Menu
HMENU subMenu; // subMenu

// Esta función se usa para construir el string que contiene la ruta a la que debe estar el archivo de configuración .ini
// En este caso, dicha ruta es la misma que el producto de este código fuente.
// Al final se le añade manualmente el nombre del archivo y su extensión usando strcat(ruta, archivo.ini)
void getExecutablePath(char* pathBuffer, size_t bufferSize) { // El tamaño del buffer siempre debe ser del tamaño del string
    GetModuleFileName(NULL, pathBuffer, bufferSize); 
    char* lastSlash = strrchr(pathBuffer, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0'; // Termina la cadena después de la última barra invertida
    }
}

// Función para guardar los directorios en el archivo .ini
void saveConf() {
    FILE* file = fopen(configFile, "w");
    if (file) {
        fprintf(file, "DirFondo1=%s\n", dirFondo1);
        fprintf(file, "DirFondo2=%s\n", dirFondo2);
        fclose(file);
    }
}

// Esta función carga los directorios desde el archivo .ini
char loadConf(HWND hwnd) {
    int resultado;
    // Obtener la ruta del ejecutable
    getExecutablePath(configFile, sizeof(configFile));
    strcat(configFile, "config.ini"); // Añadimos el nombre del archivo .ini a la ruta

    FILE* file = fopen(configFile, "r");
    
    if (!file) {
        // Si no se encuentra el archivo, pregunta al usuario las rutas
        MessageBoxW(hwnd, L"Elige el fondo que tenías antes de ejecutar la aplicación.", L"Fondo Principal", MB_OK | MB_ICONQUESTION);
        pickDirStandar(dirFondo1, hwnd, hMenu, hStopEvent, nid, hThread);
        MessageBoxW(hwnd, L"Ahora elige el fondo que se debe ver con el juego.", L"Fondo para Jugar", MB_OK | MB_ICONQUESTION);
        pickDirStandar(dirFondo2, hwnd, hMenu, hStopEvent, nid, hThread);
        saveConf(); // Guarda la configuración
        file = fopen(configFile, "r"); // Intenta reabrir el archivo y continuar con la carga
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), file)) {
        if (sscanf(buffer, "DirFondo1=%[^\n]", dirFondo1)) continue;
        if (sscanf(buffer, "DirFondo2=%[^\n]", dirFondo2)) continue;
    }
    fclose(file);

    // Validamos las rutas cargadas
    if (GetFileAttributes(dirFondo1) == INVALID_FILE_ATTRIBUTES) {

        while (GetFileAttributes(dirFondo1) == INVALID_FILE_ATTRIBUTES) {
            MessageBoxW(hwnd, L"El archivo 1 debe ser válido y no estar corrupto.", L"Fondo Principal", MB_OK | MB_ICONINFORMATION);
            resultado = pickDirStandar(dirFondo1, hwnd, hMenu, hStopEvent, nid, hThread);
            if (resultado == 1) { 
                cleanNclose(hwnd, hMenu, hStopEvent, nid, hThread);
                //printf("Cerrando. . .\n");
                return 'F';
            }
        }
        if (resultado != 1) { saveConf(); }
    }
    if (GetFileAttributes(dirFondo2) == INVALID_FILE_ATTRIBUTES) {
        while (GetFileAttributes(dirFondo2) == INVALID_FILE_ATTRIBUTES) {
            MessageBoxW(hwnd, L"El archivo 2 debe ser válido y no estar corrupto.", L"Fondo para Jugar", MB_OK | MB_ICONINFORMATION);
            resultado = pickDirStandar(dirFondo2, hwnd, hMenu, hStopEvent, nid, hThread);
            if (resultado == 1) { 
                cleanNclose(hwnd, hMenu, hStopEvent, nid, hThread);
                //printf("Cerrando. . .\n");
                return 'F'; 
            }
        }
        if (resultado != 1) { saveConf(); }
    }
    return 'T';
}

// Convierte rutas relativas MiPrograma/Fondos/1.png a rutas completas C:/directorio/a/MiPrograma/Fondos/1.png
/*void getWholeRoute(char* rutaRelativa, char* rutaCompleta) {
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0'; // Mantener solo el directorio

    snprintf(rutaCompleta, MAX_PATH, "%s%s", exePath, rutaRelativa);
}*/

// Menú Contextual
void Menu() {
    hMenu = CreatePopupMenu();
    subMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_MENU_ITEM1, "Mostrar Directorios");
    AppendMenu(subMenu, MF_STRING, ID_MENU_SET_DIR1, "Fondo Principal");
    AppendMenu(subMenu, MF_STRING, ID_MENU_SET_DIR2, "Fondo para Jugar");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)subMenu, "Configurar directorios");
    AppendMenu(hMenu, MF_STRING, ID_MENU_ITEM2, "Cerrar BgChanger");
}

void cambiarFondoPantalla(const char* rutaImagen) {
    static char fondoActual[MAX_PATH] = "";

    // Evitar llamadas innecesarias a SystemParametersInfo()
    // strcmp() se utiliza para comparar 2 cadenas de caracteres. En este caso, se busca identificar si
    //     ambas cadenas de caracteres son iguales para determinar si es la misma imagen.
    if (strcmp(fondoActual, rutaImagen) == 0) return; // Si son iguales, la imagen seleccionada ya es el fondo actual.
        
        // Se obtienen los atributos del archivo. Si son inválidos, aparece un mensaje.
    if (GetFileAttributes(rutaImagen) == INVALID_FILE_ATTRIBUTES) {
        MessageBox(NULL, "La imagen no es accesible o no existe.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    // Negación    ___ Función que permite el cambio del fondo de escitorio
    //  |         |              __ Cambio de fondo   __ Parámetros adicionales
    //  |         |             |                   |      __ Ruta de la Imagen
    //  |         |             |                   |     |                      __ Guarda el cambio en configuración
    //  |         |             |                   |     |                     |                   __ Notifica cambios al sistema
    //  v         |             |                   |     |                     |                  |
    if (!SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)rutaImagen, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        // Se intenta cambiar el fondo de pantalla poniendo la llamada a la función como condición. Si no se cumple:
        MessageBox(NULL, "Error al cambiar el fondo de pantalla.", "Error", MB_OK | MB_ICONERROR);
    } else { // Si se cambia con éxito el fondo:
        strcpy(fondoActual, rutaImagen); // Guarda la ruta de la imagen actual como fondo actual.
    }
}

int estaEjecutando(const char* nombreProceso) {
    HANDLE hProceso = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    PROCESSENTRY32 proceso;
    proceso.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hProceso, &proceso)) {
        do {
            if (strcmp(proceso.szExeFile, nombreProceso) == 0) {
                CloseHandle(hProceso);
                return 1; // El proceso está en ejecución
            }

        } while (Process32Next(hProceso, &proceso));
    }

    CloseHandle(hProceso);
    return 0; // El proceso no está en ejecución
}

void agregarIconoBandeja(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

    // Cargar el ícono desde los recursos
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON)); // Ícono
    if (nid.hIcon == NULL) {
        MessageBoxW(hwnd, L"No se pudo cargar el ícono", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    nid.uCallbackMessage = WM_TRAYICON;
    strcpy(nid.szTip, "Background Changer 4 LoL"); // Texto que aparece al pasar el ratón
    nid.szTip[sizeof(nid.szTip) - 1] = '\0'; // Asegurarse de que esté terminado en nulo

    // Truncar el texto si es necesario
    if (strlen(nid.szTip) > 128) {
        nid.szTip[128] = '\0'; // Truncar a 128 caracteres (dejar incompleto el string para que mida 128 caracteres)
    }

    Shell_NotifyIcon(NIM_ADD, &nid); 
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TRAYICON: // Aquí se programa cómo se muestran las opciones
            if (lParam == WM_LBUTTONDOWN) {
                // Click Izquierdo
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                SetForegroundWindow(hwnd); // La ventana debe estar en primer plano
                TrackPopupMenu(hMenu, TPM_LEFTBUTTON, cursorPos.x, cursorPos.y, 0, hwnd, NULL);
            } else if (lParam == WM_RBUTTONDOWN) {
                // Click Derecho
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                SetForegroundWindow(hwnd); // La ventana debe estar en primer plano
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, cursorPos.x, cursorPos.y, 0, hwnd, NULL);
            }
            break;
        case WM_COMMAND: // Aquí se programan las acciones para cada opción
            switch (LOWORD(wParam)) {
                case ID_MENU_ITEM1:
                    MessageBox(hwnd, dirFondo1, "Fondo Principal", MB_OK | MB_ICONINFORMATION);
                    MessageBox(hwnd, dirFondo2, "Fondo para Jugar", MB_OK | MB_ICONINFORMATION);
                    break;
                case ID_MENU_SET_DIR1:
                    pickDirStandar(dirFondo1, hwnd, hMenu, hStopEvent, nid, hThread);
                    saveConf();
                    loadConf(hwnd);
                    break;
                case ID_MENU_SET_DIR2:
                    pickDirStandar(dirFondo2, hwnd, hMenu, hStopEvent, nid, hThread);
                    saveConf();
                    loadConf(hwnd);
                    break;
                case ID_MENU_ITEM2:
                    cleanNclose(hwnd, hMenu, hStopEvent, nid, hThread);
                    break;
            }
            break;
        case WM_DESTROY:
            cleanNclose(hwnd, hMenu, hStopEvent, nid, hThread);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI MonitorThread(LPVOID lpParam) {
    while (WaitForSingleObject(hStopEvent, 1000) == WAIT_TIMEOUT) {  // El bucle principal se termina si el usuario cierra la aplicación     
        /*printf("1\n");          //          ^- Verifica cada 1 segundo si se cumple la condición.

        printf("%s", estaEjecutando("vgc.exe") ? "true" : "false"); 
        printf(" && "); 
        printf("%s", estaEjecutando("LeagueClientUx.exe") ? "true" : "false");
        printf(" || ");
        printf("%s", estaEjecutando("League of Legends.exe") ? "true\n" : "false\n");
        printf("%s", estaEjecutando("vgc.exe") && estaEjecutando("LeagueClientUx.exe") || estaEjecutando("League of Legends.exe") ? "true\n" : "false\n");
        */
        // Espera a que se inicie el Vanguard y el Cliente de LoL
        if (estaEjecutando("vgc.exe")) {
            //printf("2\n");
            // Si el antitrampas y el cliente están ejecutándose, se cambia el fondo del escritorio
            cambiarFondoPantalla(dirFondo2);
            
            // Esperando a que se cierre la aplicación
            while (estaEjecutando("vgc.exe") && WaitForSingleObject(hStopEvent, 1000) == WAIT_TIMEOUT) {} 
            // Con este while, no cambia el fondo de la pantalla cada vez que itera el ciclo principal
            //printf("3\n");
        } else { 
            cambiarFondoPantalla(dirFondo1); // Si se sale del juego, se restaura el fondo regular
            // Esperando a que se cierre la aplicación
            while (!estaEjecutando("vgc.exe") && WaitForSingleObject(hStopEvent, 1000) == WAIT_TIMEOUT) {} 
            // Con este while, no cambia el fondo de la pantalla cada vez que itera el ciclo principal
        }
    }
    cambiarFondoPantalla(dirFondo1); // Siempre debe ser fondo1 lo que quede al final
    return 0;
}

int main() {
    // Crear una ventana oculta
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "BackgroundChanger";
    
    hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hStopEvent) {
        MessageBoxW(NULL, L"No se pudo crear el evento de detención", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    RegisterClass(&wc);
    HWND hwnd = CreateWindow("BackgroundChanger", NULL, 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
    Menu(); // Permite ver el menú de opciones
    agregarIconoBandeja(hwnd);

    if (configFile == "") {
        // Cargar configuraciones
        loadConf(hwnd);
    }

    // Las cargamos 2 veces si no había archivo de configuración
    if (loadConf(hwnd) == 'T') {
        // Creamos un hilo para monitorear los procesos y cambiar el fondo
        HANDLE hThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
        if (hThread == NULL) {
            MessageBox(NULL, "No se pudo crear el Hilo", "Error", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    // Bucle de mensajes
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Esperar a que el hilo termine antes de salir
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    return 0;
}

// Buffer: es una región de memoria utilizada para almacenar temporalmente datos 
//         mientras se están moviendo de un lugar a otro.
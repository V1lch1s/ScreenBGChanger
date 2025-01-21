// Agregamos el ícono: windres resources.rc -o resources.o
// Compilación: gcc BgChanger.c resources.o -o BgChanger.exe -mwindows -encode utf8

//#include <unistd.h> // sleep(segundos) 
#include <windows.h>  // Sleep(milisegundos)
#include <tlhelp32.h>
#include <stdio.h>
#include "resource.h" // Incluimos el archivo de recursos del ícono
#include <commdlg.h>  // GetOpenFileName
#include <string.h> // strrchr, strcat

#define WM_TRAYICON (WM_USER + 1)
#define ID_MENU_ITEM1 1
#define ID_MENU_ITEM2 2
#define ID_MENU_SET_DIR1 101
#define ID_MENU_SET_DIR2 102

char dirFondo1[MAX_PATH] = "";
char dirFondo2[MAX_PATH] = "";
char configFile[MAX_PATH] = ""; // Ruta al archivo de configuración

NOTIFYICONDATA nid;
HANDLE hStopEvent; // Evento de Detención
HANDLE hThread; // hMenu
HMENU hMenu;   // Menu
HMENU subMenu; // subMenu

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

// Función para abrir un cuadro de diálogo y seleccionar los archivos
void pickDir(char* rutaSeleccionada) {
    OPENFILENAME ofn; // Estructura para el cuadro de diálogo
    char archivo[260] = ""; // Buffer para la ruta (260 caracteres)

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // Ventana Propietaria (NULL si no hay una)
    ofn.lpstrFile = archivo; // Apunta al buffer para almacenar la ruta
    ofn.nMaxFile = sizeof(archivo);
    ofn.lpstrFilter = "Archivos de Imagen\0*.BMP;*.JPEG;*.JPG;*.PNG;*.GIF\0";
    ofn.nFilterIndex = 1; // Índice del filtro predeterminado
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        strcpy(rutaSeleccionada, ofn.lpstrFile); // Copiar la ruta seleccionada
    }
}

// Esta función carga los directorios desde el archivo .ini
void loadConf(HWND hwnd) {
    // Obtener la ruta del ejecutable
    getExecutablePath(configFile, sizeof(configFile));
    strcat(configFile, "config.ini"); // Añadimos el nombre del archivo a la ruta del archivo .ini

    FILE* file = fopen(configFile, "r");
    
    if (!file) {
        // Si no se encuentra el archivo, pregunta al usuario las rutas
        MessageBoxW(hwnd, L"Elige el fondo que tenías antes de ejecutar la aplicación.", L"Fondo Principal", MB_OK);
        pickDir(dirFondo1);
        MessageBoxW(hwnd, L"Ahora elige el fondo que se debe ver con el juego.", L"Fondo para Jugar", MB_OK);
        pickDir(dirFondo2);
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
            MessageBoxW(hwnd, L"El archivo debe ser válido y no estar corrupto.", L"Fondo Principal", MB_OK);
            pickDir(dirFondo1);
        }
        saveConf();
    }
    if (GetFileAttributes(dirFondo2) == INVALID_FILE_ATTRIBUTES) {
        while (GetFileAttributes(dirFondo2) == INVALID_FILE_ATTRIBUTES) {
            MessageBoxW(hwnd, L"El archivo debe ser válido y no estar corrupto.", L"Fondo para Jugar", MB_OK);
            pickDir(dirFondo2);
        }
        saveConf();
    }
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

void deleteMenu() {
    if (hMenu) {
        DestroyMenu(hMenu);
        DestroyMenu(hMenu);
        hMenu = NULL;
    }
}

void cambiarFondoPantalla(const char* rutaImagen) {
    static char fondoActual[MAX_PATH] = "";

    // Evitar llamadas innecesarias a SystemParametersInfo()
    if (strcmp(fondoActual, rutaImagen) == 0) return; // Ya es el fondo actual

    if (GetFileAttributes(rutaImagen) == INVALID_FILE_ATTRIBUTES) {
        MessageBox(NULL, "La imagen no es accesible o no existe.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (!SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)rutaImagen, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        // Se intenta cambiar el fondo de pantalla poniendo la llamada a la función como condición. Si no se cumple:
        MessageBox(NULL, "Error al cambiar el fondo de pantalla.", "Error", MB_OK | MB_ICONERROR);
    } else { // Si se cambia con éxito el fondo:
        strcpy(fondoActual, rutaImagen);
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

void eliminarIconoBandeja() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
    DestroyIcon(nid.hIcon); // Liberar el ícono
}

void limpieza() {
    eliminarIconoBandeja();
    deleteMenu(); // Liberamos al menú
    SetEvent(hStopEvent); // Notifica al hilo que debe terminar
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hStopEvent);
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
                    MessageBox(hwnd, dirFondo1, "Fondo Principal", MB_OK);
                    MessageBox(hwnd, dirFondo2, "Fondo para Jugar", MB_OK);
                    break;
                case ID_MENU_SET_DIR1:
                    pickDir(dirFondo1);
                    saveConf();
                    loadConf(hwnd);
                    break;
                case ID_MENU_SET_DIR2:
                    pickDir(dirFondo2);
                    saveConf();
                    loadConf(hwnd);
                    break;
                case ID_MENU_ITEM2:
                    MessageBoxW(hwnd, L"El cambiador de fondo se cerrará a continuación.", L"Se cerrará el programa", MB_OK);
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    limpieza(hwnd, subMenu);
                    break;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            limpieza(hwnd, subMenu);
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
    
    RegisterClass(&wc);
    HWND hwnd = CreateWindow("BackgroundChanger", NULL, 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
    Menu(); // Permite ver el menú de opciones
    agregarIconoBandeja(hwnd);

    if (configFile == "") {
        // Cargar configuraciones
        loadConf(hwnd);
    }

    // Las cargamos 2 veces si no había archivo de configuración
    loadConf(hwnd);
    
    hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hStopEvent) {
        MessageBoxW(NULL, L"No se pudo crear el evento de detención", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Creamos un hilo para monitorear los procesos y cambiar el fondo
    HANDLE hThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
    if (hThread == NULL) {
        MessageBox(NULL, "No se pudo crear el Hilo", "Error", MB_OK | MB_ICONERROR);
        return 1;
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
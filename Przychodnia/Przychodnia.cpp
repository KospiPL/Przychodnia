// Przychodnia.cpp : Definiuje punkt wejścia dla aplikacji.
//

#include "framework.h"
#include "Przychodnia.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <commctrl.h>
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "comctl32.lib")
#define MAX_LOADSTRING 100
#define IDC_LOAD_DATA_BUTTON 101
#define IDC_ADD_PATIENT_BUTTON 102
#define IDC_REMOVE_PATIENT_BUTTON 103
#define IDC_ADD_DOCTOR_BUTTON 104
#define IDC_REMOVE_DOCTOR_BUTTON 105
#define IDD_ADD_PATIENT_DIALOG 101


// Zmienne globalne:
HWND hWndListView;
HINSTANCE hInst;                                // bieżące wystąpienie
WCHAR szTitle[MAX_LOADSTRING];                  // Tekst paska tytułu
WCHAR szWindowClass[MAX_LOADSTRING];


// Przekaż dalej deklaracje funkcji dołączone w tym module kodu:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    

    // Inicjuj ciągi globalne
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PRZYCHODNIA, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Wykonaj inicjowanie aplikacji:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PRZYCHODNIA));

    MSG msg;

    // Główna pętla komunikatów:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}
void DodajElementDoListView(HWND hWndListView, int liczbaKolumn, SQLHSTMT hStmt);

void PobierzDaneZBazy(HWND hWndListView) {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    // Inicjalizacja środowiska ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Alokuje uchwyt połączenia
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    // Nawiązywanie połączenia
    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\MSSQLLocalDB;Database=C:\\USERS\\KACPERCUDZIK\\SOURCE\\REPOS\\PRZYCHODNIA\\CLASSLIBRARY\\DATABASE1.MDF;";
    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        // Wykonuje zapytanie SQL
        SQLWCHAR sqlQuery[] = L"SELECT * FROM PACJENT;";
        retcode = SQLExecDirectW(hStmt, sqlQuery, SQL_NTS);

        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            // Obsługa błędów zapytania
            SQLWCHAR sqlState[1024];
            SQLWCHAR messageText[1024];
            SQLSMALLINT i = 0, textLengthPtr;
            SQLINTEGER nativeError;
            SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, ++i, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(WCHAR), &textLengthPtr);
            wprintf(L"SQL Error: %s - %s\n", sqlState, messageText);
        }
        else {
            SQLSMALLINT liczbaKolumn = 0;
            // Pobiera liczbę kolumn w wyniku zapytania
            SQLNumResultCols(hStmt, &liczbaKolumn);

            // Ładuje dane do ListView
            DodajElementDoListView(hWndListView, liczbaKolumn, hStmt);
        }
    }
    else {
        // Obsługa błędów połączenia
        SQLWCHAR sqlState[1024];
        SQLWCHAR messageText[1024];
        SQLSMALLINT i = 0, textLengthPtr;
        SQLINTEGER nativeError;
        SQLGetDiagRec(SQL_HANDLE_DBC, hDbc, ++i, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(WCHAR), &textLengthPtr);
        while (SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, ++i, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(WCHAR), &textLengthPtr) != SQL_NO_DATA) {
            wprintf(L"SQL Error: %s - %s\n", sqlState, messageText);
        }

        // Zwalnianie zasobów
        if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        if (hDbc) {
            SQLDisconnect(hDbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        }
        if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }
}
// Procedura obsługi dialogu dodawania pacjenta
INT_PTR CALLBACK AddPatientDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t name[100], surname[100], pesel[100], birthdate[100], address[100],
                email[100], phone[100], weight[100], height[100], nfz[100];
            GetDlgItemText(hDlg, IDC_EDIT_NAME, name, 100);
            GetDlgItemText(hDlg, IDC_EDIT_SURNAME, surname, 100);
            GetDlgItemText(hDlg, IDC_EDIT_PESEL, pesel, 100);
            GetDlgItemText(hDlg, IDC_EDIT_BIRTHDATE, birthdate, 100);
            GetDlgItemText(hDlg, IDC_EDIT_ADDRESS, address, 100);
            GetDlgItemText(hDlg, IDC_EDIT_EMAIL, email, 100);
            GetDlgItemText(hDlg, IDC_EDIT_PHONE, phone, 100);
            GetDlgItemText(hDlg, IDC_EDIT_WEIGHT, weight, 100);
            GetDlgItemText(hDlg, IDC_EDIT_HEIGHT, height, 100);
            GetDlgItemText(hDlg, IDC_EDIT_NFZ, nfz, 100);

            // Tu dodaj kod do zapisania danych pacjenta w bazie danych
            // Przykład: INSERT INTO Patients (...) VALUES (...)

            MessageBox(hDlg, L"Pacjent dodany!", L"Sukces", MB_OK);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void DodajPacjenta(HWND hWnd) {
    // Dialogowe okno do wprowadzenia imienia i nazwiska
    wchar_t imie[100];
    wchar_t nazwisko[100];
    wchar_t pesel[100];

    if (DialogBox(hInst, MAKEINTRESOURCE(IDD_ADD_PATIENT_DIALOG), hWnd, AddPatientDialogProc)) {
        MessageBox(hWnd, L"Pacjent dodany pomyślnie!", L"Info", MB_OK);
    }
}


void UsunPacjenta(HWND hWnd) {
    // Tutaj kawałek logiki do pobrania identyfikatora zaznaczonego pacjenta i jego usunięcia z bazy danych
    MessageBox(hWnd, L"Pacjent usunięty!", L"Info", MB_OK);
}
void DodajElementDoListView(HWND hWndListView, int liczbaKolumn, SQLHSTMT hStmt) {
    LVITEM lvItem;
    wchar_t buforTekstu[256];
    SQLWCHAR buforDanych[256];
    SQLLEN dlugoscDanych;

    int indexWiersza = 0;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        for (int i = 1; i <= liczbaKolumn; i++) { // Indeksowanie kolumn zaczyna się od 1
            SQLGetData(hStmt, i, SQL_C_WCHAR, buforDanych, sizeof(buforDanych), &dlugoscDanych);
            wsprintf(buforTekstu, L"%s", buforDanych);

            lvItem = { 0 };
            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = indexWiersza;
            lvItem.iSubItem = i - 1; // ListView subitems zaczynają się od 0
            lvItem.pszText = buforTekstu;

            if (i == 1) {
                // Wstawia nowy wiersz
                ListView_InsertItem(hWndListView, &lvItem);
            }
            else {
                // Uzupełnia wiersz o kolejne kolumny
                ListView_SetItemText(hWndListView, indexWiersza, i - 1, lvItem.pszText);
            }
        }
        indexWiersza++;
    }
}



//
//  FUNKCJA: MyRegisterClass()
//
//  PRZEZNACZENIE: Rejestruje klasę okna.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PRZYCHODNIA));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PRZYCHODNIA);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNKCJA: InitInstance(HINSTANCE, int)
//
//   PRZEZNACZENIE: Zapisuje dojście wystąpienia i tworzy okno główne
//
//   KOMENTARZE:
//
//        W tej funkcji dojście wystąpienia jest zapisywane w zmiennej globalnej i
//        jest tworzone i wyświetlane okno główne programu.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Zapisz uchwyt instancji w zmiennej globalnej

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    hWndListView = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT,
        0, 0,1200, 480, hWnd, (HMENU)IDC_MYLISTVIEW, hInstance, NULL);

    if (!hWndListView) {
        return FALSE;
    }

    LVCOLUMN lvColumn;
    ZeroMemory(&lvColumn, sizeof(lvColumn));
    lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    // Kolumna 0
    wchar_t ID[] = L"ID";
    lvColumn.pszText = ID;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListView, 0, &lvColumn);

    // Kolumna 1
    wchar_t imie[] = L"Imię";
    lvColumn.pszText = imie;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListView, 1, &lvColumn);

    // Kolumna 2
    wchar_t nazwisko[] = L"Nazwisko";
    lvColumn.pszText = nazwisko;
    ListView_InsertColumn(hWndListView, 2, &lvColumn);

    // Kolumna 3
    wchar_t pesel[] = L"PESEL";
    lvColumn.pszText = pesel;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListView, 3, &lvColumn);

    // Kolumna 4
    wchar_t Data_urodzenia[] = L"Data urodzenia";
    lvColumn.pszText = Data_urodzenia;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListView, 4, &lvColumn);

    // Kolumna 5
    wchar_t Adres[] = L"Adres";
    lvColumn.pszText = Adres;
    lvColumn.cx = 200;
    ListView_InsertColumn(hWndListView, 5, &lvColumn);

    // Kolumna 6
    wchar_t Mail[] = L"Mail";
    lvColumn.pszText = Mail;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListView, 6, &lvColumn);

    // Kolumna 7
    wchar_t Telefon[] = L"Telefon";
    lvColumn.pszText = Telefon;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListView, 7, &lvColumn);

    // Kolumna 8
    wchar_t masa_ciała[] = L"Masa ciała";
    lvColumn.pszText = masa_ciała;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListView, 8, &lvColumn);

    // Kolumna 9
    wchar_t wzrost[] = L"Wzrost";
    lvColumn.pszText = wzrost;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListView, 9, &lvColumn);

    // Kolumna 10
    wchar_t Oddział_NFZ[] = L"Oddział NFZ";
    lvColumn.pszText = Oddział_NFZ;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListView, 10, &lvColumn);

    HWND hBtnLoadData = CreateWindow(L"BUTTON", L"Wczytaj dane",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 500, 125, 25,  
        hWnd, (HMENU)IDC_LOAD_DATA_BUTTON, hInstance, NULL);
    HWND hBtnLoad = CreateWindow(L"BUTTON", L"Dodaj Pacjenta",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        255, 500, 125, 25,
        hWnd, (HMENU)IDC_ADD_PATIENT_BUTTON, hInstance, NULL);
    HWND hBtn = CreateWindow(L"BUTTON", L"Usuń Pacjenta",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        135, 500, 125, 25,
        hWnd, (HMENU)IDC_REMOVE_PATIENT_BUTTON, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNKCJA: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PRZEZNACZENIE: Przetwarza komunikaty dla okna głównego.
//
//  WM_COMMAND  - przetwarzaj menu aplikacji
//  WM_PAINT    - Maluj okno główne
//  WM_DESTROY  - opublikuj komunikat o wyjściu i wróć
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {   
        case IDC_LOAD_DATA_BUTTON:
            ListView_DeleteAllItems(hWndListView);
            PobierzDaneZBazy(hWndListView);
            break;
        case IDC_ADD_PATIENT_BUTTON:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ADD_PATIENT_DIALOG), hWnd, AddPatientDialogProc);
            break;
        case IDC_REMOVE_PATIENT_BUTTON:
            UsunPacjenta(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // Tutaj dodaj kod rysujący
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Procedura obsługi komunikatów dla okna informacji o programie.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
void DodajElementDoListView(HWND hWndListView, int liczbaKolumn, SQLHSTMT hStmt);


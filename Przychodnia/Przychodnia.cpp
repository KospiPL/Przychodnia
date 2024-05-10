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
#define IDC_LOAD_DATA_BUTTON 200
#define IDC_ADD_PATIENT_BUTTON 102
#define IDC_REMOVE_PATIENT_BUTTON 103
#define IDC_ADD_DOCTOR_BUTTON 104
#define IDC_REMOVE_DOCTOR_BUTTON 105
#define IDD_ADD_PATIENT_DIALOG 101


// Zmienne globalne:
extern SQLHDBC hDbc;
SQLHDBC hDbc = NULL;
HWND hWndListView;
HWND hWndListLek;
HWND hWndListHarm;
HINSTANCE hInst;                                // bieżące wystąpienie
WCHAR szTitle[MAX_LOADSTRING];                  // Tekst paska tytułu
WCHAR szWindowClass[MAX_LOADSTRING];


// Przekaż dalej deklaracje funkcji dołączone w tym module kodu:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
BOOL DeletePatientFromDatabase(const WCHAR* szID);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PRZYCHODNIA, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PRZYCHODNIA));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
void DodajElementDoListView(HWND hWndListView, int liczbaKolumn, SQLHSTMT hStmt);
////////////////////////////////////////////////////////////////////////////////
// 
// Pacjeci
//Dodawanie danych do tabeli pacjentów i pobieranie
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
    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";

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
//Dodawanie pacjenta
void InsertPatientData(HWND hDlg, const wchar_t* name, const wchar_t* surname, const wchar_t* pesel,
    const wchar_t* birthdate, const wchar_t* address, const wchar_t* email,
    const wchar_t* phone, const wchar_t* weight, const wchar_t* height, const wchar_t* nfz)
{
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    // Inicjalizacja środowiska ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Alokuje uchwyt połączenia
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";

    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        wchar_t sqlQuery[1024];
        swprintf_s(sqlQuery, 1024, L"INSERT INTO PACJENT (IMIE, NAZWISKO, PESEL, DATA_URODZENIA, Adres, mail, TELEFON, MASA_CIAŁA, WZROST, ODDZIAŁ_NFZ) VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');",
            name, surname, pesel, birthdate, address, email, phone, weight, height, nfz);

        // Wykonuje zapytanie SQL
        retcode = SQLExecDirectW(hStmt, sqlQuery, SQL_NTS);

        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            // Obsługa błędów zapytania
            SQLWCHAR sqlState[1024];
            SQLWCHAR messageText[1024];
            SQLSMALLINT textLengthPtr;
            SQLINTEGER nativeError;
            SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(WCHAR), &textLengthPtr);
            MessageBox(hDlg, messageText, L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
        else {
            MessageBox(hDlg, L"Dane pacjenta zapisane pomyślnie.", L"Sukces", MB_OK);
        }
    }
    else {
        MessageBox(hDlg, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    // Zwalnianie zasobów
    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
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

            MessageBox(hDlg, L"Pacjent dodany!", L"Sukces", MB_OK);
            InsertPatientData(hDlg, name, surname, pesel, birthdate, address, email, phone, weight, height, nfz);
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
//Funkcja do usuwania wybranego pacjenta i przkazanie do kolejnej funkcji
void RemoveSelectedPatient(HWND hWndListView) {
    int iSelected = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
    if (iSelected == -1) {
        MessageBox(NULL, L"Proszę wybrać pacjenta do usunięcia.", L"Błąd", MB_OK | MB_ICONERROR);
        return;
    }


    WCHAR szID[256];
    ListView_GetItemText(hWndListView, iSelected, 0, szID, sizeof(szID));

    if (DeletePatientFromDatabase(szID)) {
        ListView_DeleteItem(hWndListView, iSelected);
    }
    else {
        MessageBox(NULL, L"Wystąpił problem podczas usuwania pacjenta.", L"Błąd", MB_OK | MB_ICONERROR);
    }
}
//Usuwanie pacjenta z bazy danych
BOOL DeletePatientFromDatabase(const WCHAR* szID) {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    // Inicjalizacja środowiska ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Alokuje uchwyt połączenia
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";
    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        wchar_t sqlQuery[1024];
        swprintf_s(sqlQuery, 1024, L"DELETE FROM PACJENT WHERE ID = ? ");

        // Wykonuje zapytanie SQL
        retcode = SQLPrepareW(hStmt, sqlQuery, SQL_NTS);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return FALSE;
        }

        // Wiązanie parametru
        retcode = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)szID, 0, NULL);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return FALSE;
        }

        // Wykonanie zapytania
        retcode = SQLExecute(hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            // Obsługa błędów zapytania
            SQLWCHAR sqlState[1024];
            SQLWCHAR messageText[1024];
            SQLSMALLINT textLengthPtr;
            SQLINTEGER nativeError;
            SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(WCHAR), &textLengthPtr);
            MessageBox(hWndListView, messageText, L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
        else {
            MessageBox(NULL, L"Pacjent został usunięty.", L"Sukces", MB_OK);
        }
    }
    else {
        MessageBox(hWndListView, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    // Zwalnianie zasobów
    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

    return (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO);
}
// szukanie pacjentów
void SearchPatientsOrDoctors(HWND hWndListView, const WCHAR* searchText) {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    // Inicjalizacja środowiska ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Alokuje uchwyt połączenia
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";

    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        wchar_t sqlQuery[1024];
        swprintf_s(sqlQuery, 1024, L"SELECT * FROM PACJENT WHERE Imie LIKE '%%%s%%' OR Nazwisko LIKE '%%%s%%' OR PESEL LIKE '%%%s%%';",
            searchText, searchText, searchText);

        // Wykonuje zapytanie SQL
        retcode = SQLExecDirectW(hStmt, sqlQuery, SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            // Czyści poprzednie wyniki
            ListView_DeleteAllItems(hWndListView);
            SQLSMALLINT liczbaKolumn = 0;

            SQLNumResultCols(hStmt, &liczbaKolumn);

            // Ładuje dane do ListView
            DodajElementDoListView(hWndListView, liczbaKolumn, hStmt);
        }
        else {
            // Obsługa błędów zapytania
            MessageBox(hWndListView, L"Błąd wykonania zapytania SQL.", L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
    }
    else {
        // Obsługa błędów połączenia
        MessageBox(hWndListView, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    // Zwalnianie zasobów
    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}
//dodwaniae lekordów do listy pacjentów
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
            lvItem.iSubItem = i - 1;
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
void OpenEditPatientDialog(HWND hWnd) {
    int iSelected = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
    if (iSelected == -1) {
        MessageBox(NULL, L"Proszę wybrać pacjenta do edycji.", L"Błąd", MB_OK | MB_ICONERROR);
        return;
    }

    // Pobierz ID pacjenta
    wchar_t szID[256];
    ListView_GetItemText(hWndListView, iSelected, 0, szID, sizeof(szID));

    // Otwórz okno dialogowe do edycji
    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EDIT_PATIENT_DIALOG), hWnd, EditPatientDialogProc, (LPARAM)szID);
}
void UpdatePatientData(HWND hDlg, const wchar_t* id, const wchar_t* name, const wchar_t* surname, const wchar_t* pesel,
    const wchar_t* birthdate, const wchar_t* address, const wchar_t* email, const wchar_t* phone, const wchar_t* weight, const wchar_t* height, const wchar_t* nfz) {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    // Inicjalizacja środowiska ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Alokuje uchwyt połączenia
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";

    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        wchar_t sqlQuery[2048];
        swprintf_s(sqlQuery, 2048, L"UPDATE PACJENT SET IMIE='%s', NAZWISKO='%s', PESEL='%s', DATA_URODZENIA='%s', Adres='%s', mail='%s', TELEFON='%s', MASA_CIAŁA='%s', WZROST='%s', ODDZIAŁ_NFZ='%s' WHERE ID=%s;",
            name, surname, pesel, birthdate, address, email, phone, weight, height, nfz, id);

        // Wykonuje zapytanie SQL
        retcode = SQLExecDirectW(hStmt, sqlQuery, SQL_NTS);

        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            // Obsługa błędów zapytania
            SQLWCHAR sqlState[1024];
            SQLWCHAR messageText[1024];
            SQLSMALLINT textLengthPtr;
            SQLINTEGER nativeError;
            SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(WCHAR), &textLengthPtr);
            MessageBox(hDlg, messageText, L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
        else {
            MessageBox(hDlg, L"Dane pacjenta zaktualizowane pomyślnie.", L"Sukces", MB_OK);
        }
    }
    else {
        MessageBox(hDlg, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    // Zwalnianie zasobów
    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}

// ladowanie oacjentow do okna edycji 
void LoadPatientData(HWND hDlg, const wchar_t* patientID) {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    // Inicjalizacja środowiska ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Alokuje uchwyt połączenia
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";

    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        wchar_t sqlQuery[1024];
        swprintf_s(sqlQuery, 1024, L"SELECT IMIE, NAZWISKO, PESEL, DATA_URODZENIA, Adres, mail, TELEFON, MASA_CIAŁA, WZROST, ODDZIAŁ_NFZ FROM PACJENT WHERE ID=%s;", patientID);

        // Wykonuje zapytanie SQL
        retcode = SQLExecDirectW(hStmt, sqlQuery, SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            SQLWCHAR name[100], surname[100], pesel[100], birthdate[100], address[100], email[100], phone[100], weight[100], height[100], nfz[100];
            SQLLEN cbName, cbSurname, cbPesel, cbBirthdate, cbAddress, cbEmail, cbPhone, cbWeight, cbHeight, cbNfz;

            // Załadowanie wyników do zmiennych
            SQLBindCol(hStmt, 1, SQL_C_WCHAR, name, sizeof(name), &cbName);
            SQLBindCol(hStmt, 2, SQL_C_WCHAR, surname, sizeof(surname), &cbSurname);
            SQLBindCol(hStmt, 3, SQL_C_WCHAR, pesel, sizeof(pesel), &cbPesel);
            SQLBindCol(hStmt, 4, SQL_C_WCHAR, birthdate, sizeof(birthdate), &cbBirthdate);
            SQLBindCol(hStmt, 5, SQL_C_WCHAR, address, sizeof(address), &cbAddress);
            SQLBindCol(hStmt, 6, SQL_C_WCHAR, email, sizeof(email), &cbEmail);
            SQLBindCol(hStmt, 7, SQL_C_WCHAR, phone, sizeof(phone), &cbPhone);
            SQLBindCol(hStmt, 8, SQL_C_WCHAR, weight, sizeof(weight), &cbWeight);
            SQLBindCol(hStmt, 9, SQL_C_WCHAR, height, sizeof(height), &cbHeight);
            SQLBindCol(hStmt, 10, SQL_C_WCHAR, nfz, sizeof(nfz), &cbNfz);

            if (SQLFetch(hStmt) == SQL_SUCCESS) {
                SetDlgItemText(hDlg, IDC_EDIT_NAME, name);
                SetDlgItemText(hDlg, IDC_EDIT_SURNAME, surname);
                SetDlgItemText(hDlg, IDC_EDIT_PESEL, pesel);
                SetDlgItemText(hDlg, IDC_EDIT_BIRTHDATE, birthdate);
                SetDlgItemText(hDlg, IDC_EDIT_ADDRESS, address);
                SetDlgItemText(hDlg, IDC_EDIT_EMAIL, email);
                SetDlgItemText(hDlg, IDC_EDIT_PHONE, phone);
                SetDlgItemText(hDlg, IDC_EDIT_WEIGHT, weight);
                SetDlgItemText(hDlg, IDC_EDIT_HEIGHT, height);
                SetDlgItemText(hDlg, IDC_EDIT_NFZ, nfz);
            }
        }
        else {
            // Obsługa błędów zapytania
            MessageBox(hDlg, L"Błąd wykonania zapytania SQL.", L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
    }
    else {
        MessageBox(hDlg, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    // Zwalnianie zasobów
    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}

//ładowani wartosci z okna do zapisu do bazy
INT_PTR CALLBACK EditPatientDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static const wchar_t* patientID;

    switch (message) {
    case WM_INITDIALOG:
        patientID = (const wchar_t*)lParam;
        LoadPatientData(hDlg, patientID);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t name[100], surname[100], pesel[100], birthdate[100], address[100], email[100], phone[100], weight[100], height[100], nfz[10];
            GetDlgItemText(hDlg, IDC_EDIT_NAME, name, ARRAYSIZE(name));
            GetDlgItemText(hDlg, IDC_EDIT_SURNAME, surname, ARRAYSIZE(surname));
            GetDlgItemText(hDlg, IDC_EDIT_PESEL, pesel, ARRAYSIZE(pesel));
            GetDlgItemText(hDlg, IDC_EDIT_BIRTHDATE, birthdate, ARRAYSIZE(birthdate));
            GetDlgItemText(hDlg, IDC_EDIT_ADDRESS, address, ARRAYSIZE(address));
            GetDlgItemText(hDlg, IDC_EDIT_EMAIL, email, ARRAYSIZE(email));
            GetDlgItemText(hDlg, IDC_EDIT_PHONE, phone, ARRAYSIZE(phone));
            GetDlgItemText(hDlg, IDC_EDIT_WEIGHT, weight, ARRAYSIZE(weight));
            GetDlgItemText(hDlg, IDC_EDIT_HEIGHT, height, ARRAYSIZE(height));
            GetDlgItemText(hDlg, IDC_EDIT_NFZ, nfz, ARRAYSIZE(nfz));

            UpdatePatientData(hDlg, patientID, name, surname, pesel, birthdate, address, email, phone, weight, height, nfz);
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// 
// Lekarze

// zapisywanie lekarza do bazy danych
void InsertDoctorData(HWND hDlg, const wchar_t* name, const wchar_t* surname, const wchar_t* pesel,
    const wchar_t* num_pwz, const wchar_t* title, const wchar_t* specialization, const wchar_t* email, const wchar_t* phone) 
{
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    // Inicjalizacja środowiska ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Alokuje uchwyt połączenia
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";

    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        wchar_t sqlQuery[1024];
        swprintf_s(sqlQuery, 1024, L"INSERT INTO lekarz (imie, nazwisko, Pesel, Numer_PWZ, Tytuł,  Specja, Email, Telefon) VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');",
            name, surname, pesel, num_pwz, title, specialization, email, phone);

        // Wykonuje zapytanie SQL
        retcode = SQLExecDirectW(hStmt, sqlQuery, SQL_NTS);

        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            // Obsługa błędów zapytania
            SQLWCHAR sqlState[1024];
            SQLWCHAR messageText[1024];
            SQLSMALLINT textLengthPtr;
            SQLINTEGER nativeError;
            SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(WCHAR), &textLengthPtr);
            MessageBox(hDlg, messageText, L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
        else {
            MessageBox(hDlg, L"Dane pacjenta zapisane pomyślnie.", L"Sukces", MB_OK);
        }
    }
    else {
        MessageBox(hDlg, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    // Zwalnianie zasobów
    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}
//Dodawanie nowego leakrza
INT_PTR CALLBACK AddDoctorDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t name[100], surname[100], pesel[100], num_pwz[100], title[100], specialization[100], email[100], phone[100];
            GetDlgItemText(hDlg, IDC_EDIT_NAME1, name, 100);
            GetDlgItemText(hDlg, IDC_EDIT_SURNAME2, surname, 100);
            GetDlgItemText(hDlg, IDC_EDIT_PESEL3, pesel, 100);
            GetDlgItemText(hDlg, IDC_EDIT_NUM, num_pwz, 100);
            GetDlgItemText(hDlg, IDC_EDIT_TYT, title, 100);
            GetDlgItemText(hDlg, IDC_EDIT_SPE, specialization, 100);
            GetDlgItemText(hDlg, IDC_EDIT_EM, email, 100);
            GetDlgItemText(hDlg, IDC_EDIT_TEL, phone, 100);

            MessageBox(hDlg, L"Lekarz dodany!", L"Sukces", MB_OK);
            InsertDoctorData(hDlg, name, surname, pesel, num_pwz, title, specialization, email, phone);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


//szukanie lekarzy
void SearchDoctors(HWND hWndListLek, const WCHAR* searchText) {
        
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    // Inicjalizacja środowiska ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Alokuje uchwyt połączenia
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";

    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        wchar_t sqlQuery[1024];
        swprintf_s(sqlQuery, 1024, L"SELECT * FROM LEKARZ WHERE Imie LIKE '%%%s%%' OR Nazwisko LIKE '%%%s%%' OR PESEL LIKE '%%%s%%';",
            searchText, searchText, searchText);

        // Wykonuje zapytanie SQL
        retcode = SQLExecDirectW(hStmt, sqlQuery, SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            // Czyści poprzednie wyniki
            ListView_DeleteAllItems(hWndListLek);
            SQLSMALLINT liczbaKolumn = 0;

            SQLNumResultCols(hStmt, &liczbaKolumn);

            // Ładuje dane do ListView
            DodajElementDoListView(hWndListLek, liczbaKolumn, hStmt);
        }
        else {
            // Obsługa błędów zapytania
            MessageBox(hWndListLek, L"Błąd wykonania zapytania SQL.", L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
    }
    else {
        // Obsługa błędów połączenia
        MessageBox(hWndListLek, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    // Zwalnianie zasobów
    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}
BOOL DeleteDoctorFromDatabase(const WCHAR* szID) {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    // Inicjalizacja środowiska ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Alokuje uchwyt połączenia
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";
    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        wchar_t sqlQuery[1024];
        swprintf_s(sqlQuery, 1024, L"DELETE FROM Lekarz WHERE ID = ? ");

        // Wykonuje zapytanie SQL
        retcode = SQLPrepareW(hStmt, sqlQuery, SQL_NTS);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return FALSE;
        }

        // Wiązanie parametru
        retcode = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)szID, 0, NULL);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return FALSE;
        }

        // Wykonanie zapytania
        retcode = SQLExecute(hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            // Obsługa błędów zapytania
            SQLWCHAR sqlState[1024];
            SQLWCHAR messageText[1024];
            SQLSMALLINT textLengthPtr;
            SQLINTEGER nativeError;
            SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(WCHAR), &textLengthPtr);
            MessageBox(hWndListLek, messageText, L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
        else {
            MessageBox(NULL, L"Lekarz został usunięty.", L"Sukces", MB_OK);
        }
    }
    else {
        MessageBox(hWndListLek, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    // Zwalnianie zasobów
    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

    return (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO);
}
//Funkcja do usuwania wybranego lekarza i przkazanie do kolejnej funkcji
void RemoveSelectedDoctor(HWND hWndListLek) {
    int iSelected = ListView_GetNextItem(hWndListLek, -1, LVNI_SELECTED);
    if (iSelected == -1) {
        MessageBox(NULL, L"Proszę wybrać pacjenta do usunięcia.", L"Błąd", MB_OK | MB_ICONERROR);
        return;
    }


    WCHAR szID[256];
    ListView_GetItemText(hWndListLek, iSelected, 0, szID, sizeof(szID));

    if (DeleteDoctorFromDatabase(szID)) {
        ListView_DeleteItem(hWndListLek, iSelected);
    }
    else {
        MessageBox(NULL, L"Wystąpił problem podczas usuwania pacjenta.", L"Błąd", MB_OK | MB_ICONERROR);
    }
}
//pobieranie z bazy rekrdow lekarzy
void PobierzLekarzyZBazy(HWND hWndListLek) {
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
    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";

    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        // Alokuje uchwyt zapytania
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

        // Wykonuje zapytanie SQL
        SQLWCHAR sqlQuery[] = L"SELECT * FROM Lekarz;";
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
            DodajElementDoListView(hWndListLek, liczbaKolumn, hStmt);
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
//Dodawanie rekordów do listy lekarzy
void DodajElementDoListLek(HWND hWndListLek, int liczbaKolumn, SQLHSTMT hStmt) {
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
            lvItem.iSubItem = i - 1;
            lvItem.pszText = buforTekstu;

            if (i == 1) {
                // Wstawia nowy wiersz
                ListView_InsertItem(hWndListLek, &lvItem);
            }
            else {
                // Uzupełnia wiersz o kolejne kolumny
                ListView_SetItemText(hWndListLek, indexWiersza, i - 1, lvItem.pszText);
            }
        }
        indexWiersza++;
    }
}



// ladowanie lekarzy do okna edycji 
void LoadDoctorData(HWND hDlg, const wchar_t* doctorID) {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";
    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
        wchar_t sqlQuery[1024];
        swprintf_s(sqlQuery, 1024, L"SELECT Imie, Nazwisko, Pesel, Numer_PWZ, Tytuł, Specja, Email, Telefon FROM LEKARZ WHERE ID=%s;", doctorID);
        retcode = SQLExecDirectW(hStmt, sqlQuery, SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            SQLWCHAR name[100], surname[100], pesel[100], num[100], tyt[100], spec[100], email[100], phone[100];
            SQLBindCol(hStmt, 1, SQL_C_WCHAR, name, sizeof(name), NULL);
            SQLBindCol(hStmt, 2, SQL_C_WCHAR, surname, sizeof(surname), NULL);
            SQLBindCol(hStmt, 3, SQL_C_WCHAR, pesel, sizeof(pesel), NULL);
            SQLBindCol(hStmt, 4, SQL_C_WCHAR, num, sizeof(num), NULL);
            SQLBindCol(hStmt, 5, SQL_C_WCHAR, tyt, sizeof(tyt), NULL);
            SQLBindCol(hStmt, 6, SQL_C_WCHAR, spec, sizeof(spec), NULL);
            SQLBindCol(hStmt, 7, SQL_C_WCHAR, email, sizeof(email), NULL);
            SQLBindCol(hStmt, 8, SQL_C_WCHAR, phone, sizeof(phone), NULL);

            if (SQLFetch(hStmt) == SQL_SUCCESS) {
                SetDlgItemText(hDlg, IDC_EDIT_NAME1, name);
                SetDlgItemText(hDlg, IDC_EDIT_SURNAME2, surname);
                SetDlgItemText(hDlg, IDC_EDIT_PESEL3, pesel);
                SetDlgItemText(hDlg, IDC_EDIT_NUM, num);
                SetDlgItemText(hDlg, IDC_EDIT_TYT, tyt);
                SetDlgItemText(hDlg, IDC_EDIT_SPE, spec);
                SetDlgItemText(hDlg, IDC_EDIT_EM, email);
                SetDlgItemText(hDlg, IDC_EDIT_TEL, phone);
            }
        }
        else {
            MessageBox(hDlg, L"Błąd wykonania zapytania SQL.", L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
    }
    else {
        MessageBox(hDlg, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}


void UpdateDoctorData(HWND hDlg, const wchar_t* id, const wchar_t* name, const wchar_t* surname, const wchar_t* pesel,
    const wchar_t* num, const wchar_t* tyt, const wchar_t* spe, const wchar_t* em, const wchar_t* tel) {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN retcode;

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    SQLWCHAR connectionString[] = L"Driver={ODBC Driver 17 for SQL Server};Server=d-w-c.database.windows.net;Database=przychodnia;UID=Kopsi;PWD=Aslanxd12.;";
    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
        wchar_t sqlQuery[2048];
        swprintf_s(sqlQuery, 2048, L"UPDATE LEKARZ SET Imie='%s', Nazwisko='%s', Pesel='%s', Numer_PWZ='%s', Tytuł='%s', Specja='%s', Email='%s', Telefon='%s' WHERE ID=%s;",
            name, surname, pesel, num, tyt, spe, em, tel, id);

        retcode = SQLExecDirectW(hStmt, sqlQuery, SQL_NTS);

        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
            // Obsługa błędów zapytania
            SQLWCHAR sqlState[1024];
            SQLWCHAR messageText[1024];
            SQLSMALLINT textLengthPtr;
            SQLINTEGER nativeError;
            SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(WCHAR), &textLengthPtr);
            MessageBox(hDlg, messageText, L"Błąd SQL", MB_OK | MB_ICONERROR);
        }
        else {
            MessageBox(hDlg, L"Dane lekarza zaktualizowane pomyślnie.", L"Sukces", MB_OK);
        }
    }
    else {
        MessageBox(hDlg, L"Błąd połączenia z bazą danych.", L"Błąd połączenia", MB_OK | MB_ICONERROR);
    }

    // Zwalnianie zasobów
    if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}
//ładowani wartosci z okna do zapisu do bazy
INT_PTR CALLBACK EditDoctorDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static const wchar_t* doctorID;

    switch (message) {
    case WM_INITDIALOG:
        doctorID = (const wchar_t*)lParam;
        LoadDoctorData(hDlg, doctorID);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t name[100], surname[100], pesel[100], num[100], tyt[100], spe[100], em[100], tel[100];
            GetDlgItemText(hDlg, IDC_EDIT_NAME1, name, ARRAYSIZE(name));
            GetDlgItemText(hDlg, IDC_EDIT_SURNAME2, surname, ARRAYSIZE(surname));
            GetDlgItemText(hDlg, IDC_EDIT_PESEL3, pesel, ARRAYSIZE(pesel));
            GetDlgItemText(hDlg, IDC_EDIT_NUM, num, ARRAYSIZE(num));
            GetDlgItemText(hDlg, IDC_EDIT_TYT, tyt, ARRAYSIZE(tyt));
            GetDlgItemText(hDlg, IDC_EDIT_SPE, spe, ARRAYSIZE(spe));
            GetDlgItemText(hDlg, IDC_EDIT_EM, em, ARRAYSIZE(em));
            GetDlgItemText(hDlg, IDC_EDIT_TEL, tel, ARRAYSIZE(tel));

            UpdateDoctorData(hDlg, doctorID, name, surname, pesel, num, tyt, spe, em, tel);
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
void OpenEditDoctorDialog(HWND hWnd) {
    int iSelected = ListView_GetNextItem(hWndListLek, -1, LVNI_SELECTED);
    if (iSelected == -1) {
        MessageBox(NULL, L"Proszę wybrać lekarza do edycji.", L"Błąd", MB_OK | MB_ICONERROR);
        return;
    }

    // Pobierz ID lekarza
    wchar_t szID[256];
    ListView_GetItemText(hWndListLek, iSelected, 0, szID, sizeof(szID));

    // Otwórz okno dialogowe do edycji
    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EDIT_DOCTOR_DIALOG), hWnd, EditDoctorDialogProc, (LPARAM)szID);
}
////
////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
/// Sortowanie kolumn 
typedef struct {
    HWND hwndList;        // Uchwyt do kontrolki ListView
    int iSortColumn;      // Numer kolumny, po której sortujemy
    BOOL bSortAscending;  // Flaga określająca kierunek sortowania
} SORTINFO;
int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
    SORTINFO* sortInfo = (SORTINFO*)lParamSort;
    HWND hwndList = sortInfo->hwndList;
    int iSortColumn = sortInfo->iSortColumn;
    BOOL bSortAscending = sortInfo->bSortAscending;

    WCHAR szText1[256], szText2[256];
    ListView_GetItemText(hwndList, lParam1, iSortColumn, szText1, sizeof(szText1));
    ListView_GetItemText(hwndList, lParam2, iSortColumn, szText2, sizeof(szText2));

    int iResult = wcscmp(szText1, szText2);
    if (!bSortAscending) iResult = -iResult;

    return iResult;
}
//////////////////////////////////////////////////////////////////////////////
////
//// Harmonogram pracy lekarzy
////


//
//  FUNKCJA: MyRegisterClass()
//
//  PRZEZNACZENIE: Rejestruje klasę okna.
//

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PRZYCHODNIA));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PRZYCHODNIA);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        0, 0, 1200, 480, hWnd, (HMENU)IDC_MYLISTVIEW, hInstance, NULL);
    ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT);
    ShowWindow(hWndListView, SW_HIDE); 

    hWndListLek = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        0, 0, 1000, 480, hWnd, (HMENU)IDC_DOCTOR_LIST, hInstance, NULL);
    ListView_SetExtendedListViewStyle(hWndListLek, LVS_EX_FULLROWSELECT);
    ShowWindow(hWndListLek, SW_HIDE);
    
    hWndListHarm = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        0, 0, 1000, 480, hWnd, (HMENU)IDC_HARMONOGRAM_LISTVIEW, hInstance, NULL);
    ListView_SetExtendedListViewStyle(hWndListHarm, LVS_EX_FULLROWSELECT);
    ShowWindow(hWndListHarm, SW_HIDE);

    if (!hWndListView) {
        return FALSE;
    }

    LVCOLUMN lvColumn;
    ZeroMemory(&lvColumn, sizeof(lvColumn));
    lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    // Tworzenie kolumn dla ListView
    LVCOLUMN lvCol;
    ZeroMemory(&lvCol, sizeof(lvCol));
    lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    // Kolumna "Dzień"
    wchar_t dayColumn[] = L"Dzień";  
    lvCol.pszText = dayColumn;  
    lvCol.cx = 200;  // Szerokość kolumny w pikselach
    ListView_InsertColumn(hWndListHarm, 0, &lvCol);

    // Kolumna "Godzina Rozpoczęcia"
    wchar_t startColumn[] = L"Godzina Rozpoczęcia";
    lvCol.pszText = startColumn;
    lvCol.cx = 200;
    ListView_InsertColumn(hWndListHarm, 1, &lvCol);

    // Kolumna "Godzina Zakończenia"
    wchar_t endColumn[] = L"Godzina Zakończenia";
    lvCol.pszText = endColumn;
    lvCol.cx = 200;
    ListView_InsertColumn(hWndListHarm, 2, &lvCol);

    wchar_t imeilek[] = L"Imie Lekarza";
    lvCol.pszText = imeilek;
    lvCol.cx = 200;
    ListView_InsertColumn(hWndListHarm, 3, &lvCol);

    wchar_t nazlek[] = L"Nazwisko Lekarza";
    lvCol.pszText = nazlek;
    lvCol.cx = 200;
    ListView_InsertColumn(hWndListHarm, 4, &lvCol);

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
    lvColumn.cx = 100;
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

    // Kolumna 0 LEK
    wchar_t IDj[] = L"ID";
    lvColumn.pszText = ID;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListLek, 0, &lvColumn);

    // Kolumna 1 LEK
    wchar_t imiej[] = L"Imię";
    lvColumn.pszText = imie;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListLek, 1, &lvColumn);

    // Kolumna 2 LEK
    wchar_t nazwiskoj[] = L"Nazwisko";
    lvColumn.pszText = nazwisko;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListLek, 2, &lvColumn);

    // Kolumna 3 LEK
    wchar_t peselj[] = L"PESEL";
    lvColumn.pszText = pesel;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListLek, 3, &lvColumn);

    // Kolumna 4
    wchar_t numerPWZ[] = L"Numer PWZ";
    lvColumn.pszText = numerPWZ;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListLek, 4, &lvColumn);

    // Kolumna 5
    wchar_t TYTUL[] = L"Tytuł";
    lvColumn.pszText = TYTUL;
    lvColumn.cx = 200;
    ListView_InsertColumn(hWndListLek, 5, &lvColumn);

    // Kolumna 6
    wchar_t Spec[] = L"Specjalizacja";
    lvColumn.pszText = Spec;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListLek, 6, &lvColumn);

    // Kolumna 7
    wchar_t Ema[] = L"Email";
    lvColumn.pszText = Ema;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListLek, 7, &lvColumn);

    // Kolumna 8
    wchar_t Tel[] = L"Telefon";
    lvColumn.pszText = Tel;
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndListLek, 8, &lvColumn);


    HWND hBtnLoadData = CreateWindow(L"BUTTON", L"Wczytaj dane",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 500, 125, 25,  
        hWnd, (HMENU)IDC_LOAD_DATA_BUTTON, hInstance, NULL);
    ShowWindow(hBtnLoadData, SW_HIDE); 
    HWND hBtnLoad = CreateWindow(L"BUTTON", L"Dodaj Pacjenta",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        260, 500, 125, 25,
        hWnd, (HMENU)IDC_ADD_PATIENT_BUTTON, hInstance, NULL);
    ShowWindow(hBtnLoad, SW_HIDE);
    HWND hBtn = CreateWindow(L"BUTTON", L"Usuń Pacjenta",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        135, 500, 125, 25,
        hWnd, (HMENU)IDC_REMOVE_PATIENT_BUTTON, hInstance, NULL);
    ShowWindow(hBtn, SW_HIDE);
    HWND hBtnpcjentedit = CreateWindow(L"BUTTON", L"Edytuj Pacjenta",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        379, 500, 125, 25,
        hWnd, (HMENU)IDC_EDIT_PATIENT_BUTTON, hInstance, NULL);
    ShowWindow(hBtnpcjentedit, SW_HIDE);
    HWND hDoctorLoad = CreateWindow(L"BUTTON", L"Wczytaj dane",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 500, 125, 25,
        hWnd, (HMENU)IDC_DOCTOR_LOAD, hInstance, NULL);
    ShowWindow(hDoctorLoad, SW_HIDE);
    HWND hDoctorAdd = CreateWindow(L"BUTTON", L"Dodaj Lekarza",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        260, 500, 125, 25,
        hWnd, (HMENU)IDC_DOCTOR_ADD, hInstance, NULL);
    ShowWindow(hDoctorAdd, SW_HIDE);
    HWND hDoctorDelete = CreateWindow(L"BUTTON", L"Usuń Lekarza",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        379, 500, 125, 25,
        hWnd, (HMENU)IDC_DOCTOR_DELETE, hInstance, NULL);
    ShowWindow(hDoctorDelete, SW_HIDE);
    HWND hDoctorEdit = CreateWindow(L"BUTTON", L"Edytuj Lekarza",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        135, 500, 125, 25,
        hWnd, (HMENU)IDC_DOCTOR_EDIT, hInst, NULL);
    ShowWindow(hDoctorEdit, SW_HIDE);
    HWND hSearchBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_AUTOHSCROLL,
        1200, 0, 250, 30, hWnd, (HMENU)IDC_SEARCH_BOX, hInstance, NULL);
    ShowWindow(hSearchBox, SW_HIDE);
    HWND hSearchButton = CreateWindow(L"BUTTON", L"Szukaj Pacjenta",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        1450, 0, 110, 30, hWnd, (HMENU)IDC_SEARCH_BUTTON, hInstance, NULL);
    ShowWindow(hSearchButton, SW_HIDE);
    HWND hSearchBoxDoctor = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_AUTOHSCROLL,
        1000, 0, 250, 30, hWnd, (HMENU)IDC_SEARCH_BOX_DOCTOR, hInstance, NULL);
    ShowWindow(hSearchBoxDoctor, SW_HIDE);
    HWND hSearchButtonDoctor = CreateWindow(L"BUTTON", L"Szukaj Lekarza",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        1250, 0, 110, 30, hWnd, (HMENU)IDC_SEARCH_BUTTON_DOCTOR, hInstance, NULL);
    ShowWindow(hSearchButtonDoctor, SW_HIDE);
    HWND hHarLoad = CreateWindow(L"BUTTON", L"Wczytaj dane",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 500, 125, 25,
        hWnd, (HMENU)IDC_HAR_LOAD, hInstance, NULL);
    ShowWindow(hHarLoad, SW_HIDE);
    HWND hHarrAdd = CreateWindow(L"BUTTON", L"Dodaj wpis pracy",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        260, 500, 125, 25,
        hWnd, (HMENU)IDC_HAR_ADD, hInstance, NULL);
    ShowWindow(hHarrAdd, SW_HIDE);
    HWND hHarDelete = CreateWindow(L"BUTTON", L"Usuń wpis pracy",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        379, 500, 125, 25,
        hWnd, (HMENU)IDC_HAR_DELETE, hInstance, NULL);
    ShowWindow(hHarDelete, SW_HIDE);
    HWND hHarEdit = CreateWindow(L"BUTTON", L"Edytuj wpis pracy",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        135, 500, 125, 25,
        hWnd, (HMENU)IDC_HAR_EDIT, hInst, NULL);
    ShowWindow(hHarEdit, SW_HIDE);



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
    case WM_NOTIFY:
    {
        LPNMHDR lpnmh = (LPNMHDR)lParam;
        if (lpnmh->hwndFrom == hWndListView || lpnmh->hwndFrom == hWndListLek)
        {
            switch (lpnmh->code)
            {
            case LVN_COLUMNCLICK:
            {
                NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
                SORTINFO sortInfo;
                sortInfo.hwndList = lpnmh->hwndFrom;
                sortInfo.iSortColumn = pnmv->iSubItem;
                static BOOL bSortAscending = TRUE;
                bSortAscending = !bSortAscending;
                sortInfo.bSortAscending = bSortAscending;

                ListView_SortItemsEx(sortInfo.hwndList, CompareFunc, &sortInfo);
            }
            break;
            }
        }
    }
    break;
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
            RemoveSelectedPatient(hWndListView);
            break;
        case IDC_EDIT_PATIENT_BUTTON:
            OpenEditPatientDialog(hWnd); 
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDC_DOCTOR_ADD:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ADD_DOCTOR_DIALOG), hWnd, AddDoctorDialogProc);
            break;
        case IDC_DOCTOR_LOAD:
            ListView_DeleteAllItems(hWndListLek);
            PobierzLekarzyZBazy(hWndListLek);
            break;
        case IDC_DOCTOR_DELETE:
            RemoveSelectedDoctor(hWndListLek);
            break;
        case IDC_DOCTOR_EDIT:
            OpenEditDoctorDialog(hWnd);
            break;
        case IDM_LEK:  
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_ADD), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_DELETE), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_LIST), SW_SHOW); 
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_LOAD), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_EDIT), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_LOAD_DATA_BUTTON), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_REMOVE_PATIENT_BUTTON), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_ADD_PATIENT_BUTTON), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_MYLISTVIEW), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_PATIENT_BUTTON), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BUTTON), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BOX), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BUTTON_DOCTOR), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BOX_DOCTOR), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_DELETE), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_ADD), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_EDIT), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_HARMONOGRAM_LISTVIEW), SW_HIDE);
            break;
        case IDM_WIZ:
            break;
        case IDM_PAC:
            ShowWindow(GetDlgItem(hWnd, IDC_LOAD_DATA_BUTTON), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_REMOVE_PATIENT_BUTTON), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_ADD_PATIENT_BUTTON), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_MYLISTVIEW), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_ADD), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_DELETE), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_EDIT), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_LIST), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_LOAD), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_PATIENT_BUTTON), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BUTTON), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BOX), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BUTTON_DOCTOR), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BOX_DOCTOR), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_LOAD), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_DELETE), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_ADD), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_EDIT), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_HARMONOGRAM_LISTVIEW), SW_HIDE);
            break;
        case IDM_HAR:           
            ShowWindow(GetDlgItem(hWnd, IDC_HARMONOGRAM_LISTVIEW), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_LOAD), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_DELETE), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_ADD), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_HAR_EDIT), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_LIST), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_LOAD_DATA_BUTTON), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_REMOVE_PATIENT_BUTTON), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_ADD_PATIENT_BUTTON), SW_HIDE); 
            ShowWindow(GetDlgItem(hWnd, IDC_MYLISTVIEW), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_ADD), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_DELETE), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_EDIT), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_LIST), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_DOCTOR_LOAD), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_PATIENT_BUTTON), SW_HIDE); 
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BUTTON), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BOX), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BUTTON_DOCTOR), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_SEARCH_BOX_DOCTOR), SW_HIDE);

            break;
        case IDC_SEARCH_BUTTON: 
            if (LOWORD(wParam) == IDC_SEARCH_BUTTON) {
                WCHAR hSearchBox[256];
                GetDlgItemText(hWnd, IDC_SEARCH_BOX, hSearchBox, 256);
                SearchPatientsOrDoctors(hWndListView, hSearchBox);
            }
            break;
        case IDC_SEARCH_BUTTON_DOCTOR:
            WCHAR searchTextDoctor[256];
            GetDlgItemText(hWnd, IDC_SEARCH_BOX_DOCTOR, searchTextDoctor, 256);
            SearchDoctors(hWndListLek, searchTextDoctor);
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


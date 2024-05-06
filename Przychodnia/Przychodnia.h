#pragma once

#include <windows.h>
#include <commctrl.h>
#include "resource.h"

// Global variables
extern HINSTANCE hInst;
extern HWND hWndListView;

// Function prototypes
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AddPatientDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK EditPatientDialogProc(HWND, UINT, WPARAM, LPARAM);
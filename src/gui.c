#include "steg.h"
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Window dimensions */
#define WINDOW_WIDTH  450
#define WINDOW_HEIGHT 280

/* Control IDs */
#define ID_BROWSE_COVER     1001
#define ID_BROWSE_PAYLOAD   1002
#define ID_BROWSE_OUTPUT    1003
#define ID_EMBED_BUTTON     1004
#define ID_EXTRACT_BUTTON   1005
#define ID_COVER_EDIT       1006
#define ID_PAYLOAD_EDIT     1007
#define ID_OUTPUT_EDIT      1008

/* Global variables */
static HWND g_hWnd;
static HWND g_hCoverEdit, g_hPayloadEdit, g_hOutputEdit;
static HWND g_hOutputLabel, g_hOutputBrowseBtn;
static HWND g_hStatus;
static bool g_extractMode = false;

/* Function prototypes */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hwnd);
BOOL GetOpenFileName_Custom(HWND hwnd, char* buffer, const char* filter, const char* title);
BOOL GetSaveFileName_Custom(HWND hwnd, char* buffer, const char* filter, const char* title);
void OnEmbedClicked(void);
void OnExtractClicked(void);
void UpdateStatus(const char* message);
void SetMode(bool extractMode);

/* Entry point */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    
    /* Register window class */
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "pxplGUI";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window registration failed!", "Error", MB_ICONERROR);
        return 1;
    }
    
    /* Create main window */
    g_hWnd = CreateWindowEx(
        0,
        "pxplGUI",
        "Pixel Payload",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    
    if (!g_hWnd) {
        MessageBox(NULL, "Window creation failed!", "Error", MB_ICONERROR);
        return 1;
    }
    
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    
    /* Message loop */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}

/* Window procedure */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            CreateControls(hwnd);
            UpdateStatus("Ready - Select files to embed or extract data");
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BROWSE_COVER: {
                    char buffer[MAX_PATH] = { 0 };
                    const char* title = g_extractMode ? "Select Steg Image" : "Select Cover Image";
                    if (GetOpenFileName_Custom(hwnd, buffer, 
                        "PNG Files\0*.png\0All Files\0*.*\0", title)) {
                        SetWindowText(g_hCoverEdit, buffer);
                    }
                    break;
                }
                case ID_BROWSE_PAYLOAD: {
                    char buffer[MAX_PATH] = { 0 };
                    if (g_extractMode) {
                        if (GetSaveFileName_Custom(hwnd, buffer, 
                            "All Files\0*.*\0Binary Files\0*\0Text Files\0*.txt\0", 
                            "Save Extracted Data As")) {
                            SetWindowText(g_hPayloadEdit, buffer);
                        }
                    } else {
                        if (GetOpenFileName_Custom(hwnd, buffer, 
                            "All Files\0*.*\0", "Select Payload File")) {
                            SetWindowText(g_hPayloadEdit, buffer);
                        }
                    }
                    break;
                }
                case ID_BROWSE_OUTPUT: {
                    char buffer[MAX_PATH] = { 0 };
                    if (GetSaveFileName_Custom(hwnd, buffer, 
                        "PNG Files\0*.png\0All Files\0*.*\0", 
                        "Save Output Image As")) {
                        SetWindowText(g_hOutputEdit, buffer);
                    }
                    break;
                }
                case ID_EMBED_BUTTON:
                    if (g_extractMode) {
                        OnExtractClicked();
                    } else {
                        OnEmbedClicked();
                    }
                    break;
                case ID_EXTRACT_BUTTON:
                    SetMode(!g_extractMode);
                    break;
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

/* Create all controls */
void CreateControls(HWND hwnd) {
    int y = 20;
    
    /* File 1 (Cover/Steg Image) */
    CreateWindow("STATIC", "Image:", WS_VISIBLE | WS_CHILD,
                20, y, 80, 20, hwnd, NULL, NULL, NULL);
    g_hCoverEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                               110, y, 240, 20, hwnd, (HMENU)ID_COVER_EDIT, NULL, NULL);
    CreateWindow("BUTTON", "Browse", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                360, y, 60, 22, hwnd, (HMENU)ID_BROWSE_COVER, NULL, NULL);
    y += 35;
    
    /* File 2 (Payload/Extract) */
    CreateWindow("STATIC", "Data:", WS_VISIBLE | WS_CHILD,
                20, y, 80, 20, hwnd, NULL, NULL, NULL);
    g_hPayloadEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                 110, y, 240, 20, hwnd, (HMENU)ID_PAYLOAD_EDIT, NULL, NULL);
    CreateWindow("BUTTON", "Browse", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                360, y, 60, 22, hwnd, (HMENU)ID_BROWSE_PAYLOAD, NULL, NULL);
    y += 35;
    
    /* Output file (only for embed mode) */
    g_hOutputLabel = CreateWindow("STATIC", "Output:", WS_VISIBLE | WS_CHILD,
                20, y, 80, 20, hwnd, NULL, NULL, NULL);
    g_hOutputEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                110, y, 240, 20, hwnd, (HMENU)ID_OUTPUT_EDIT, NULL, NULL);
    g_hOutputBrowseBtn = CreateWindow("BUTTON", "Browse", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                360, y, 60, 22, hwnd, (HMENU)ID_BROWSE_OUTPUT, NULL, NULL);
    y += 50;
    
    /* Action buttons */
    CreateWindow("BUTTON", "Embed Data", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON,
                20, y, 120, 30, hwnd, (HMENU)ID_EMBED_BUTTON, NULL, NULL);
    CreateWindow("BUTTON", "Switch to Extract", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                280, y, 140, 30, hwnd, (HMENU)ID_EXTRACT_BUTTON, NULL, NULL);
    y += 45;
    
    /* Status bar */
    g_hStatus = CreateWindow("STATIC", "Ready", WS_VISIBLE | WS_CHILD | SS_SUNKEN,
                            20, y, 400, 20, hwnd, NULL, NULL, NULL);
}

/* Set interface mode */
void SetMode(bool extractMode) {
    g_extractMode = extractMode;
    
    if (extractMode) {
        SetWindowText(GetDlgItem(g_hWnd, ID_EMBED_BUTTON), "Extract Data");
        SetWindowText(GetDlgItem(g_hWnd, ID_EXTRACT_BUTTON), "Switch to Embed");
        ShowWindow(g_hOutputEdit, SW_HIDE);
        ShowWindow(g_hOutputLabel, SW_HIDE);
        ShowWindow(g_hOutputBrowseBtn, SW_HIDE);
        UpdateStatus("Extract mode - Select steg image and output file");
    } else {
        SetWindowText(GetDlgItem(g_hWnd, ID_EMBED_BUTTON), "Embed Data");
        SetWindowText(GetDlgItem(g_hWnd, ID_EXTRACT_BUTTON), "Switch to Extract");
        ShowWindow(g_hOutputEdit, SW_SHOW);
        ShowWindow(g_hOutputLabel, SW_SHOW);
        ShowWindow(g_hOutputBrowseBtn, SW_SHOW);
        UpdateStatus("Embed mode - Select cover image, payload, and output file");
    }
    
    /* Clear all fields when switching modes */
    SetWindowText(g_hCoverEdit, "");
    SetWindowText(g_hPayloadEdit, "");
    SetWindowText(g_hOutputEdit, "");
}

/* File dialog helpers */
BOOL GetOpenFileName_Custom(HWND hwnd, char* buffer, const char* filter, const char* title) {
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    
    return GetOpenFileName(&ofn);
}

BOOL GetSaveFileName_Custom(HWND hwnd, char* buffer, const char* filter, const char* title) {
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    
    /* Set default extension based on filter */
    if (strstr(filter, "PNG Files") != NULL) {
        ofn.lpstrDefExt = "png";
    } else if (strstr(filter, "Text Files") != NULL && strstr(title, "Extract") == NULL) {
        ofn.lpstrDefExt = "txt";
    }
    /* For extracted data (binary by default), don't set a default extension */
    
    return GetSaveFileName(&ofn);
}

/* Embed operation */
void OnEmbedClicked(void) {
    char cover[MAX_PATH], payload[MAX_PATH], output[MAX_PATH];
    
    GetWindowText(g_hCoverEdit, cover, MAX_PATH);
    GetWindowText(g_hPayloadEdit, payload, MAX_PATH);
    GetWindowText(g_hOutputEdit, output, MAX_PATH);
    
    if (strlen(cover) == 0 || strlen(payload) == 0 || strlen(output) == 0) {
        MessageBox(g_hWnd, "Please select all required files.", "Missing Files", MB_ICONWARNING);
        return;
    }
    
    UpdateStatus("Embedding data...");
    
    if (steg_embed(cover, payload, output)) {
        UpdateStatus("Data embedded successfully!");
        MessageBox(g_hWnd, "Data has been successfully hidden in the image.", "Success", MB_ICONINFORMATION);
    } else {
        UpdateStatus("Failed to embed data.");
        MessageBox(g_hWnd, "Failed to embed data. Check file formats and permissions.", "Error", MB_ICONERROR);
    }
}

/* Extract operation */
void OnExtractClicked(void) {
    char steg[MAX_PATH], extract[MAX_PATH];
    
    GetWindowText(g_hCoverEdit, steg, MAX_PATH);
    GetWindowText(g_hPayloadEdit, extract, MAX_PATH);
    
    if (strlen(steg) == 0 || strlen(extract) == 0) {
        MessageBox(g_hWnd, "Please select steg image and output file.", "Missing Files", MB_ICONWARNING);
        return;
    }
    
    UpdateStatus("Extracting data...");
    
    if (steg_extract(steg, extract)) {
        UpdateStatus("Data extracted successfully!");
        MessageBox(g_hWnd, "Hidden data has been successfully extracted.", "Success", MB_ICONINFORMATION);
    } else {
        UpdateStatus("Failed to extract data.");
        MessageBox(g_hWnd, "Failed to extract data. Image may not contain hidden data.", "Error", MB_ICONERROR);
    }
}

/* Update status bar */
void UpdateStatus(const char* message) {
    SetWindowText(g_hStatus, message);
    UpdateWindow(g_hStatus);
}

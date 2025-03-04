// AntSimulation.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "AntSimulation.h"
#include <vector>
#include <map>
#include <random>
#include <cmath>
#include <d2d1.h>
#include <chrono> // Add this line for time measurement
#pragma comment(lib, "d2d1")

#define MAX_LOADSTRING 100
#define TIMER_ID 1
#define TIMER_INTERVAL 1


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
RECT clientRect;
HWND hWnd;

ID2D1Factory* pD2DFactory = NULL;
ID2D1HwndRenderTarget* pRenderTarget = NULL;
ID2D1SolidColorBrush* pBrush = NULL;

// Add variables to store draw time
std::chrono::high_resolution_clock::time_point startTime;
std::chrono::high_resolution_clock::time_point endTime;
double drawTime = 0.0;
double fps = 0.0;


struct Ant
{
    bool hasfood;
    int x, y;
    int direction;
};

const int AntCount = 5000;
POINT clickPosition;
bool isClicked = false;
std::map<int, Ant> Ants;
const int antDrawRadius = 4;
const int foodDrawSize = 2;
const int scanSize = 4;
const double PI = 3.14159265358979323846;
const int MAX_TURN_ANGLE = 30;
const int homeX = 50;
const int homeY = 50;
const int homeRadius = 20;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> angleDist(-MAX_TURN_ANGLE, MAX_TURN_ANGLE);

bool** foodGrid = nullptr; // 2D array to store food presence
int** greenPheromone = nullptr;
int** bluePheromone = nullptr;

void InitDirect2D(HWND);
void DrawScreen();

bool IsInsideCircle(int pixelX, int pixelY, int centerX, int centerY, int radius) {
    int dx = pixelX - centerX;
    int dy = pixelY - centerY;
    return (dx * dx + dy * dy) <= (radius * radius);
}


// Forward declarations of functions included in this code module:
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

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ANTSIMULATION, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ANTSIMULATION));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        // Start measuring time
        startTime = std::chrono::high_resolution_clock::now();

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        for (auto& Ant : Ants)
        {
            int x = Ant.second.x;
            int y = Ant.second.y;

            // Update the direction with a random turn angle
            Ant.second.direction = (Ant.second.direction + angleDist(gen)) % 360;

            // Normalize direction to keep it in the 0-360 range
            Ant.second.direction = (Ant.second.direction + 360) % 360;

            if (Ant.second.hasfood)
            {
                for (int scanX = x - scanSize; scanX < x + scanSize; ++scanX)
                {
                    for (int scanY = y - scanSize; scanY < y + scanSize; ++scanY)
                    {
                        if (scanX > 0 && scanX <= clientRect.right && scanY > 0 && scanY <= clientRect.bottom)
                        {
                            if (bluePheromone[scanX][scanY] < 0)
                            {
                                double angleToHome = atan2(scanY - y, scanX - x) * 180.0 / PI;
                                Ant.second.direction = static_cast<int>(angleToHome) % 360;
                                break;
                            }
                        }
                    }
                }

				greenPheromone[x][y] = 255;

                if (IsInsideCircle(x, y, homeX, homeY, homeRadius))
                {
                    Ant.second.hasfood = false;
                }
            }

            if (!Ant.second.hasfood)
            {
                for (int scanX = x - scanSize; scanX < x + scanSize; ++scanX)
                {
					for (int scanY = y - scanSize; scanY < y + scanSize; ++scanY)
					{
                        if (scanX > 0 && scanX <= clientRect.right && scanY > 0 && scanY <= clientRect.bottom)
						{
                            if (greenPheromone[scanX][scanY] < 0)
                            {
                                double angleToHome = atan2(scanY - y, scanX - x) * 180.0 / PI;
                                Ant.second.direction = static_cast<int>(angleToHome) % 360;
                            }
                            if (foodGrid[scanX][scanY])
                            {
                                foodGrid[scanX][scanY] = false;
                                Ant.second.hasfood = true;
								break;
                            }
						}
					}
                }
				bluePheromone[x][y] = 255;
            }

            // Calculate movement
            double radians = Ant.second.direction * PI / 180.0;
            int nextPosX = x + static_cast<int>(antDrawRadius * cos(radians));
            int nextPosY = y + static_cast<int>(antDrawRadius * sin(radians));

            // Boundary check and reflection
            if (nextPosX < 0) {
                nextPosX = 0;
                Ant.second.direction = 180 - Ant.second.direction;
            }
            else if (nextPosX > clientRect.right) {
                nextPosX = clientRect.right;
                Ant.second.direction = 180 - Ant.second.direction;
            }

            if (nextPosY < 0) {
                nextPosY = 0;
                Ant.second.direction = -Ant.second.direction;
            }
            else if (nextPosY > clientRect.bottom) {
                nextPosY = clientRect.bottom;
                Ant.second.direction = -Ant.second.direction;
            }

            // Update position if not bouncing
            Ant.second.x = nextPosX;
            Ant.second.y = nextPosY;
        }
        DrawScreen();

        for (int x = 0; x < clientRect.right; ++x) {
            for (int y = 0; y < clientRect.bottom; ++y) {
                if (greenPheromone[x][y] > 0) {
                    greenPheromone[x][y] -= 1;
                }
                if (bluePheromone[x][y] > 0) {
                    bluePheromone[x][y] -= 1;
                }
            }
        }

        // End measuring time
        endTime = std::chrono::high_resolution_clock::now();
        drawTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        fps = 1000.0 / drawTime;

        // Display draw time
        WCHAR buffer[50];
        swprintf_s(buffer, 50, L"FPS:%.2f", fps);
        SetWindowText(hWnd, buffer);
    }
    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ANTSIMULATION));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ANTSIMULATION);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      500, 300, 500, 300, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // Create ants
   for (int i = 0; i < AntCount; i++)
   {
       std::random_device rd;  // Obtain a random seed
       std::mt19937 gen(rd()); // Seed the generator
       std::uniform_int_distribution<int> angleDist(0,360);
       Ants[i] = { false, 50, 50, angleDist(gen)};
   }


   ShowWindow(hWnd, SW_SHOWMAXIMIZED);
   //ShowWindow(hWnd, SW_NORMAL);

   SetTimer(hWnd, TIMER_ID, TIMER_INTERVAL, NULL);
   
   InitDirect2D(hWnd);

   return TRUE;
}

void InitDirect2D(HWND hWnd) {
    // Create the Direct2D factory
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);

    // Get window size
    GetClientRect(hWnd, &clientRect);

    // Initialize arrays
    int gridWidth = (clientRect.right - clientRect.left) + 1;
    int gridHeight = (clientRect.bottom - clientRect.top) + 1;
    foodGrid = new bool*[gridWidth];
	greenPheromone = new int* [gridWidth];
	bluePheromone = new int* [gridWidth];
    for (int i = 0; i < gridWidth; ++i) {
        foodGrid[i] = new bool[gridHeight]();
		greenPheromone[i] = new int[gridHeight]();
		bluePheromone[i] = new int[gridHeight]();
    }

    // Create a Direct2D render target
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(
            hWnd, D2D1::SizeU(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top)),
        &pRenderTarget);

    // Create a reusable brush
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
            break;

        case WM_LBUTTONDOWN:
        {
            clickPosition.x = LOWORD(lParam);
            clickPosition.y = HIWORD(lParam);
            isClicked = true;

            // Number of food pieces to create
            int foodCount = 10;  // Adjust as needed
            int spread = 3;      // Food cluster spread

            // Generate food positions in a circular pattern
            for (int x = 0; x <= foodCount; x += spread)
            {
				for (int y = 0; y <= foodCount; y += spread)
				{
					int foodX = (clickPosition.x + x) - (foodCount / 2);
					int foodY = (clickPosition.y + y) - (foodCount / 2);
					// Ensure food positions do not go beyond the edges
					if (foodX < 0) foodX = 0;
					if (foodY < 0) foodY = 0;
					if (foodX >= clientRect.right) foodX = clientRect.right - 1;
					if (foodY >= clientRect.bottom) foodY = clientRect.bottom - 1;

					foodGrid[foodX][foodY] = true; // Update food grid
				}
            }

            DrawScreen();
        }
            break;
        case WM_DESTROY:
        {
            if (pBrush) pBrush->Release();
            if (pRenderTarget) pRenderTarget->Release();
            if (pD2DFactory) pD2DFactory->Release();

            // Deallocate food grid
            int gridWidth = clientRect.right - clientRect.left;
            for (int i = 0; i < gridWidth; ++i) {
                delete[] foodGrid[i];
            }
            delete[] foodGrid;

            PostQuitMessage(0);
        }
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    return 0;
}

void DrawScreen()
{
    PAINTSTRUCT ps;
    BeginPaint(hWnd, &ps);

    // Start drawing
    pRenderTarget->BeginDraw();
    pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black)); // Black background

    // Draw food as red circles
    for (int x = 0; x < clientRect.right; ++x) {
		for (int y = 0; y < clientRect.bottom; ++y) {
			if (foodGrid[x][y]) {
				pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
				pRenderTarget->FillRectangle(D2D1::RectF(x - (foodDrawSize/2), y - (foodDrawSize / 2), x + (foodDrawSize / 2), y + (foodDrawSize / 2)), pBrush);
			}
		}
    }

    // Draw ants as white or yellow circles
    for (const auto& Ant : Ants) {
        pBrush->SetColor(Ant.second.hasfood ? D2D1::ColorF(D2D1::ColorF::Yellow) : D2D1::ColorF(D2D1::ColorF::White));
        pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(Ant.second.x, Ant.second.y), antDrawRadius, antDrawRadius), pBrush);
    }

    pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Green));
    pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(homeX, homeY), homeRadius, homeRadius), pBrush);

    // Finish drawing
    HRESULT hr = pRenderTarget->EndDraw();
    if (FAILED(hr)) {
        // If device lost, recreate render target
        pRenderTarget->Release();
        InitDirect2D(hWnd);
    }

    EndPaint(hWnd, &ps);
}

// Message handler for about box.
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
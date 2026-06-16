#pragma once
#include <vector>
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include "CSVReader.hpp"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

class FlightWindow {
public:
    static int Show(const std::vector<FlightSample>& path, HINSTANCE hInstance = GetModuleHandleW(nullptr));

private:
    struct Vec3 {
        float x, y, z;
    };

    FlightWindow(const std::vector<FlightSample>& path);
    ~FlightWindow();

    HRESULT Initialize(HINSTANCE hInstance);
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();

    void OnPaint();
    void OnResize(UINT width, UINT height);
    void OnTimer();
    void OnLButtonDown(int x, int y);
    void OnLButtonUp();
    void OnMouseMove(int x, int y, WPARAM flags);
    void OnMouseWheel(short delta);

    void Draw3DScene(const D2D1_RECT_F& rect);
    void DrawAltitudeProfile(const D2D1_RECT_F& rect);
    void DrawTelemetryPanel(const D2D1_RECT_F& rect);

    D2D1_POINT_2F ProjectPoint(const Vec3& p, const D2D1_RECT_F& rect) const;
    Vec3 SampleToWorld(const FlightSample& s) const;
    void ComputeBounds();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hwnd = nullptr;
    std::vector<FlightSample> m_path;

    ID2D1Factory* m_d2dFactory = nullptr;
    IDWriteFactory* m_dwriteFactory = nullptr;
    IDWriteTextFormat* m_textFormat = nullptr;
    ID2D1HwndRenderTarget* m_renderTarget = nullptr;

    ID2D1SolidColorBrush* m_brushStart = nullptr;
    ID2D1SolidColorBrush* m_brushEnd = nullptr;
    ID2D1SolidColorBrush* m_brushText = nullptr;
    ID2D1SolidColorBrush* m_brushGrid = nullptr;
    ID2D1SolidColorBrush* m_brushPanel = nullptr;
    ID2D1SolidColorBrush* m_brushAccent = nullptr;
    ID2D1SolidColorBrush* m_brushGround = nullptr;
    ID2D1SolidColorBrush* m_brushStick = nullptr;
    ID2D1SolidColorBrush* m_brushDynamic = nullptr;

    double m_minLat = 0.0, m_maxLat = 0.0;
    double m_minLon = 0.0, m_maxLon = 0.0;
    double m_minAlt = 0.0, m_maxAlt = 0.0;

    float m_worldScaleX = 1.0f;
    float m_worldScaleZ = 1.0f;
    float m_altScale = 1.0f;

    float m_yaw = 0.75f;
    float m_pitch = 0.35f;
    float m_zoom = 1.0f;
    float m_cameraDistance = 900.0f;
    float m_focalLength = 900.0f;

    bool m_dragging = false;
    POINT m_lastMouse{};

    size_t m_visibleCount = 0;
    bool m_animating = true;
};
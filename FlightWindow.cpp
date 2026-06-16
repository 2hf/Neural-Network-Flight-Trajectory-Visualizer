#include <windowsx.h>
#include "FlightWindow.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

template<typename T>
static T Clamp(T v, T lo, T hi) {
    return max(lo, min(v, hi));
}

static D2D1_COLOR_F LerpColor(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b, float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return D2D1::ColorF(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}

FlightWindow::FlightWindow(const std::vector<FlightSample>& path)
    : m_path(path) {
    ComputeBounds();
    m_visibleCount = std::min<size_t>(2, m_path.size());
}

FlightWindow::~FlightWindow() {
    if (m_hwnd) KillTimer(m_hwnd, 1);
    DiscardDeviceResources();
    if (m_textFormat) m_textFormat->Release();
    if (m_dwriteFactory) m_dwriteFactory->Release();
    if (m_d2dFactory) m_d2dFactory->Release();
}

void FlightWindow::ComputeBounds() {
    if (m_path.empty()) return;

    m_minLat = m_maxLat = m_path[0].latitude;
    m_minLon = m_maxLon = m_path[0].longitude;
    m_minAlt = m_maxAlt = m_path[0].altitude;

    for (const auto& p : m_path) {
        m_minLat = min(m_minLat, p.latitude);
        m_maxLat = max(m_maxLat, p.latitude);
        m_minLon = min(m_minLon, p.longitude);
        m_maxLon = max(m_maxLon, p.longitude);
        m_minAlt = min(m_minAlt, p.altitude);
        m_maxAlt = max(m_maxAlt, p.altitude);
    }

    double altSpan = max(1.0, m_maxAlt - m_minAlt);
    m_worldScaleX = 100000.0f;
    m_worldScaleZ = 100000.0f;
    m_altScale = 0.14f;

    m_minAlt = min(0.0, m_minAlt - altSpan * 0.05);
    m_maxAlt += altSpan * 0.10;
}

HRESULT FlightWindow::CreateDeviceIndependentResources() {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2dFactory);
    if (FAILED(hr)) return hr;

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_dwriteFactory)
    );
    if (FAILED(hr)) return hr;

    hr = m_dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        18.0f,
        L"en-us",
        &m_textFormat
    );
    if (FAILED(hr)) return hr;

    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    return S_OK;
}

HRESULT FlightWindow::CreateDeviceResources() {
    HRESULT hr = S_OK;

    if (!m_renderTarget) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        hr = m_d2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_renderTarget
        );
        if (FAILED(hr)) return hr;

        hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.18f, 0.85f, 0.38f), &m_brushStart);
        if (FAILED(hr)) return hr;
        hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.25f, 0.22f), &m_brushEnd);
        if (FAILED(hr)) return hr;
        hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.88f, 0.91f, 0.94f), &m_brushText);
        if (FAILED(hr)) return hr;
        hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.24f, 0.27f, 0.32f), &m_brushGrid);
        if (FAILED(hr)) return hr;
        hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.11f, 0.12f, 0.15f, 0.95f), &m_brushPanel);
        if (FAILED(hr)) return hr;
        hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.96f, 0.74f, 0.24f), &m_brushAccent);
        if (FAILED(hr)) return hr;
        hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.14f, 0.18f, 0.14f), &m_brushGround);
        if (FAILED(hr)) return hr;
        hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.60f, 0.64f, 0.72f, 0.45f), &m_brushStick);
        if (FAILED(hr)) return hr;
        hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.20f, 0.70f, 1.00f), &m_brushDynamic);
        if (FAILED(hr)) return hr;
    }

    return hr;
}

void FlightWindow::DiscardDeviceResources() {
    if (m_brushStart) { m_brushStart->Release(); m_brushStart = nullptr; }
    if (m_brushEnd) { m_brushEnd->Release(); m_brushEnd = nullptr; }
    if (m_brushText) { m_brushText->Release(); m_brushText = nullptr; }
    if (m_brushGrid) { m_brushGrid->Release(); m_brushGrid = nullptr; }
    if (m_brushPanel) { m_brushPanel->Release(); m_brushPanel = nullptr; }
    if (m_brushAccent) { m_brushAccent->Release(); m_brushAccent = nullptr; }
    if (m_brushGround) { m_brushGround->Release(); m_brushGround = nullptr; }
    if (m_brushStick) { m_brushStick->Release(); m_brushStick = nullptr; }
    if (m_brushDynamic) { m_brushDynamic->Release(); m_brushDynamic = nullptr; }
    if (m_renderTarget) { m_renderTarget->Release(); m_renderTarget = nullptr; }
}

HRESULT FlightWindow::Initialize(HINSTANCE hInstance) {
    HRESULT hr = CreateDeviceIndependentResources();
    if (FAILED(hr)) return hr;

    const wchar_t CLASS_NAME[] = L"FlightWindowClass3DInteractive";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = FlightWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    m_hwnd = CreateWindowW(
        CLASS_NAME,
        L"Flight Viewer - Direct2D Interactive 3D",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1400, 900,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!m_hwnd) return E_FAIL;

    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);
    SetTimer(m_hwnd, 1, 33, nullptr);

    return S_OK;
}

FlightWindow::Vec3 FlightWindow::SampleToWorld(const FlightSample& s) const {
    double lonCenter = (m_minLon + m_maxLon) * 0.5;
    double latCenter = (m_minLat + m_maxLat) * 0.5;

    float x = static_cast<float>((s.longitude - lonCenter) * m_worldScaleX);
    float z = static_cast<float>((s.latitude - latCenter) * m_worldScaleZ);
    float y = static_cast<float>((s.altitude - m_minAlt) * m_altScale);

    return { x, y, z };
}

D2D1_POINT_2F FlightWindow::ProjectPoint(const Vec3& p, const D2D1_RECT_F& rect) const {
    float cosYaw = std::cos(m_yaw);
    float sinYaw = std::sin(m_yaw);
    float cosPitch = std::cos(m_pitch);
    float sinPitch = std::sin(m_pitch);

    float x1 = p.x * cosYaw - p.z * sinYaw;
    float z1 = p.x * sinYaw + p.z * cosYaw;
    float y1 = p.y;

    float y2 = y1 * cosPitch - z1 * sinPitch;
    float z2 = y1 * sinPitch + z1 * cosPitch;

    z2 += m_cameraDistance / m_zoom;
    if (z2 < 1.0f) z2 = 1.0f;

    float cx = rect.left + (rect.right - rect.left) * 0.5f;
    float cy = rect.top + (rect.bottom - rect.top) * 0.72f;

    float sx = cx + (x1 / z2) * m_focalLength * m_zoom;
    float sy = cy - (y2 / z2) * m_focalLength * m_zoom;

    return D2D1::Point2F(sx, sy);
}

void FlightWindow::OnTimer() {
    if (m_animating && m_visibleCount < m_path.size()) {
        m_visibleCount = min(m_visibleCount + 1, m_path.size());
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void FlightWindow::OnLButtonDown(int x, int y) {
    m_dragging = true;
    m_lastMouse.x = x;
    m_lastMouse.y = y;
    SetCapture(m_hwnd);
}

void FlightWindow::OnLButtonUp() {
    m_dragging = false;
    ReleaseCapture();
}

void FlightWindow::OnMouseMove(int x, int y, WPARAM flags) {
    if (!m_dragging || !(flags & MK_LBUTTON)) return;

    int dx = x - m_lastMouse.x;
    int dy = y - m_lastMouse.y;

    m_yaw += dx * 0.01f;
    m_pitch += dy * 0.006f;
    m_pitch = Clamp(m_pitch, -1.2f, 1.2f);

    m_lastMouse.x = x;
    m_lastMouse.y = y;

    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void FlightWindow::OnMouseWheel(short delta) {
    float factor = (delta > 0) ? 1.1f : 0.9f;
    m_zoom *= factor;
    m_zoom = Clamp(m_zoom, 0.35f, 4.0f);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void FlightWindow::Draw3DScene(const D2D1_RECT_F& rect) {
    m_renderTarget->FillRectangle(rect, m_brushPanel);
    m_renderTarget->DrawRectangle(rect, m_brushAccent, 1.5f);

    std::wstring title = L"3D flight scene";
    m_renderTarget->DrawTextW(
        title.c_str(), (UINT32)title.size(), m_textFormat,
        D2D1::RectF(rect.left + 10, rect.top + 8, rect.right - 10, rect.top + 34),
        m_brushText
    );

    const float gridSize = 240.0f;
    const int gridCount = 10;

    for (int i = -gridCount; i <= gridCount; ++i) {
        Vec3 a = { i * gridSize, 0.0f, -gridCount * gridSize };
        Vec3 b = { i * gridSize, 0.0f,  gridCount * gridSize };
        auto pa = ProjectPoint(a, rect);
        auto pb = ProjectPoint(b, rect);
        m_renderTarget->DrawLine(pa, pb, m_brushGrid, 1.0f);

        Vec3 c = { -gridCount * gridSize, 0.0f, i * gridSize };
        Vec3 d = { gridCount * gridSize, 0.0f, i * gridSize };
        auto pc = ProjectPoint(c, rect);
        auto pd = ProjectPoint(d, rect);
        m_renderTarget->DrawLine(pc, pd, m_brushGrid, 1.0f);
    }

    size_t visible = min(m_visibleCount, m_path.size());
    if (visible == 0) return;

    D2D1_COLOR_F lowAlt = D2D1::ColorF(0.15f, 0.75f, 1.00f, 1.0f);
    D2D1_COLOR_F highAlt = D2D1::ColorF(1.00f, 0.55f, 0.15f, 1.0f);
    double altSpan = max(1.0, m_maxAlt - m_minAlt);

    for (size_t i = 0; i < visible; ++i) {
        Vec3 world = SampleToWorld(m_path[i]);
        Vec3 ground = { world.x, 0.0f, world.z };

        auto p3 = ProjectPoint(world, rect);
        auto pg = ProjectPoint(ground, rect);
        m_renderTarget->DrawLine(pg, p3, m_brushStick, 1.0f);

        if (i > 0) {
            Vec3 prevWorld = SampleToWorld(m_path[i - 1]);
            auto prev = ProjectPoint(prevWorld, rect);

            float t = (float)((m_path[i].altitude - m_minAlt) / altSpan);
            auto col = LerpColor(lowAlt, highAlt, t);
            m_brushDynamic->SetColor(col);
            m_renderTarget->DrawLine(prev, p3, m_brushDynamic, 2.8f);
        }
    }

    auto start = ProjectPoint(SampleToWorld(m_path.front()), rect);
    auto current = ProjectPoint(SampleToWorld(m_path[visible - 1]), rect);

    m_renderTarget->FillEllipse(D2D1::Ellipse(start, 5.5f, 5.5f), m_brushStart);
    m_renderTarget->FillEllipse(D2D1::Ellipse(current, 6.5f, 6.5f), m_brushEnd);
}

void FlightWindow::DrawAltitudeProfile(const D2D1_RECT_F& rect) {
    m_renderTarget->FillRectangle(rect, m_brushPanel);
    m_renderTarget->DrawRectangle(rect, m_brushAccent, 1.5f);

    for (int i = 0; i <= 8; ++i) {
        float y = rect.top + (rect.bottom - rect.top) * i / 8.0f;
        m_renderTarget->DrawLine(D2D1::Point2F(rect.left, y), D2D1::Point2F(rect.right, y), m_brushGrid, 1.0f);
    }

    size_t visible = min(m_visibleCount, m_path.size());
    double altSpan = max(1.0, m_maxAlt - m_minAlt);

    D2D1_POINT_2F prev{};
    bool hasPrev = false;

    for (size_t i = 0; i < visible; ++i) {
        float x = rect.left + (float)i / std::max<size_t>(1, m_path.size() - 1) * (rect.right - rect.left);
        float y = rect.bottom - (float)((m_path[i].altitude - m_minAlt) / altSpan) * (rect.bottom - rect.top);
        D2D1_POINT_2F cur = D2D1::Point2F(x, y);

        if (hasPrev) {
            float t = (float)((m_path[i].altitude - m_minAlt) / altSpan);
            m_brushDynamic->SetColor(LerpColor(
                D2D1::ColorF(0.15f, 0.75f, 1.00f, 1.0f),
                D2D1::ColorF(1.00f, 0.55f, 0.15f, 1.0f),
                t
            ));
            m_renderTarget->DrawLine(prev, cur, m_brushDynamic, 2.2f);
        }

        prev = cur;
        hasPrev = true;
    }

    if (visible > 0) {
        float x0 = rect.left;
        float y0 = rect.bottom - (float)((m_path.front().altitude - m_minAlt) / altSpan) * (rect.bottom - rect.top);
        m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x0, y0), 4.5f, 4.5f), m_brushStart);

        float x1 = rect.left + (float)(visible - 1) / std::max<size_t>(1, m_path.size() - 1) * (rect.right - rect.left);
        float y1 = rect.bottom - (float)((m_path[visible - 1].altitude - m_minAlt) / altSpan) * (rect.bottom - rect.top);
        m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x1, y1), 5.5f, 5.5f), m_brushEnd);
    }

    std::wstring title = L"Altitude profile";
    m_renderTarget->DrawTextW(
        title.c_str(), (UINT32)title.size(), m_textFormat,
        D2D1::RectF(rect.left + 10, rect.top + 8, rect.right - 10, rect.top + 34),
        m_brushText
    );
}

void FlightWindow::DrawTelemetryPanel(const D2D1_RECT_F& rect) {
    m_renderTarget->FillRectangle(rect, m_brushPanel);
    m_renderTarget->DrawRectangle(rect, m_brushAccent, 1.5f);

    if (m_path.empty()) return;
    size_t visible = std::max<size_t>(1, min(m_visibleCount, m_path.size()));
    const auto& s = m_path.front();
    const auto& e = m_path[visible - 1];

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << L"Telemetry\n\n";
    ss << L"Current sample: " << visible << L" / " << m_path.size() << L"\n";
    ss << L"Animation: " << (m_animating ? L"ON" : L"OFF") << L"\n\n";
    ss << L"Lat: " << e.latitude << L"\n";
    ss << L"Lon: " << e.longitude << L"\n";

    ss << std::setprecision(2);
    ss << L"Alt: " << e.altitude << L" m\n";
    ss << L"Pitch: " << e.pitchDeg << L" deg\n";
    ss << L"Yaw: " << e.yawDeg << L" deg\n";
    ss << L"Speed: " << e.speed << L"\n\n";

    ss << L"Camera yaw: " << (m_yaw * 57.2958f) << L" deg\n";
    ss << L"Camera pitch: " << (m_pitch * 57.2958f) << L" deg\n";
    ss << L"Zoom: " << m_zoom << L"x\n\n";

    ss << L"Controls\n";
    ss << L"LMB drag: orbit camera\n";
    ss << L"Wheel: zoom\n";
    ss << L"Space: play/pause\n";
    ss << L"R: reset camera\n";

    std::wstring text = ss.str();
    m_renderTarget->DrawTextW(
        text.c_str(), (UINT32)text.size(), m_textFormat,
        D2D1::RectF(rect.left + 12, rect.top + 12, rect.right - 12, rect.bottom - 12),
        m_brushText
    );
}

void FlightWindow::OnPaint() {
    HRESULT hr = CreateDeviceResources();
    if (FAILED(hr)) return;

    PAINTSTRUCT ps;
    BeginPaint(m_hwnd, &ps);

    m_renderTarget->BeginDraw();
    m_renderTarget->Clear(D2D1::ColorF(0.06f, 0.07f, 0.09f));

    D2D1_SIZE_F size = m_renderTarget->GetSize();

    D2D1_RECT_F sceneRect = D2D1::RectF(20, 20, size.width * 0.70f, size.height * 0.68f);
    D2D1_RECT_F altitudeRect = D2D1::RectF(20, size.height * 0.72f, size.width * 0.70f, size.height - 20);
    D2D1_RECT_F panelRect = D2D1::RectF(size.width * 0.74f, 20, size.width - 20, size.height - 20);

    Draw3DScene(sceneRect);
    DrawAltitudeProfile(altitudeRect);
    DrawTelemetryPanel(panelRect);

    hr = m_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
    }

    EndPaint(m_hwnd, &ps);
}

void FlightWindow::OnResize(UINT width, UINT height) {
    if (m_renderTarget) {
        m_renderTarget->Resize(D2D1::SizeU(width, height));
    }
}

LRESULT CALLBACK FlightWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    FlightWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = static_cast<FlightWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    }
    else {
        self = reinterpret_cast<FlightWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_PAINT:
    case WM_DISPLAYCHANGE:
        self->OnPaint();
        return 0;

    case WM_SIZE:
        self->OnResize(LOWORD(lParam), HIWORD(lParam));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_TIMER:
        self->OnTimer();
        return 0;

    case WM_LBUTTONDOWN:
        self->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_LBUTTONUP:
        self->OnLButtonUp();
        return 0;

    case WM_MOUSEMOVE:
        self->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
        return 0;

    case WM_MOUSEWHEEL:
        self->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_SPACE) {
            self->m_animating = !self->m_animating;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (wParam == 'R') {
            self->m_yaw = 0.75f;
            self->m_pitch = 0.35f;
            self->m_zoom = 1.0f;
            self->m_visibleCount = std::min<size_t>(2, self->m_path.size());
            self->m_animating = true;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int FlightWindow::Show(const std::vector<FlightSample>& path, HINSTANCE hInstance) {
    FlightWindow app(path);
    if (FAILED(app.Initialize(hInstance))) {
        return -1;
    }

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
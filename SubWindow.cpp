#include "SubWindow.h"

SubWindow::SubWindow(int width, int height, QWidget *parent) : QWidget(nullptr), isDarkMode(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setFixedSize(width,height);
    setupTitleBar();
}

void SubWindow::setDarkMode(bool value) {
    isDarkMode = value; 
    if (closeBtn) closeBtn->setDarkMode(isDarkMode);
    update();
    applyThemedIcons();
}

void SubWindow::applyThemedIcons() {
    if (closeBtn)  closeBtn->setUnicodeIcon("\uE8BB", 10);
}

void SubWindow::paintEvent(QPaintEvent *event) {
    QColor BG = isDarkMode ? QColor("#1F1F1F") : QColor("#FFFFFF");
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    painter.fillRect(rect(), BG);
}

Button* SubWindow::windowButton() {
    Button *b = new Button;
    b->setSecondary(true);
    b->setIconSize(QSize(16,16));
    b->setDisplayMode(Button::IconOnly);
    b->setSize(QSize(26, 26));
    return b;
}

void SubWindow::setupTitleBar() {
    // Content Area
    _contentArea = new QWidget(this);
    _contentArea->setGeometry(0,0, width(), height());
    _contentArea->setContentsMargins(0, 0, 0, 0);
    _contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _contentArea->setAttribute(Qt::WA_TranslucentBackground);

    // Title Bar
    titleBar = new QWidget(this);
    titleBar->setGeometry(0, 3, width(), 30);
    titleBar->setContentsMargins(0, 0, 0, 0);
    titleBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    titleBar->setAttribute(Qt::WA_TranslucentBackground);

    // Close Button
    closeBtn = windowButton();
    closeBtn->setParent(titleBar);
    connect(closeBtn, &Button::clicked, this, &SubWindow::onCloseClicked);

    hwnd = reinterpret_cast<HWND>(winId());

    // Apply Icons
    applyThemedIcons();

    // Initial button position
    if (closeBtn)
        closeBtn->move(titleBar->width() - closeBtn->width() - 5, (titleBar->height() - closeBtn->height()) / 2);

    titleBar->raise();
    closeBtn->raise();
}

void SubWindow::onCloseClicked() {
    ::SendMessage(hwnd, WM_CLOSE, 0, 0);
}

bool SubWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    /*
     * MSG *msg = (MSG *)message;
     *
     * - MSG: A Windows structure that contains information about a message in the event queue.
     *   Defined in WinUser.h as:
     *       typedef struct tagMSG {
     *           HWND   hwnd;    // Window handle the message belongs to
     *           UINT   message; // The message identifier (e.g., WM_PAINT, WM_CLOSE, etc.)
     *           WPARAM wParam;  // Additional message info (unsigned integer/pointer-sized)
     *           LPARAM lParam;  // Additional message info (signed integer/pointer-sized)
     *           DWORD  time;    // Timestamp of the message
     *           POINT  pt;      // Mouse cursor position when message was posted
     *       } MSG;
     *
     * - message: This comes from Qt’s nativeEvent handler (void*),
     *            which points to the Windows MSG structure.
     * - We cast it to MSG* so we can interpret it properly.
     */
    MSG *msg = (MSG *)message;

    switch (msg->message) {
    /*
     * case WM_NCCALCSIZE:
     *
     * - WM_NCCALCSIZE: A Windows message (0x0083).
     *   Sent when the size and position of the client area must be calculated.
     *   "Non-client" area means the title bar, borders, and menu bar.
     *
     * - By setting *result = 0 and returning true,
     *   we tell Windows: "Don’t draw the default non-client area."
     *   This allows us to completely control how the border/title bar looks.
     */
    case WM_NCCALCSIZE:
        *result = 0;
        return true;

    /*
     * case WM_NCHITTEST:
     *
     * - WM_NCHITTEST: A Windows message (0x0084).
     *   Sent when the cursor moves or a mouse button is pressed.
     *   It asks: "What part of the window is under this point?"
     *
     * - The return value tells Windows what kind of area it is:
     *   HTCLIENT  = 1 (normal client area, mouse clicks handled by app).
     *   HTCAPTION = 2 (title bar area, can be used for dragging the window).
     *   HTLEFT / HTRIGHT / HTTOP / HTBOTTOM = resize borders.
     *
     * - We use this to simulate a draggable title bar inside a frameless window.
     */
    case WM_NCHITTEST: {
        /*
         * RECT winrect;
         * GetWindowRect(msg->hwnd, &winrect);
         *
         * - RECT: A Windows struct { LONG left, top, right, bottom; }
         *   Defines the bounding rectangle of a window in screen coordinates.
         *
         * - GetWindowRect: Fills the RECT with the current position and size of the window.
         */
        RECT winrect;
        GetWindowRect(msg->hwnd, &winrect);

        /*
         * Extract mouse coordinates from lParam:
         * - GET_X_LPARAM: Macro to extract low-order word as signed int (x coordinate).
         * - GET_Y_LPARAM: Macro to extract high-order word as signed int (y coordinate).
         *
         * These coordinates are in screen space.
         */
        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        /*
         * Convert screen coordinates to local (window-relative) coordinates.
         * - Subtract the window’s top-left corner (winrect.left, winrect.top).
         */
        long local_x = x - winrect.left;
        long local_y = y - winrect.top;

        /*
         * Ignore the close button region.
         * - closeBtn->geometry(): Returns a QRect in local window coordinates.
         * - If the mouse is inside this rectangle, treat it as HTCLIENT.
         *   (Means: don’t drag the window, let the button handle the click.)
         */
        if (closeBtn) {
            QRect btnRect = closeBtn->geometry();
            if (btnRect.contains(local_x, local_y)) {
                *result = HTCLIENT;
                return true;
            }
        }

        /*
         * Title bar draggable area.
         * - If the mouse is inside the custom titleBar widget,
         *   return HTCAPTION.
         * - This tells Windows: "Act like this is the system title bar."
         *   Result: user can drag the window around by holding mouse here.
         */
        if (titleBar && titleBar->geometry().contains(local_x, local_y)) {
            *result = HTCAPTION;
            return true;
        }

        /*
         * Otherwise, everything else is treated as normal client area.
         */
        *result = HTCLIENT;
        break;
    }

    default:
        break;
    }

    /*
     * Returning false means:
     * - We didn’t handle this message.
     * - Windows should continue with its default processing.
     */
    return false;
}

void SubWindow::showEvent(QShowEvent *event)
{
    hwnd = reinterpret_cast<HWND>(winId());
    /*
     * LONG style = GetWindowLong(hwnd, GWL_STYLE);
     *
     * - LONG: A 32-bit signed integer type (typedef long). In the Win32 API,
     *         window style flags are represented as a LONG bitmask.
     *
     * - GetWindowLong(hwnd, GWL_STYLE): Retrieves information about a window.
     *   hwnd: The handle to the window.
     *   GWL_STYLE: A constant (-16) that means "get the window's style flags".
     *   These flags define how the window behaves (has a caption, border, etc.).
     */
    LONG style = GetWindowLong(hwnd, GWL_STYLE);

    /*
     * style |= WS_CAPTION;
     *
     * - WS_CAPTION: A predefined constant (0x00C00000) in WinUser.h.
     *   It enables a title bar for the window. Even though we’re using
     *   a frameless Qt window, setting WS_CAPTION is useful to let Windows
     *   handle resizing and dragging logic when we simulate a custom title bar.
     *
     * - |= operator: Bitwise OR assignment.
     *   Adds the WS_CAPTION bit to the style without removing other flags.
     */
    style |= WS_CAPTION;

    /*
     * SetWindowLong(hwnd, GWL_STYLE, style);
     *
     * - SetWindowLong: Changes information about a window.
     *   hwnd: Handle to the window we want to modify.
     *   GWL_STYLE: Index specifying we’re updating style flags.
     *   style: The new LONG bitmask with WS_CAPTION included.
     *
     * - This applies the modified style back to the window.
     */
    SetWindowLong(hwnd, GWL_STYLE, style);

    /*
     * const DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
     *
     * - DWORD: A 32-bit unsigned integer type (typedef unsigned long).
     *   Frequently used in Windows APIs for flags, enums, or indices.
     *
     * - DWMWA_WINDOW_CORNER_PREFERENCE: A constant attribute index
     *   used with DwmSetWindowAttribute().
     *   It tells Windows we want to control how corners are drawn.
     *   Value 33 is defined in newer Windows SDK headers.
     */
    const DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;

    /*
     * enum DWM_WINDOW_CORNER_PREFERENCE
     *
     * - Defines the options for how Windows should render corners
     *   on a window.
     *
     *   DWMWCP_DEFAULT   (0): Let the system decide (based on theme/OS).
     *   DWMWCP_DONOTROUND(1): Keep sharp 90° corners.
     *   DWMWCP_ROUND     (2): Use large rounded corners (Windows 11 style).
     *   DWMWCP_ROUNDSMALL(3): Use slightly rounded corners.
     */
    enum DWM_WINDOW_CORNER_PREFERENCE {
        DWMWCP_DEFAULT   = 0,
        DWMWCP_DONOTROUND = 1,
        DWMWCP_ROUND      = 2,
        DWMWCP_ROUNDSMALL = 3
    };

    /*
     * DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_ROUND;
     * - DWMWCP_ROUND: Value 2, meaning we want large rounded corners.
     */
    DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_ROUND;

    /*
     * DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
     *
     * - DwmSetWindowAttribute: A Windows API function that sets a Desktop Window
     *   Manager (DWM) visual effect or property on a window.
     *
     *   hwnd: Handle to the target window.
     *   DWMWA_WINDOW_CORNER_PREFERENCE: The attribute index we’re setting (33).
     *   &pref: Pointer to the value we want to set (our enum instance).
     *   sizeof(pref): Size in bytes of the value being passed.
     */
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));

    /*
     * SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
     *              SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
     *
     * - SetWindowPos: Changes the size, position, or Z-order of a window.
     *   hwnd: Handle to the window.
     *   nullptr: No insert-after window (we’re not changing Z-order).
     *   0,0,0,0: Since we’re not moving/resizing, these values are ignored.
     *
     * - Flags:
     *   SWP_NOZORDER: Don’t change stacking order.
     *   SWP_NOMOVE: Don’t move the window.
     *   SWP_NOSIZE: Don’t resize the window.
     *   SWP_FRAMECHANGED: Recalculate the window’s non-client area
     *                     (caption, borders) based on new style flags.
     *
     * - Net effect: Forces Windows to refresh/redraw the frame without
     *   actually resizing or moving the window.
     */
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

QWidget* SubWindow::titleBarArea() const { return titleBar; }
QWidget* SubWindow::contentArea() const { return _contentArea; }

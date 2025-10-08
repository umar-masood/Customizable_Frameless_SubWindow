#pragma once
#include <QtWidgets>
#include "../Button.h"
#include <dwmapi.h>
#include <windowsx.h>
#include <windows.h>
#include <QWindow>

class SubWindow : public QWidget {
    Q_OBJECT

public:
    explicit SubWindow(int width = 250, int height = 250, QWidget *parent = nullptr);
    virtual ~SubWindow() = default;

    void setDarkMode(bool value);
    void applyThemedIcons();
    void setupTitleBar();

    QWidget* contentArea() const;
    QWidget* titleBarArea() const;

    HWND hwnd;

protected:
    void paintEvent(QPaintEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void showEvent(QShowEvent *event) override;

private:
    Button* windowButton();
    Button *closeBtn = nullptr;

    bool isDarkMode;

    QWidget *titleBar = nullptr;
    QWidget *_contentArea = nullptr;
    QVBoxLayout *entireLayout = nullptr;

private slots:
    void onCloseClicked();
};

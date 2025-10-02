# SubWindow

**SubWindow** is a reusable Qt widget that acts as a **frameless subwindow** inside another QWidget.  
It provides a custom **title bar**, **close button**, and supports **dark mode** for modern styled UIs.

---

## ✨ Features
- Frameless child window (no native OS frame).
- Custom title bar with close button.
- Dark mode and themed icon support.
- Extend content into titlebar without facing any difficulty


---

## 🚀 Usage

### Include in your project
1. Copy `SubWindow.h` and `SubWindow.cpp` into your project’s `src/` folder.
2. Include the header in your code:

```cpp
#include "SubWindow.h"
SubWindow *sub = new SubWindow(1000, 720, parentWidget);
sub->show();

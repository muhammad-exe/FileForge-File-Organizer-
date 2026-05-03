# FileForge-File-Organizer-
 FileForge — Smart File Organizer  A powerful, modern desktop application built with C++ and Qt6 that automatically organizes your cluttered folders into clean, categorized structures — with a beautiful themed UI.




🗂️ FileForge — Smart File Organizer

A powerful, modern desktop application built with C++ and Qt6 that automatically organizes your cluttered folders into clean, categorized structures — with a beautiful themed UI.


🚀 Features

⚡ One-Click Organization — Scans your selected folder and automatically sorts files into categorized subfolders
👁️ Preview Before Organizing — See exactly where every file will go before making any changes
↩️ Undo Support — Made a mistake? Restore all moved files back to their original location instantly
📊 Live Statistics — Real-time stats showing scanned, moved, and failed files with category breakdown
📝 Session Logging — Every action is logged to a timestamped session file saved on your Desktop
🎨 Multiple Themes — 6 built-in themes including Dark, Light, Batman, Hello Kitty, Ocean, and Dracula
🖼️ Custom Background — Set any image from your PC as the app background
🔒 Safe Operation — Prevents organizing the logs folder itself, avoiding accidental data loss
⚙️ Background Threading — File operations run on a separate thread so the UI never freezes
📁 Duplicate Handling — Automatically renames duplicate files to avoid overwrites


🗂️ File Categories
CategoryExtensions📄 Documents.pdf, .docx, .doc, .txt, .xlsx, .xls, .pptx, .ppt🖼️ Images.jpg, .jpeg, .png, .gif, .bmp🎵 Audio.mp3, .wav🎬 Videos.mp4, .mkv, .avi🗜️ Archives.zip, .rar💻 Code.cpp, .py, .java, .html, .css, .js📁 ProjectsFiles containing "project" in the name📦 OthersEverything else



🛠️ Built With

Language — C++17
Framework — Qt 6.11.0
IDE — Qt Creator
Build System — qmake
Platform — Windows (MinGW 64-bit)




📁 Project Structure
FileForge/
├── main.cpp              # App entry point, icon setup
├── mainwindow.h          # Main window declarations
├── mainwindow.cpp        # UI, themes, slots, logic
├── fileorganizer.h       # Rule, Logger, Organizer classes
├── organizerworker.h     # Background thread workers
├── resources.qrc         # Qt resource file
└── fp.pro                # Qt project file



⚙️ How to Build
Prerequisites

Qt 6.x installed with MinGW 64-bit kit
Qt Creator


Steps
bash# 1. Clone the repository
git clone https://github.com/YourUsername/FileForge.git

# 2. Open fp.pro in Qt Creator

# 3. Select Desktop Qt 6.x MinGW 64-bit kit

# 4. Build → Build All (Ctrl+Shift+B)

# 5. Run (Ctrl+R)



📦 How to Release
bash# After building in Release mode, run windeployqt:
windeployqt "path\to\release\FileForge.exe"

# Then zip the entire release folder and share it


"Forge order out of chaos." 🔥

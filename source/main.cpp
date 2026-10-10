#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

constexpr float kBassGain = 2.5f;
constexpr float kLowPassAlpha = 0.18f;

struct WavInfo {
    uint16_t channels = 2;
    uint32_t sampleRate = 44100;
    uint16_t bitsPerSample = 16;
    uint32_t dataOffset = 0;
    uint32_t dataSize = 0;
};

std::string g_lastInput;
std::string g_lastProcessed;
HWND g_hEdit = nullptr;
HWND g_hStatus = nullptr;

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::string quoteForCommand(const std::string& s) {
    std::string out;
    out.push_back('"');
    for (char c : s) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

std::string tempDirectory() {
    char buffer[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, buffer);
    std::string dir = std::string(buffer) + "RadioEQ";
    fs::create_directories(dir);
    return dir;
}

bool fileExists(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

bool commandSucceeded(const std::string& command) {
    int result = std::system(command.c_str());
    return result == 0;
}

std::string findYtDlp() {
    const char* candidates[] = {
        "yt-dlp.exe",
        "yt-dlp",
        "C:/Program Files/yt-dlp/yt-dlp.exe",
        "C:/Users/%USERPROFILE%/AppData/Local/Programs/Python/Python*/Scripts/yt-dlp.exe"
    };
    for (const auto& candidate : candidates) {
        if (fileExists(candidate)) return candidate;
    }
    return "yt-dlp";
}

std::string findFfmpeg() {
    const char* candidates[] = {
        "ffmpeg.exe",
        "ffmpeg",
        "C:/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files/ffmpeg/bin/ffmpeg.exe"
    };
    for (const auto& candidate : candidates) {
        if (fileExists(candidate)) return candidate;
    }
    return "ffmpeg";
}

std::string normalizeToWavPath(const std::string& urlOrPath) {
    std::string clean = trim(urlOrPath);
    if (clean.empty()) return "";
    if (clean.find("http://") == 0 || clean.find("https://") == 0) {
        std::string dir = tempDirectory();
        return dir + "\\radioeq_source.wav";
    }
    return clean;
}

bool downloadYoutubeToWav(const std::string& url, const std::string& outputFile) {
    std::string yt = findYtDlp();
    std::string ff = findFfmpeg();

    std::string cmd =
        quoteForCommand(yt) +
        " --no-warnings --restrict-filenames --no-playlist --output " +
        quoteForCommand(outputFile) +
        " --extract-audio --audio-format wav --audio-quality 0 " +
        quoteForCommand(url);

    if (!commandSucceeded(cmd)) {
        return false;
    }

    if (!fileExists(outputFile)) {
        const std::string dir = fs::path(outputFile).parent_path().string();
        std::string found;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                const auto ext = entry.path().extension().string();
                if (ext == ".wav" || ext == ".WAV") {
                    found = entry.path().string();
                    break;
                }
            }
        }
        if (found.empty()) {
            return false;
        }
        if (found != outputFile) {
            fs::copy_file(found, outputFile, fs::copy_options::overwrite_existing);
        }
    }

    return fileExists(outputFile);
}

bool convertToWav(const std::string& inputFile, const std::string& outputFile) {
    const std::string ff = findFfmpeg();
    std::string cmd =
        quoteForCommand(ff) +
        " -y -i " + quoteForCommand(inputFile) +
        " -vn -acodec pcm_s16le -ar 44100 -ac 2 " +
        quoteForCommand(outputFile);
    return commandSucceeded(cmd) && fileExists(outputFile);
}

bool readWavInfo(const std::string& path, WavInfo& info) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    char riff[4] = {0};
    char wave[4] = {0};
    char fmt[4] = {0};
    char data[4] = {0};

    file.read(riff, 4);
    file.read(reinterpret_cast<char*>(&info.dataSize), 4);
    file.read(wave, 4);
    file.read(fmt, 4);

    uint32_t fmtSize = 0;
    file.read(reinterpret_cast<char*>(&fmtSize), 4);

    uint16_t audioFormat = 0;
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint32_t byteRate = 0;
    uint16_t blockAlign = 0;
    uint16_t bitsPerSample = 0;

    file.read(reinterpret_cast<char*>(&audioFormat), 2);
    file.read(reinterpret_cast<char*>(&numChannels), 2);
    file.read(reinterpret_cast<char*>(&sampleRate), 4);
    file.read(reinterpret_cast<char*>(&byteRate), 4);
    file.read(reinterpret_cast<char*>(&blockAlign), 2);
    file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

    if (audioFormat != 1) {
        // Non-PCM output will be converted by ffmpeg before this stage.
        return false;
    }

    info.channels = numChannels;
    info.sampleRate = sampleRate;
    info.bitsPerSample = bitsPerSample;

    uint32_t pos = 16 + fmtSize - 16;
    file.seekg(pos, std::ios::beg);
    while (file.peek() != EOF) {
        char chunkId[5] = {0};
        file.read(chunkId, 4);
        if (file.fail()) break;
        uint32_t chunkSize = 0;
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (std::string(chunkId) == "data") {
            info.dataOffset = static_cast<uint32_t>(file.tellg()) - 8;
            info.dataSize = chunkSize;
            break;
        }
        file.seekg(chunkSize, std::ios::cur);
    }

    return info.bitsPerSample > 0 && info.dataSize > 0;
}

void applyBassBoostToWav(const std::string& inputFile, const std::string& outputFile) {
    WavInfo info;
    if (!readWavInfo(inputFile, info)) {
        fs::copy_file(inputFile, outputFile, fs::copy_options::overwrite_existing);
        return;
    }

    std::ifstream in(inputFile, std::ios::binary);
    std::ofstream out(outputFile, std::ios::binary);

    if (!in || !out) return;

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), {});
    const size_t dataOffset = info.dataOffset;
    const size_t dataSize = info.dataSize;

    if (dataOffset + dataSize > buffer.size()) return;

    std::vector<uint8_t> outputBuffer = buffer;
    const size_t bytesPerSample = info.bitsPerSample / 8;
    const size_t sampleCount = dataSize / bytesPerSample;

    float lowL = 0.0f;
    float lowR = 0.0f;

    for (size_t i = 0; i < sampleCount; ++i) {
        const size_t index = dataOffset + i * bytesPerSample;
        if (index + bytesPerSample > outputBuffer.size()) break;

        int16_t sample = static_cast<int16_t>(
            static_cast<uint16_t>(outputBuffer[index]) |
            (static_cast<uint16_t>(outputBuffer[index + 1]) << 8));

        float x = sample / 32768.0f;
        float low = lowL + kLowPassAlpha * (x - lowL);
        lowL = low;
        float boosted = x + low * kBassGain;
        boosted = std::clamp(boosted, -1.0f, 1.0f);
        int16_t value = static_cast<int16_t>(boosted * 32767.0f);

        outputBuffer[index] = static_cast<uint8_t>(value & 0xFF);
        outputBuffer[index + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);

        if (info.channels == 2 && (i + 1) * bytesPerSample + index + 1 < outputBuffer.size()) {
            size_t nextIndex = index + 2;
            int16_t rightSample = static_cast<int16_t>(
                static_cast<uint16_t>(outputBuffer[nextIndex]) |
                (static_cast<uint16_t>(outputBuffer[nextIndex + 1]) << 8));
            float y = rightSample / 32768.0f;
            float lowRight = lowR + kLowPassAlpha * (y - lowR);
            lowR = lowRight;
            float boostedRight = y + lowRight * kBassGain;
            boostedRight = std::clamp(boostedRight, -1.0f, 1.0f);
            int16_t rightValue = static_cast<int16_t>(boostedRight * 32767.0f);
            outputBuffer[nextIndex] = static_cast<uint8_t>(rightValue & 0xFF);
            outputBuffer[nextIndex + 1] = static_cast<uint8_t>((rightValue >> 8) & 0xFF);
        }
    }

    out.write(reinterpret_cast<const char*>(outputBuffer.data()), static_cast<std::streamsize>(outputBuffer.size()));
}

bool prepareAudioFile(const std::string& input, std::string& resultPath) {
    std::string clean = trim(input);
    if (clean.empty()) return false;

    std::string tmpDir = tempDirectory();
    std::string sourcePath = tmpDir + "\\radioeq_input.wav";
    std::string outputPath = tmpDir + "\\radioeq_bass.wav";

    if (clean.find("http://") == 0 || clean.find("https://") == 0) {
        if (!downloadYoutubeToWav(clean, sourcePath)) {
            return false;
        }
    } else {
        if (!fs::exists(clean)) return false;
        const std::string ext = fs::path(clean).extension().string();
        if (ext == ".wav" || ext == ".WAV") {
            sourcePath = clean;
        } else {
            if (!convertToWav(clean, sourcePath)) return false;
        }
    }

    applyBassBoostToWav(sourcePath, outputPath);
    if (!fileExists(outputPath)) {
        outputPath = sourcePath;
    }

    resultPath = outputPath;
    g_lastProcessed = outputPath;
    return fileExists(resultPath);
}

void setStatusText(const std::string& text) {
    if (g_hStatus) {
        SetWindowTextA(g_hStatus, text.c_str());
    }
}

void onPlayClicked() {
    const int length = GetWindowTextLengthA(g_hEdit);
    if (length <= 0) {
        setStatusText("Insert a URL or a local file path.");
        return;
    }

    char buffer[4096] = {};
    GetWindowTextA(g_hEdit, buffer, sizeof(buffer));
    g_lastInput = buffer;
    std::string path;
    if (!prepareAudioFile(g_lastInput, path)) {
        setStatusText("Could not prepare the audio. Check yt-dlp/ffmpeg installation or the file path.");
        return;
    }

    PlaySoundA(path.c_str(), nullptr, SND_ASYNC | SND_NODEFAULT);
    std::string status = "Playing: " + fs::path(path).filename().string();
    setStatusText(status);
}

void onBrowseClicked() {
    OPENFILENAMEA ofn = {};
    char buffer[MAX_PATH] = {};
    buffer[0] = '\0';

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = "Audio Files (*.mp3;*.wav;*.flac;*.m4a;*.aac;*.ogg)\0*.mp3;*.wav;*.flac;*.m4a;*.aac;*.ogg\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        SetWindowTextA(g_hEdit, buffer);
        setStatusText("File selected.");
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            RECT rc;
            GetClientRect(hwnd, &rc);

            CreateWindowExA(0, "STATIC", "Audio URL / file:", WS_CHILD | WS_VISIBLE,
                            12, 12, 120, 24, hwnd, nullptr, ((LPCREATESTRUCT)lParam)->hInstance, nullptr);

            g_hEdit = CreateWindowExA(0, "EDIT", "",
                                      WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
                                      130, 10, 340, 24, hwnd, nullptr, ((LPCREATESTRUCT)lParam)->hInstance, nullptr);

            g_hStatus = CreateWindowExA(0, "STATIC", "Ready.", WS_CHILD | WS_VISIBLE,
                                       12, 60, 460, 24, hwnd, nullptr, ((LPCREATESTRUCT)lParam)->hInstance, nullptr);

            CreateWindowExA(0, "BUTTON", "Play", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            12, 100, 100, 32, hwnd, (HMENU)1001, ((LPCREATESTRUCT)lParam)->hInstance, nullptr);

            CreateWindowExA(0, "BUTTON", "Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            130, 100, 100, 32, hwnd, (HMENU)1002, ((LPCREATESTRUCT)lParam)->hInstance, nullptr);

            CreateWindowExA(0, "BUTTON", "Stop", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            248, 100, 100, 32, hwnd, (HMENU)1003, ((LPCREATESTRUCT)lParam)->hInstance, nullptr);
            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == 1001) {
                onPlayClicked();
            } else if (LOWORD(wParam) == 1002) {
                onBrowseClicked();
            } else if (LOWORD(wParam) == 1003) {
                PlaySoundA(nullptr, nullptr, SND_PURGE);
                setStatusText("Playback stopped.");
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    const char kClassName[] = "RadioEQWindowsNative";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(nullptr, "Failed to register the window class.", "Radio EQ", MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowExA(
        0,
        kClassName,
        "Radio EQ Windows Native",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        560,
        220,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hwnd) {
        MessageBoxA(nullptr, "Failed to create the main window.", "Radio EQ", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return static_cast<int>(msg.wParam);
}

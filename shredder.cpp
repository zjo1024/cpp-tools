#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cstring>

using namespace std;

// 控制台颜色
void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

class FileShredder {
private:
    static void generateRandomData(BYTE* buffer, size_t size) {
        for (size_t i = 0; i < size; i++) {
            buffer[i] = static_cast<BYTE>(rand() % 256);
        }
    }
    
    static void showProgress(int pass, int totalPasses, LONGLONG current, LONGLONG total) {
        int percent = static_cast<int>((current * 100) / total);
        setColor(11); // CYAN
        printf("\r[第%d/%d遍] 进度: %d%% [", pass, totalPasses, percent);
        int barWidth = 30;
        int pos = (barWidth * percent) / 100;
        for (int i = 0; i < barWidth; i++) {
            if (i < pos) printf("=");
            else if (i == pos) printf(">");
            else printf(" ");
        }
        printf("] %I64dKB/%I64dKB", current / 1024, total / 1024);
        setColor(7); // WHITE
    }

public:
    static bool shredFile(const wstring& filename, int passes = 3) {
        setColor(14); // YELLOW
        wprintf(L"正在打开文件: %s\n", filename.c_str());
        
        HANDLE hFile = CreateFileW(
            filename.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
            NULL
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            setColor(12); // RED
            wprintf(L"错误: 无法打开文件 %s (错误代码: %d)\n", filename.c_str(), GetLastError());
            setColor(7);
            return false;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize)) {
            setColor(12);
            wprintf(L"错误: 无法获取文件大小\n");
            setColor(7);
            CloseHandle(hFile);
            return false;
        }

        setColor(10); // GREEN
        wprintf(L"文件大小: %I64d 字节 (%I64d KB)\n", fileSize.QuadPart, fileSize.QuadPart / 1024);
        setColor(7);

        srand(static_cast<unsigned int>(time(NULL)));
        bool success = true;

        for (int pass = 0; pass < passes; pass++) {
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

            const size_t BUFFER_SIZE = 4096;
            BYTE buffer[BUFFER_SIZE];
            DWORD bytesWritten;
            LARGE_INTEGER totalWritten = {0};

            while (totalWritten.QuadPart < fileSize.QuadPart) {
                size_t writeSize = BUFFER_SIZE;
                if (totalWritten.QuadPart + BUFFER_SIZE > fileSize.QuadPart) {
                    writeSize = static_cast<size_t>(fileSize.QuadPart - totalWritten.QuadPart);
                }

                // 不同的覆盖模式
                switch(pass % 4) {
                    case 0: memset(buffer, 0x00, writeSize); break;
                    case 1: memset(buffer, 0xFF, writeSize); break;
                    case 2: memset(buffer, 0xAA, writeSize); break;
                    case 3: generateRandomData(buffer, writeSize); break;
                }

                if (!WriteFile(hFile, buffer, static_cast<DWORD>(writeSize), &bytesWritten, NULL)) {
                    setColor(12);
                    wprintf(L"\n写入错误!\n");
                    setColor(7);
                    success = false;
                    break;
                }

                totalWritten.QuadPart += bytesWritten;
                showProgress(pass + 1, passes, totalWritten.QuadPart, fileSize.QuadPart);
            }

            FlushFileBuffers(hFile);
            printf("\n");
            
            if (!success) break;
        }

        CloseHandle(hFile);
        return success;
    }

    static bool deleteFile(const wstring& filename) {
        wstring newName = filename + L".deleted";
        if (!MoveFileW(filename.c_str(), newName.c_str())) {
            return false;
        }
        return DeleteFileW(newName.c_str()) != 0;
    }
    
    static bool renameBeforeDelete(const wstring& filename) {
        wstring currentName = filename;
        wstring path = filename.substr(0, filename.find_last_of(L"\\") + 1);
        
        const wchar_t* tempNames[] = {
            L"deleted.tmp",
            L"removed.tmp", 
            L"gone.tmp",
            L"secure.tmp"
        };
        
        for (int i = 0; i < 4; i++) {
            wstring newName = path + tempNames[i];
            if (MoveFileW(currentName.c_str(), newName.c_str())) {
                currentName = newName;
                Sleep(10);
            }
        }
        
        return DeleteFileW(currentName.c_str()) != 0;
    }

    static bool shredAndDelete(const wstring& filename, int passes = 3, bool multipleRename = false) {
        wprintf(L"\n");
        setColor(11);
        wprintf(L"========================================\n");
        wprintf(L"      文件粉碎工具 v1.0                \n");
        wprintf(L"========================================\n");
        setColor(7);
        wprintf(L"\n");
        
        setColor(14);
        wprintf(L"目标文件: %s\n", filename.c_str());
        wprintf(L"覆盖次数: %d 遍\n", passes);
        wprintf(L"重命名: %s\n", multipleRename ? L"多次" : L"单次");
        wprintf(L"\n");
        setColor(7);
        
        wprintf(L"正在粉碎文件...\n");
        wprintf(L"----------------------------------------\n");
        
        if (!shredFile(filename, passes)) {
            setColor(12);
            wprintf(L"文件覆盖失败!\n");
            setColor(7);
            return false;
        }

        setColor(10);
        wprintf(L"覆盖完成！\n");
        setColor(7);
        wprintf(L"正在删除文件...\n");

        bool deleted;
        if (multipleRename) {
            deleted = renameBeforeDelete(filename);
        } else {
            deleted = deleteFile(filename);
        }

        if (deleted) {
            setColor(10);
            wprintf(L"文件已成功删除！\n");
            setColor(7);
        } else {
            setColor(12);
            wprintf(L"文件删除失败! (错误代码: %d)\n", GetLastError());
            setColor(7);
        }
        
        return deleted;
    }
};

void printUsage() {
    setColor(14);
    printf("使用方法:\n");
    printf("  shred.exe <文件路径> [选项]\n");
    printf("  shred.exe /f <文件夹路径> [选项]\n");
    printf("\n");
    printf("选项:\n");
    printf("  -p <次数>    覆盖次数 (默认: 3)\n");
    printf("  -m           多次重命名\n");
    printf("  -h           显示帮助\n");
    printf("\n");
    printf("示例:\n");
    setColor(10);
    printf("  shred.exe C:\\secret.txt\n");
    printf("  shred.exe D:\\data\\file.doc -p 7\n");
    printf("  shred.exe E:\\private\\video.mp4 -p 5 -m\n");
    printf("  shred.exe /f C:\\sensitive_folder -p 3\n");
    setColor(7);
}

bool DeleteDirectory(const wstring& dirPath) {
    wstring searchPath = dirPath + L"\\*.*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
                wstring fullPath = dirPath + L"\\" + findData.cFileName;
                
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    DeleteDirectory(fullPath);
                } else {
                    DeleteFileW(fullPath.c_str());
                }
            }
        } while (FindNextFileW(hFind, &findData) != 0);
        FindClose(hFind);
    }
    
    return RemoveDirectoryW(dirPath.c_str()) != 0;
}

bool shredFolder(const wstring& folderPath, int passes = 3, bool multipleRename = false) {
    wstring searchPath = folderPath + L"\\*.*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    int fileCount = 0;
    int successCount = 0;
    
    if (hFind == INVALID_HANDLE_VALUE) {
        setColor(12);
        wprintf(L"无法打开文件夹: %s\n", folderPath.c_str());
        setColor(7);
        return false;
    }
    
    setColor(14);
    wprintf(L"正在扫描文件夹: %s\n", folderPath.c_str());
    setColor(7);
    
    do {
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
            wstring fullPath = folderPath + L"\\" + findData.cFileName;
            
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                wprintf(L"\n进入子文件夹: %s\n", findData.cFileName);
                shredFolder(fullPath, passes, multipleRename);
            } else {
                fileCount++;
                wprintf(L"\n处理文件 [%d]: %s\n", fileCount, findData.cFileName);
                
                if (FileShredder::shredAndDelete(fullPath, passes, multipleRename)) {
                    successCount++;
                } else {
                    setColor(12);
                    wprintf(L"处理失败!\n");
                    setColor(7);
                }
            }
        }
    } while (FindNextFileW(hFind, &findData) != 0);
    
    FindClose(hFind);
    
    setColor(11);
    wprintf(L"\n----------------------------------------\n");
    wprintf(L"文件夹处理完成!\n");
    wprintf(L"总计文件: %d\n", fileCount);
    wprintf(L"成功处理: %d\n", successCount);
    wprintf(L"失败文件: %d\n", fileCount - successCount);
    setColor(7);
    
    if (successCount == fileCount && fileCount > 0) {
        if (DeleteDirectory(folderPath)) {
            setColor(10);
            wprintf(L"文件夹已删除\n");
            setColor(7);
        }
    }
    
    return successCount == fileCount;
}

int wmain(int argc, wchar_t* argv[]) {
    // 设置控制台标题
    SetConsoleTitleW(L"文件粉碎工具");
    
    if (argc < 2) {
        printUsage();
        system("pause");
        return 1;
    }
    
    // 解析参数
    wstring target = argv[1];
    int passes = 3;
    bool multipleRename = false;
    bool shredFolderMode = false;
    
    // 检查是否为文件夹模式
    if (target == L"/f" || target == L"/F") {
        if (argc < 3) {
            setColor(12);
            wprintf(L"错误: 缺少文件夹路径!\n");
            setColor(7);
            printUsage();
            system("pause");
            return 1;
        }
        shredFolderMode = true;
        target = argv[2];
    }
    
    // 解析其他选项
    for (int i = shredFolderMode ? 3 : 2; i < argc; i++) {
        wstring arg = argv[i];
        if (arg == L"-p" || arg == L"/p") {
            if (i + 1 < argc) {
                passes = _wtoi(argv[++i]);
                if (passes < 1) passes = 1;
                if (passes > 35) passes = 35;
            }
        } else if (arg == L"-m" || arg == L"/m") {
            multipleRename = true;
        } else if (arg == L"-h" || arg == L"/h" || arg == L"/?") {
            printUsage();
            system("pause");
            return 0;
        }
    }
    
    // 检查文件/文件夹是否存在
    DWORD attrib = GetFileAttributesW(target.c_str());
    if (attrib == INVALID_FILE_ATTRIBUTES) {
        setColor(12);
        wprintf(L"错误: 路径不存在 - %s\n", target.c_str());
        setColor(7);
        system("pause");
        return 1;
    }
    
    bool result;
    if (shredFolderMode || (attrib & FILE_ATTRIBUTE_DIRECTORY)) {
        wprintf(L"目标: 文件夹模式\n");
        result = shredFolder(target, passes, multipleRename);
    } else {
        wprintf(L"目标: 单文件模式\n");
        result = FileShredder::shredAndDelete(target, passes, multipleRename);
    }
    
    setColor(11);
    wprintf(L"\n----------------------------------------\n");
    if (result) {
        setColor(10);
        wprintf(L"操作成功完成!\n");
    } else {
        setColor(12);
        wprintf(L"操作完成，但有错误发生!\n");
    }
    setColor(7);
    
    printf("\n按任意键退出...");
    system("pause > nul");
    return result ? 0 : 1;
}

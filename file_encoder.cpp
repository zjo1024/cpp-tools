#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define SEPARATOR '\\'
#define MKDIR(dir) _mkdir(dir)
#else
#include <sys/types.h>
#include <unistd.h>
#define SEPARATOR '/'
#define MKDIR(dir) mkdir(dir, 0755)
#endif

using namespace std;

// 工具函数：从完整路径中提取目录部分
string get_directory(const string& full_path) {
    size_t pos = full_path.find_last_of(SEPARATOR);
    if (pos != string::npos) {
        return full_path.substr(0, pos + 1);
    }
    return "";
}

// 工具函数：从完整路径中提取文件名（不含目录）
string get_filename(const string& full_path) {
    size_t pos = full_path.find_last_of(SEPARATOR);
    if (pos != string::npos) {
        return full_path.substr(pos + 1);
    }
    return full_path;
}

// 工具函数：从文件名中提取基本名称（不含扩展名）
string get_basename(const string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != string::npos) {
        return filename.substr(0, dot_pos);
    }
    return filename;
}

// 工具函数：从文件名中提取扩展名
string get_extension(const string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != string::npos) {
        return filename.substr(dot_pos);
    }
    return "";
}

// 工具函数：确保目录存在
bool ensure_directory_exists(const string& dir_path) {
    if (dir_path.empty()) return true;
    
    struct stat info;
    if (stat(dir_path.c_str(), &info) != 0) {
        return MKDIR(dir_path.c_str()) == 0;
    } else if (info.st_mode & S_IFDIR) {
        return true;
    }
    return false;
}

// 根据输入文件名和编码类型生成输出文件名
string generate_output_filename(const string& input_filename, const string& encoding) {
    string dir = get_directory(input_filename);
    string filename = get_filename(input_filename);
    string basename = get_basename(filename);
    
    // 根据编码类型确定后缀和扩展名
    string suffix;
    string extension;
    
    if (encoding == "utf-8") {
        suffix = "_utf8";
    } else if (encoding == "utf-8-bin") {  // 新增：二进制UTF-8
        suffix = "_utf8_bin";
    } else if (encoding == "ascii-2") {
        suffix = "_binary";
    } else if (encoding == "ascii-16") {
        suffix = "_hex";
    } else if (encoding == "base64") {
        suffix = "_base64";
    } else if (encoding == "url") {
        suffix = "_url";
    } else {
        suffix = "_encoded";
    }
    extension = ".txt";
    
    return dir + basename + suffix + extension;
}

// Base64编解码相关函数
const string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

string base64_encode(const string& input) {
    string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    size_t in_len = input.size();
    const char* bytes_to_encode = input.c_str();
    
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for(i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        
        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];
        
        while(i++ < 3)
            ret += '=';
    }
    
    return ret;
}

// URL编码函数
string url_encode(const string& value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;
    
    for (size_t idx = 0; idx < value.size(); ++idx) {
        char c = value[idx];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        escaped << '%' << setw(2) << int((unsigned char)c);
    }
    
    return escaped.str();
}

// UTF-8转换函数 - 显示UTF-8字节序列（十六进制）
string to_utf8_hex(const string& input) {
    ostringstream oss;
    for (size_t i = 0; i < input.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        oss << hex << setw(2) << setfill('0') 
            << static_cast<int>(c);
        
        if (i < input.length() - 1) {
            oss << " ";
        }
    }
    return oss.str();
}

// UTF-8转换函数 - 显示UTF-8字节序列（二进制）新增
string to_utf8_binary(const string& input) {
    string result;
    for (size_t i = 0; i < input.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        
        // 分析UTF-8字符的字节数
        int bytes = 1;
        if ((c & 0xE0) == 0xC0) {
            bytes = 2;
        } else if ((c & 0xF0) == 0xE0) {
            bytes = 3;
        } else if ((c & 0xF8) == 0xF0) {
            bytes = 4;
        }
        
        // 输出当前字符的所有字节（二进制格式）
        for (int j = 0; j < bytes && (i + j) < input.length(); ++j) {
            unsigned char byte = static_cast<unsigned char>(input[i + j]);
            bitset<8> bits(byte);
            result += bits.to_string();
            
            // 在字节之间添加空格
            if (j < bytes - 1) {
                result += " ";
            }
        }
        
        // 在不同字符之间添加分隔符
        if (i + bytes - 1 < input.length() - 1) {
            result += "  ";  // 两个空格作为字符分隔
        }
        
        // 跳过已处理的字节
        if (bytes > 1) {
            i += (bytes - 1);
        }
    }
    return result;
}

// ASCII-二进制转换
string to_ascii_binary(const string& input) {
    string result;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        bitset<8> bits(static_cast<unsigned char>(c));
        result += bits.to_string();
        if (i != input.size() - 1) {
            result += " ";
        }
    }
    return result;
}

// ASCII-十六进制转换
string to_ascii_hex(const string& input) {
    ostringstream oss;
    for (size_t i = 0; i < input.length(); ++i) {
        oss << hex << setw(2) << setfill('0') 
            << static_cast<int>(static_cast<unsigned char>(input[i]));
        if (i != input.length() - 1) {
            oss << " ";
        }
    }
    return oss.str();
}

// 显示帮助信息
void show_help() {
    cout << endl << "文件编码转换工具" << endl;
    cout << "==================" << endl;
    cout << "使用方法：" << endl << endl;
    cout << "file_encoder [输入文件名] [编码方式]" << endl;
    cout << endl;
    cout << "参数说明：" << endl << endl;
    cout << "  输入文件名 - 要转换的源文件" << endl;
    cout << "  编码方式   - 支持的编码方式：" << endl;
    cout << "       utf-8        UTF-8字节序列十六进制" << endl;
    cout << "       utf-8-bin    UTF-8字节序列二进制" << endl;
    cout << "       ascii-2      ASCII码二进制" << endl;
    cout << "       ascii-16     ASCII码十六进制" << endl;
    cout << "       base64       Base64编码" << endl;
    cout << "       url          URL编码" << endl;
    cout << endl;
    cout << "示例：" << endl;
    cout << "  file_encoder document.txt utf-8" << endl;
    cout << "  file_encoder document.txt utf-8-bin" << endl;
    cout << "  file_encoder image.jpg base64" << endl;
    cout << "  file_encoder data.bin ascii-2" << endl;
    cout << endl;
    cout << "输出文件将自动保存在原文件目录，" << endl;
    cout << "文件名格式为：原文件名_编码类型.扩展名" << endl;
    cout << endl;
    cout << "要显示此帮助信息，请输入：file_encoder /?" << endl;
}

// 读取文件内容
string read_file(const string& filename) {
    ifstream file(filename.c_str(), ios::binary);
    if (!file) {
        throw runtime_error("无法打开输入文件: " + filename);
    }
    
    file.seekg(0, ios::end);
    streampos file_size = file.tellg();
    file.seekg(0, ios::beg);
    
    if (file_size <= 0) {
        file.close();
        return string();
    }
    
    vector<char> buffer(file_size);
    file.read(&buffer[0], file_size);
    file.close();
    
    return string(buffer.begin(), buffer.end());
}

// 写入文件内容
void write_file(const string& filename, const string& content) {
    string dir = get_directory(filename);
    if (!ensure_directory_exists(dir)) {
        throw runtime_error("无法创建目录: " + dir);
    }
    
    ofstream file(filename.c_str(), ios::binary);
    if (!file) {
        throw runtime_error("无法创建输出文件: " + filename);
    }
    
    file.write(content.c_str(), content.size());
    file.close();
}

int main(int argc, char* argv[]) {
    // 如果参数是/?，显示帮助信息
    if (argc == 2 && string(argv[1]) == "/?") {
        show_help();
        return 0;
    }
    
    // 检查参数数量
    if (argc != 3) {
        cerr << endl << "错误：参数数量不正确！" << endl;
        cerr << "使用方法：file_encoder [输入文件名] [编码方式]" << endl;
        cerr << "使用 /? 查看详细帮助信息" << endl;
        return 1;
    }
    
    string input_filename = argv[1];
    string encoding = argv[2];
    
    try {
        // 生成输出文件名
        string output_filename = generate_output_filename(input_filename, encoding);
        
        // 读取输入文件
        string input_content = read_file(input_filename);
        
        string result;
        
        // 根据编码方式转换
        if (encoding == "utf-8") {
            result = to_utf8_hex(input_content);
        }
        else if (encoding == "utf-8-bin") {  // 新增：二进制UTF-8
            result = to_utf8_binary(input_content);
        }
        else if (encoding == "ascii-2") {
            result = to_ascii_binary(input_content);
        }
        else if (encoding == "ascii-16") {
            result = to_ascii_hex(input_content);
        }
        else if (encoding == "base64") {
            result = base64_encode(input_content);
        }
        else if (encoding == "url") {
            result = url_encode(input_content);
        }
        else {
            cerr << "错误：不支持的编码方式: " << encoding << endl;
            cerr << "支持的编码方式: utf-8, utf-8-bin, ascii-2, ascii-16, base64, url" << endl;
            return 1;
        }
        
        // 写入输出文件
        write_file(output_filename, result);
        
        cout << "转换完成！" << endl;
        cout << "输入文件: " << input_filename << endl;
        cout << "输出文件: " << output_filename << endl;
        cout << "编码方式: " << encoding << endl;
        cout << "输出大小: " << result.size() << " 字节" << endl;
        
    } catch (const exception& e) {
        cerr << "错误: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "未知错误发生！" << endl;
        return 1;
    }
    
    return 0;
}

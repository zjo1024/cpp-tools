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

// 从文件名检测编码类型
string detect_encoding_from_filename(const string& filename) {
    string basename = get_basename(filename);
    
    // 检查是否有编码后缀
    if (basename.find("_utf8_bin") != string::npos) {
        return "utf-8-bin";
    } else if (basename.find("_utf8") != string::npos) {
        return "utf-8";
    } else if (basename.find("_binary") != string::npos) {
        return "ascii-2";
    } else if (basename.find("_hex") != string::npos) {
        return "ascii-16";
    } else if (basename.find("_base64") != string::npos) {
        return "base64";
    } else if (basename.find("_url") != string::npos) {
        return "url";
    }
    
    return ""; // 未检测到编码后缀
}

// 根据输入文件名和编码类型生成输出文件名
string generate_output_filename(const string& input_filename, const string& encoding) {
    string dir = get_directory(input_filename);
    string filename = get_filename(input_filename);
    string basename = get_basename(filename);
    
    // 移除编码后缀
    if (encoding == "utf-8-bin") {
        size_t pos = basename.find("_utf8_bin");
        if (pos != string::npos) {
            basename = basename.substr(0, pos);
        }
    } else if (encoding == "utf-8") {
        size_t pos = basename.find("_utf8");
        if (pos != string::npos) {
            basename = basename.substr(0, pos);
        }
    } else if (encoding == "ascii-2") {
        size_t pos = basename.find("_binary");
        if (pos != string::npos) {
            basename = basename.substr(0, pos);
        }
    } else if (encoding == "ascii-16") {
        size_t pos = basename.find("_hex");
        if (pos != string::npos) {
            basename = basename.substr(0, pos);
        }
    } else if (encoding == "base64") {
        size_t pos = basename.find("_base64");
        if (pos != string::npos) {
            basename = basename.substr(0, pos);
        }
    } else if (encoding == "url") {
        size_t pos = basename.find("_url");
        if (pos != string::npos) {
            basename = basename.substr(0, pos);
        }
    }
    
    // 获取原文件的扩展名（如果存在）
    string extension = get_extension(filename);
    
    // 如果有原扩展名，使用原扩展名
    // 如果没有扩展名，添加一个默认扩展名
    if (extension.empty()) {
        return dir + basename + "_decoded.txt";
    } else {
        return dir + basename + extension;
    }
}

// Base64编解码相关函数
const string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

// Base64解码函数
string base64_decode(const string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    string ret;
    
    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                ret += char_array_3[i];
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;
        
        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        
        for (j = 0; j < i - 1; j++)
            ret += char_array_3[j];
    }
    
    return ret;
}

// URL解码函数
string url_decode(const string& value) {
    ostringstream decoded;
    
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%') {
            if (i + 2 < value.length()) {
                string hex = value.substr(i + 1, 2);
                char chr = (char)strtol(hex.c_str(), NULL, 16);
                decoded << chr;
                i += 2;
            }
        } else if (value[i] == '+') {
            decoded << ' ';
        } else {
            decoded << value[i];
        }
    }
    
    return decoded.str();
}

// 从十六进制字符串解码
string from_hex_string(const string& hex_str) {
    string result;
    stringstream ss(hex_str);
    string hex_byte;
    
    while (ss >> hex_byte) {
        // 移除可能存在的空格或换行符
        hex_byte.erase(remove(hex_byte.begin(), hex_byte.end(), ' '), hex_byte.end());
        hex_byte.erase(remove(hex_byte.begin(), hex_byte.end(), '\n'), hex_byte.end());
        hex_byte.erase(remove(hex_byte.begin(), hex_byte.end(), '\r'), hex_byte.end());
        
        if (hex_byte.length() == 2) {
            char byte = (char)strtol(hex_byte.c_str(), NULL, 16);
            result += byte;
        }
    }
    
    // 如果没有空格分隔，尝试直接解析
    if (result.empty() && !hex_str.empty()) {
        for (size_t i = 0; i < hex_str.length(); i += 2) {
            if (i + 1 < hex_str.length()) {
                string hex_byte = hex_str.substr(i, 2);
                char byte = (char)strtol(hex_byte.c_str(), NULL, 16);
                result += byte;
            }
        }
    }
    
    return result;
}

// 从二进制字符串解码 - 增强版，支持UTF-8二进制格式
string from_binary_string(const string& bin_str) {
    string result;
    stringstream ss(bin_str);
    string bin_byte;
    
    // 统计连续8位二进制数的数量
    int byte_count = 0;
    while (ss >> bin_byte) {
        if (bin_byte.length() == 8) {
            byte_count++;
        }
    }
    
    // 重置流
    ss.clear();
    ss.str(bin_str);
    
    // 如果是大量连续字节，按简单方式处理
    if (byte_count > 10) {
        while (ss >> bin_byte) {
            if (bin_byte.length() == 8) {
                bitset<8> bits(bin_byte);
                char byte = static_cast<char>(bits.to_ulong());
                result += byte;
            }
        }
    } else {
        // 处理可能有字符分隔的情况
        string line;
        while (getline(ss, line)) {
            stringstream line_ss(line);
            while (line_ss >> bin_byte) {
                if (bin_byte.length() == 8) {
                    bitset<8> bits(bin_byte);
                    char byte = static_cast<char>(bits.to_ulong());
                    result += byte;
                }
            }
        }
    }
    
    // 如果没有空格分隔，尝试直接解析
    if (result.empty() && !bin_str.empty()) {
        // 移除所有空格
        string clean_str = bin_str;
        clean_str.erase(remove(clean_str.begin(), clean_str.end(), ' '), clean_str.end());
        clean_str.erase(remove(clean_str.begin(), clean_str.end(), '\n'), clean_str.end());
        clean_str.erase(remove(clean_str.begin(), clean_str.end(), '\r'), clean_str.end());
        
        for (size_t i = 0; i < clean_str.length(); i += 8) {
            if (i + 7 < clean_str.length()) {
                string bin_byte = clean_str.substr(i, 8);
                bitset<8> bits(bin_byte);
                char byte = static_cast<char>(bits.to_ulong());
                result += byte;
            }
        }
    }
    
    return result;
}

// 从UTF-8二进制字符串解码 - 新增函数
string from_utf8_binary(const string& utf8_bin_str) {
    // UTF-8二进制格式其实就是二进制表示，使用相同的解码函数
    return from_binary_string(utf8_bin_str);
}

// 从UTF-8十六进制字节序列解码
string from_utf8_bytes(const string& utf8_str) {
    return from_hex_string(utf8_str);
}

// 显示帮助信息
void show_help() {
    cout << endl << "文件解码工具" << endl;
    cout << "=============" << endl;
    cout << "使用方法：" << endl << endl;
    cout << "模式1: 自动检测编码" << endl;
    cout << "  file_decoder [编码文件]" << endl << endl;
    cout << "模式2: 手动指定编码" << endl;
    cout << "  file_decoder [编码文件] [编码方式]" << endl << endl;
    cout << "参数说明：" << endl << endl;
    cout << "  编码文件 - 要解码的文件" << endl;
    cout << "  编码方式 - 支持的编码方式（可选）：" << endl;
    cout << "       utf-8        从UTF-8字节序列十六进制解码" << endl;
    cout << "       utf-8-bin    从UTF-8字节序列（进制解码" << endl;
    cout << "       ascii-2      从ASCII二进制解码" << endl;
    cout << "       ascii-16     从ASCII十六进制解码" << endl;
    cout << "       base64       从Base64解码" << endl;
    cout << "       url          从URL解码" << endl;
    cout << endl;
    cout << "注意：" << endl;
    cout << "  1. 如果不指定编码方式，程序会自动从文件名检测编码" << endl;
    cout << "  2. 支持的文件名后缀：" << endl;
    cout << "     _utf8_bin  (utf-8-bin格式)" << endl;
    cout << "     _utf8      (utf-8格式)" << endl;
    cout << "     _binary    (ascii-2格式)" << endl;
    cout << "     _hex       (ascii-16格式)" << endl;
    cout << "     _base64    (base64格式)" << endl;
    cout << "     _url       (url格式)" << endl;
    cout << "  3. 输出文件自动保存在原文件目录" << endl;
    cout << "  4. 如果原文件有扩展名，恢复原扩展名；否则添加_decoded.txt" << endl;
    cout << endl;
    cout << "示例：" << endl;
    cout << "  file_decoder document_utf8_bin.bin" << endl;
    cout << "  file_decoder document_utf8.hex" << endl;
    cout << "  file_decoder image_base64.txt" << endl;
    cout << "  file_decoder data_binary.bin ascii-2" << endl;
    cout << endl;
    cout << "要显示此帮助信息，请输入：file_decoder /?" << endl;
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
    if (argc != 2 && argc != 3) {
        cerr << endl << "错误：参数数量不正确！" << endl;
        cerr << "使用方法：file_decoder [编码文件名] [编码方式（可选）]" << endl;
        cerr << "使用 /? 查看详细帮助信息" << endl;
        return 1;
    }
    
    string input_filename = argv[1];
    string encoding;
    bool auto_detect = false;
    
    try {
        if (argc == 2) {
            // 模式1: 自动检测编码
            encoding = detect_encoding_from_filename(input_filename);
            if (encoding.empty()) {
                cerr << "错误：无法从文件名检测编码类型！" << endl;
                cerr << "文件名应包含以下后缀之一：" << endl;
                cerr << "  _utf8_bin  (utf-8二进制格式)" << endl;
                cerr << "  _utf8      (utf-8十六进制格式)" << endl;
                cerr << "  _binary    (ascii-2格式)" << endl;
                cerr << "  _hex       (ascii-16格式)" << endl;
                cerr << "  _base64    (base64格式)" << endl;
                cerr << "  _url       (url格式)" << endl;
                cerr << "或者使用：file_decoder [文件名] [编码方式]" << endl;
                return 1;
            }
            auto_detect = true;
        } else if (argc == 3) {
            // 模式2: 手动指定编码
            encoding = argv[2];
        }
        
        // 生成输出文件名
        string output_filename = generate_output_filename(input_filename, encoding);
        
        // 读取输入文件
        string input_content = read_file(input_filename);
        
        string result;
        
        // 根据编码方式解码
        if (encoding == "utf-8-bin") {  // 新增：UTF-8二进制解码
            result = from_utf8_binary(input_content);
        }
        else if (encoding == "utf-8") {
            result = from_utf8_bytes(input_content);
        }
        else if (encoding == "ascii-2") {
            result = from_binary_string(input_content);
        }
        else if (encoding == "ascii-16") {
            result = from_hex_string(input_content);
        }
        else if (encoding == "base64") {
            result = base64_decode(input_content);
        }
        else if (encoding == "url") {
            result = url_decode(input_content);
        }
        else {
            cerr << "错误：不支持的编码方式: " << encoding << endl;
            cerr << "支持的编码方式: utf-8-bin, utf-8, ascii-2, ascii-16, base64, url" << endl;
            return 1;
        }
        
        // 写入输出文件
        write_file(output_filename, result);
        
        cout << "解码完成！" << endl;
        cout << "输入文件: " << input_filename << endl;
        cout << "输出文件: " << output_filename << endl;
        if (auto_detect) {
            cout << "检测编码: " << encoding << endl;
        } else {
            cout << "使用编码: " << encoding << endl;
        }
        cout << "解码大小: " << result.size() << " 字节" << endl;
        
    } catch (const exception& e) {
        cerr << "错误: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "未知错误发生！" << endl;
        return 1;
    }
    
    return 0;
}

#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cstdlib>

// Base64编解码相关函数
const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(const std::string& input) {
    std::string ret;
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

std::string base64_decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;
    
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

// URL编码解码函数
std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    
    for (size_t idx = 0; idx < value.size(); ++idx) {
        char c = value[idx];
        // 保留字母数字字符和一些特殊字符
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        
        // 其他字符都进行百分号编码
        escaped << '%' << std::setw(2) << int((unsigned char)c);
    }
    
    return escaped.str();
}

std::string url_decode(const std::string& value) {
    std::ostringstream decoded;
    
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%') {
            if (i + 2 < value.length()) {
                std::string hex = value.substr(i + 1, 2);
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

// UTF-8转换函数 - 显示UTF-8字节序列（十六进制）
std::string to_utf8_bytes(const std::string& input) {
    std::ostringstream oss;
    for (size_t i = 0; i < input.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        
        // 判断UTF-8字符的字节数
        int bytes = 0;
        if (c <= 0x7F) {
            bytes = 1;  // ASCII字符
        } else if ((c & 0xE0) == 0xC0) {
            bytes = 2;  // 2字节UTF-8
        } else if ((c & 0xF0) == 0xE0) {
            bytes = 3;  // 3字节UTF-8
        } else if ((c & 0xF8) == 0xF0) {
            bytes = 4;  // 4字节UTF-8
        } else {
            bytes = 1;  // 无效UTF-8，按单字节处理
        }
        
        // 输出当前字符的所有字节
        for (int j = 0; j < bytes && (i + j) < input.length(); ++j) {
            unsigned char byte = static_cast<unsigned char>(input[i + j]);
            oss << std::hex << std::setw(2) << std::setfill('0') 
                << static_cast<int>(byte);
            if (j < bytes - 1) {
                oss << " ";
            }
        }
        
        if (bytes > 1) {
            i += (bytes - 1);  // 跳过已处理的字节
        }
        
        if (i < input.length() - 1) {
            oss << " ";
        }
    }
    return oss.str();
}

// 改进版UTF-8转换函数 - 显示字符和字节信息
std::string to_utf8_detailed(const std::string& input) {
    std::ostringstream oss;
    oss << "UTF-8字节序列 (十六进制):\n";
    
    for (size_t i = 0; i < input.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(c);
        
        if (i < input.length() - 1) {
            oss << " ";
        }
    }
    
    return oss.str();
}

// 简化版UTF-8转换 - 只显示字节序列
std::string to_utf8_simple(const std::string& input) {
    std::ostringstream oss;
    for (size_t i = 0; i < input.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(c);
        
        if (i < input.length() - 1) {
            oss << " ";
        }
    }
    return oss.str();
}

// ASCII-二进制转换
std::string to_ascii_binary(const std::string& input) {
    std::string result;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        std::bitset<8> bits(static_cast<unsigned char>(c));
        result += bits.to_string();
        if (i != input.size() - 1) {
            result += " ";
        }
    }
    return result;
}

// ASCII-十六进制转换
std::string to_ascii_hex(const std::string& input) {
    std::ostringstream oss;
    for (size_t i = 0; i < input.length(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(static_cast<unsigned char>(input[i]));
        if (i != input.length() - 1) {
            oss << " ";
        }
    }
    return oss.str();
}

int main() {
    std::string input_str;
    std::string encoding;
    
    // 读取输入字符串
    std::getline(std::cin, input_str);
    
    // 读取编码方式
    std::getline(std::cin, encoding);
    
    std::string result;
    
    try {
        if (encoding == "utf-8") {
            // 使用简化版，只显示UTF-8字节序列
            result = to_utf8_simple(input_str);
        }
        else if (encoding == "ascii-2") {
            result = to_ascii_binary(input_str);
        }
        else if (encoding == "ascii-16") {
            result = to_ascii_hex(input_str);
        }
        else if (encoding == "base64") {
            result = base64_encode(input_str);
        }
        else if (encoding == "url") {
            result = url_encode(input_str);
        }
        else {
            std::cerr << "错误：不支持的编码方式，支持的编码方式：utf-8 、ascii-2 、ascii-16 、base64 、url" << std::endl;
            return 1;
        }
        
        // 输出结果
        std::cout << result << std::endl;
        
    } catch (...) {
        std::cerr << "错误：转换过程中发生异常" << std::endl;
        return 1;
    }
    
    return 0;
}

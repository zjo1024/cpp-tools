# C++ 编译器
CXX = g++
CXXFLAGS = -Wall -O2

# 所有工具名（不含扩展名）
TOOLS = ball-v str-e file_encoder file_decoder guard pin_to_top shredder prott jyt

# 每个工具需要的链接库
guard_LIBS = -luser32 -lkernel32
pin_to_top_LIBS = -luser32 -lgdi32
shredder_LIBS = -luser32
prott_LIBS = -ladvapi32
jyt_LIBS = -luser32 -lshell32 -ladvapi32 -lgdi32

# 默认目标：编译所有工具
all: $(TOOLS)

# 通用编译规则
%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@.exe $($*_LIBS)

# 清理
clean:
	del /f *.exe 2>nul || rm -f *.exe

# 显示帮助
help:
	@echo "可用命令:"
	@echo "  make         编译所有工具"
	@echo "  make 工具名  编译指定工具，如 make guard"
	@echo "  make clean   删除所有 .exe 文件"

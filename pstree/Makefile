# 编译器和编译选项  
CXX = g++  
CXXFLAGS = -Wall -std=c++11  
  
# 源文件和目标文件  
SRC = pstree.cpp  
TARGET = pstree  
  
# 默认目标  
all: $(TARGET)  
  
# 链接目标文件生成可执行文件  
$(TARGET): $(SRC)  
    $(CXX) $(CXXFLAGS) -o $@ $^  
  
# 清除生成的目标文件和可执行文件  
clean:  
    rm -f $(TARGET) *.o  
  
# 运行程序（假设它在当前目录下）  
run: $(TARGET)  
    ./$(TARGET)  
  

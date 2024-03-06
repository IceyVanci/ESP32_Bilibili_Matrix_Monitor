// MAX7219 命令:
#define CMD_NOOP   0
#define CMD_DIGIT0 1
#define CMD_DIGIT1 2
#define CMD_DIGIT2 3
#define CMD_DIGIT3 4
#define CMD_DIGIT4 5
#define CMD_DIGIT5 6
#define CMD_DIGIT6 7
#define CMD_DIGIT7 8
#define CMD_DECODEMODE  9
#define CMD_INTENSITY   10
#define CMD_SCANLIMIT   11
#define CMD_SHUTDOWN    12
#define CMD_DISPLAYTEST 15

byte scr[NUM_MAX*8 + 8]; // 存储 LED 点阵的数组，额外 +8 是为了存储滚动字符

// 向所有 MAX7219 模块发送命令的函数
void sendCmdAll(byte cmd, byte data) {
  digitalWrite(CS_PIN, LOW); // 将 CS 引脚设置为低电平以开始 SPI 通信
  for (int i = NUM_MAX-1; i>=0; i--) { // 遍历所有 MAX7219 模块
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, cmd); // 发送命令字节
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, data); // 发送数据字节
  }
  digitalWrite(CS_PIN, HIGH); // 将 CS 引脚设置为高电平以结束 SPI 通信
}

// 当旋转 270 度时刷新显示的函数
void refreshAllRot270() {
  byte mask = 0x01; // 初始化掩码
  for (int c = 0; c < 8; c++) { // 遍历每一列
    digitalWrite(CS_PIN, LOW); // 将 CS 引脚设置为低电平以开始 SPI 通信
    for(int i=NUM_MAX-1; i>=0; i--) { // 遍历所有 MAX7219 模块
      byte bt = 0; // 初始化要发送的字节
      for(int b=0; b<8; b++) { // 遍历每一行
        bt <<= 1; // 将字节左移一位
        if(scr[i * 8 + b] & mask) bt |= 0x01; // 如果对应 LED 为开启状态，则设置该位
      }
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, CMD_DIGIT0 + c); // 发送当前列的命令字节
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, bt); // 发送当前列的数据字节
    }
    digitalWrite(CS_PIN, HIGH); // 将 CS 引脚设置为高电平以结束 SPI 通信
    mask <<= 1; // 将掩码左移一位
  }
}

// 刷新所有LED点阵显示内容（旋转90度）
void refreshAllRot90() 
{
  byte mask = 0x80; // 初始化掩码
  for (int c = 0; c < 8; c++) { // 遍历每一列
    digitalWrite(CS_PIN, LOW); // 将CS引脚置低以开始SPI通信
    for(int i=NUM_MAX-1; i>=0; i--) { // 遍历所有MAX7219模块
      byte bt = 0; // 初始化要发送的字节
      for(int b=0; b<8; b++) { // 遍历每一行
        bt >>= 1; // 将字节右移一位
        if(scr[i * 8 + b] & mask) bt |= 0x80; // 如果对应LED点亮，则设置位
      }
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, CMD_DIGIT0 + c); // 发送当前列的命令字节
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, bt); // 发送当前列的数据字节
    }
    digitalWrite(CS_PIN, HIGH); // 将CS引脚置高以结束SPI通信
    mask >>= 1; // 将掩码右移一位
  }
}

// 刷新所有LED点阵显示内容
void refreshAll() {
#if ROTATE==270
  refreshAllRot270(); // 若旋转270度，则调用旋转270度的刷新函数
#elif ROTATE==90
  refreshAllRot90(); // 若旋转90度，则调用旋转90度的刷新函数
#else
  for (int c = 0; c < 8; c++) { // 遍历每一列
    digitalWrite(CS_PIN, LOW); // 将CS引脚置低以开始SPI通信
    for(int i=NUM_MAX-1; i>=0; i--) { // 遍历所有MAX7219模块
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, CMD_DIGIT0 + c); // 发送当前列的命令字节
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, scr[i * 8 + c]); // 发送当前列的数据字节
    }
    digitalWrite(CS_PIN, HIGH); // 将CS引脚置高以结束SPI通信
  }
#endif
}

// 清除LED点阵显示内容
void clr()
{
  for (int i = 0; i < NUM_MAX*8; i++) scr[i] = 0; // 将所有LED点阵的内容清零
}

// 将LED点阵内容向左滚动
void scrollLeft()
{
  for(int i=0; i < NUM_MAX*8+7; i++) scr[i] = scr[i+1]; // 将LED点阵内容向左移动一位
}

// 初始化MAX7219驱动
void initMAX7219()
{
  pinMode(DIN_PIN, OUTPUT); // 将DIN引脚设置为输出模式
  pinMode(CLK_PIN, OUTPUT); // 将CLK引脚设置为输出模式
  pinMode(CS_PIN, OUTPUT); // 将CS引脚设置为输出模式
  digitalWrite(CS_PIN, HIGH); // 将CS引脚置高
  sendCmdAll(CMD_DISPLAYTEST, 0); // 发送显示测试命令
  sendCmdAll(CMD_SCANLIMIT, 7); // 发送扫描限制命令
  sendCmdAll(CMD_DECODEMODE, 0); // 发送解码模式命令
  sendCmdAll(CMD_INTENSITY, 1); // 发送亮度设置命令
  sendCmdAll(CMD_SHUTDOWN, 0); // 发送关闭命令
  clr(); // 清除LED点阵内容
  refreshAll(); // 刷新LED点阵内容
}


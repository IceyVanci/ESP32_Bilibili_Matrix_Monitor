//  硬件接线说明
//  GPIO 8 - DataIn
//  GPIO 4 - LOAD/CS
//  GPIO 5 - CLK
//  NodeMCU
//  29 - DataIn
//  28 - LOAD/CS
//  27 - CLK

#include "Arduino.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#define NUM_MAX 4
#define ROTATE 90
// for NodeMCU
#define DIN_PIN 8  // 29
#define CS_PIN  4  // 28
#define CLK_PIN 5  // 27
#include "max7219.h"
#include "fonts.h"
// =======================================================================
// 配置信息
// =======================================================================
const char* ssid     = "sample";                 // WiFi名
const char* password = "sample";            //WiFi密码
String channelId = "sample";                   //bilibili UID
long utcOffset = 8;                              //时区，默认 +8 为东八区（中国北京）
// =======================================================================
WiFiClientSecure client; 
void setup()
{
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 0);
  WiFi.enableAP(false);     //关闭AP热点
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift(" CCCP .~", 15, font, ' ');     //开机文字
  delay(500);      //开机文字显示时长
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);
  }
  Serial.println("");
  Serial.print("Connected: "); Serial.println(WiFi.localIP());
  Serial.println("Getting data ...");
  printStringWithShift("Bi ... ~", 15, font, ' ');
}
// =======================================================================
// 视频观看数
long viewCount;

// 订阅者数量
long subscriberCount;

// 计数器，用于跟踪程序中的循环次数
int cnt = 0;

// 记录时间的变量，1小时和24小时
unsigned long time1h, time24h;

// 本地时间戳和上次更新时间的本地毫秒数
long localEpoc = 0;
long localMillisAtUpdate = 0;

// 时、分、秒
int h, m, s;

// 日期
String date;

void loop()
{
  if (cnt <= 0) {
    if ((getYTData() == 0)) {
      cnt = 1;  // data is refreshed every 50 loops
    }
  }
  cnt--;
  updateTime();
  int del = 4000;
  int scrollDel = 50;
  int scroll = 25;
  int rollslow = 75;
  int rollfast = 35;
  char txt[10];

  //+++++++++++++++++++++粉丝数+++++++++++++++++++++//
  printStringWithShift("  Fans: ", scrollDel, font, ' '); // eng
  printValueWithShift(subscriberCount, scrollDel, 0);
  delay(del); 
   //+++++++++++++++++++++播放数+++++++++++++++++++++//
  printStringWithShift("  Play: ", scrollDel, font, ' '); // eng
  printValueWithShift(viewCount, scrollDel, 0);
  delay(del);
  //+++++++++++++++++++++时间（根据前面设定的时区决定）+++++++++++++++++++++//
  sprintf(txt, "    %02d:%02d  ", h, m);
  printStringWithShift(txt, scrollDel, font, ' '); // real time
  delay(del);
}
// =======================================================================
int dualChar = 0; // 用于记录双字符
// =======================================================================
// 计算字符的宽度
int charWidth(char ch, const uint8_t *data) {
  int len = pgm_read_byte(data);
  return pgm_read_byte(data + 1 + ch * len);
}
// =======================================================================
// 显示字符
int showChar(char ch, const uint8_t *data) {
  int len = pgm_read_byte(data);
  int i, w = pgm_read_byte(data + 1 + ch * len);
  scr[NUM_MAX * 8] = 0;
  for (i = 0; i < w; i++)
    scr[NUM_MAX * 8 + i + 1] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  return w;
}
// =======================================================================
void printCharWithShift(unsigned char c, int shiftDelay, const uint8_t *data, int offs)
{
  // 函数用于在显示器上打印一个字符，并带有滚动效果
  if (c < offs || c > MAX_CHAR) return; // 如果字符不在有效范围内，则退出函数
  c -= offs; // 根据偏移量调整字符
  int w = showChar(c, data); // 获取字符的宽度
  for (int i = 0; i < w + 1; i++) { // 循环滚动字符
    delay(shiftDelay); // 等待一段时间
    scrollLeft(); // 向左滚动
    refreshAll(); // 刷新显示器
  }
}
// =======================================================================
void printStringWithShift(const char *s, int shiftDelay, const uint8_t *data, int offs)
{
  // 函数用于在显示器上打印一个字符串，并带有滚动效果
  while (*s) // 循环遍历字符串中的每个字符
    printCharWithShift(*s++, shiftDelay, data, offs); // 调用打印单个字符的函数
}

void printValueWithShift(long val, int shiftDelay, int sign)
{
  const uint8_t *digits = digits5x7;       // 使用5x7字体，适用于最多5位数字
  if (val > 1999999) digits = digits3x7;   // 使用3x7字体，适用于最多8位数字
  else if (val > 99999) digits = digits4x7; // 使用4x7字体，适用于最多6-7位数字
  String str = String(val); // 将长整型数值转换为字符串
  if (sign) { // 如果需要显示符号
    if (val < 0) str = ";" + str; // 如果值为负数，在值前加上';'
    else str = "<" + str; // 如果值为正数或零，在值前加上'<'
  }
  const char *s = str.c_str(); // 将字符串转换为C风格字符串
  int wd = 0;
  while (*s) wd += 1 + charWidth(*s++ - '0', digits); // 计算字符串的总宽度
  wd--;
  int wdL = (NUM_MAX * 8 - wd) / 2; // 计算左侧空白区域的宽度
  int wdR = NUM_MAX * 8 - wdL - wd; // 计算右侧空白区域的宽度
  while (wdL > 0) { // 在左侧空白区域打印冒号
    printCharWithShift(':', shiftDelay, digits, '0');
    wdL--;
  }
  s = str.c_str(); // 将字符串重新指向第一个字符
  while (*s) printCharWithShift(*s++, shiftDelay, digits, '0'); // 逐个打印字符
  while (wdR > 0) { // 在右侧空白区域打印冒号
    printCharWithShift(':', shiftDelay, digits, '0');
    wdR--;
  }
}
// =====================订阅数==================================================
const char *ytHost = "api.bilibili.com"; // 定义API主机地址
int getYTData()
{
  WiFiClientSecure client; // 创建安全的WiFi客户端对象
  Serial.print("connecting to "); Serial.println(ytHost); // 打印连接信息
  // 连接到YouTube主机的端口443
  if (!client.connect(ytHost, 443)) {
    Serial.println("connection failed"); // 连接失败打印错误信息
    return -1; // 返回错误码
  }
  // 构建HTTP请求字符串
  String cmd = String("GET /x/relation/stat?vmid=") + channelId + " HTTP/1.1\r\n" +
               "Host: " + ytHost + "\r\nUser-Agent: ESP32/1.1\r\nConnection: close\r\n\r\n";
  client.print(cmd); // 发送HTTP请求
  // 等待服务器响应
  int repeatCounter = 10;
  while (!client.available() && repeatCounter--) {
    Serial.println("Waiting for response."); 
    delay(500);
  }
  // 读取服务器响应
  String line, buf = "";
  int startJson = 0, dateFound = 0;
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n'); // 读取一行
    if (line[0] == '{') startJson = 1; // 标记开始JSON数据
    if (startJson) {
      // 处理行中的JSON数据
      for (int i = 0; i < line.length(); i++)
        if (line[i] == '[' || line[i] == ']') line[i] = ' '; // 移除非法字符
      buf += line + "\n"; // 添加到数据缓冲区
    }
    if (!dateFound && line.startsWith("Date: ")) {
      // 读取日期信息
      dateFound = 1;
      date = line.substring(6, 22);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s); // 更新本地时钟
    }
  }
  client.stop(); // 关闭连接
  DynamicJsonDocument jsonDoc(1024); // 创建JSON文档对象，设置初始容量为1024字节
  DeserializationError error = deserializeJson(jsonDoc, buf); // 将缓冲区中的JSON数据解析到JSON文档对象中
  if (error) {
    Serial.println("deserializeJson() failed"); // JSON解析失败打印错误信息
    return -1; // 返回错误码
  }
  JsonObject root = jsonDoc.as<JsonObject>(); // 获取根JSON对象
  subscriberCount = root["data"]["follower"]; // 从JSON中提取订阅者数量信息
  return 0; // 返回成功码
}
// =======================================================================
// =====================播放数=================================================
const char *playHost = "api.bilibili.com"; // 定义API地址
int getplayData()
{
  WiFiClientSecure client; // 创建安全的WiFi客户端对象
  Serial.print("connecting to "); Serial.println(playHost); // 打印连接信息
  if (!client.connect(playHost, 443)) {
    Serial.println("connection failed"); // 连接失败打印错误信息
    return -1; // 返回错误码
  }
  // 构建HTTP请求字符串
  String cmd = String("GET /x/space/upstat?mid=") + channelId + " HTTP/1.1\r\n" +
               "Host: " + playHost + "\r\n" +
               "User-Agent: ESP32/1.1\r\n" +
               "Connection: close\r\n" +
               "Cookie: SESSDATA= \r\n" + // 设置Cookie信息
               "\r\n";
  // 等待服务器响应
  int repeatCounter = 10;
  while (!client.available() && repeatCounter--) {
    Serial.println("Waiting for response."); 
    delay(500);
  }
  // 读取服务器响应
  String line, buf = "";
  int startJson = 0, dateFound = 0;
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n'); // 读取一行
    if (line[0] == '{') startJson = 1; // 标记开始JSON数据
    if (startJson) {
      // 处理行中的JSON数据
      for (int i = 0; i < line.length(); i++)
        if (line[i] == '[' || line[i] == ']') line[i] = ' '; // 移除非法字符
      buf += line + "\n"; // 添加到数据缓冲区
    }
    if (!dateFound && line.startsWith("Date: ")) {
      // 读取日期信息
      dateFound = 1;
      date = line.substring(6, 22);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s); // 更新本地时钟
    }
  }
  client.stop(); // 关闭连接
  DynamicJsonDocument jsonDoc(1024); // 创建JSON文档对象，设置初始容量为1024字节
  DeserializationError error = deserializeJson(jsonDoc, buf); // 将缓冲区中的JSON数据解析到JSON文档对象中
  if (error) {
    Serial.println("deserializeJson() failed"); // JSON解析失败打印错误信息
    return -1; // 返回错误码
  }
  JsonObject root = jsonDoc.as<JsonObject>(); // 获取根JSON对象
  int videoCount = root["data"]["archive"]["view"]; // 从JSON中提取视频播放次数信息
  return 0; // 返回成功码
}
void updateTime() {
  // 计算当前时刻的时间戳
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  // 将时间戳转换为UTC时间并校正时区偏移
  long epoch = (curEpoch + 3600 * utcOffset + 86400L) % 86400L;
  // 提取小时、分钟和秒
  h = (epoch / 3600) % 24; // 小时（0-23）
  m = (epoch % 3600) / 60; // 分钟（0-59）
  s = epoch % 60; // 秒（0-59）
}

#include "esphome.h"

#define CHECK_BIT(var, pos) (((var) >> (pos)) & 1)

class LD2410S : public PollingComponent, public UARTDevice
{
public:
  LD2410S(UARTComponent *parent) : UARTDevice(parent) {}

  BinarySensor *hasTarget = new BinarySensor(); //人体存在判断
  BinarySensor *lastCommandSuccess = new BinarySensor();
  Sensor *TargetDistance = new Sensor(); //距离
  TextSensor *outputMode = new TextSensor(); //输出模式：极简（推荐）、正常、门限值设置

  long lastPeriodicMillis = millis();

  //发送控制命令封包
  void sendCommand(char *commandStr, char *commandValue, int commandValueLen)
  {
    lastCommandSuccess->publish_state(false);
    // frame start bytes
    write_byte(0xFD);
    write_byte(0xFC);
    write_byte(0xFB);
    write_byte(0xFA);
    // length bytes
    int len = 2;
    if (commandValue != nullptr)
      len += commandValueLen;
    write_byte(lowByte(len));
    write_byte(highByte(len));
    // command string bytes
    write_byte(commandStr[0]);
    write_byte(commandStr[1]);
    // command value bytes
    if (commandValue != nullptr)
    {
      for (int i = 0; i < commandValueLen; i++)
      {
        write_byte(commandValue[i]);
      }
    }
    // frame end bytes
    write_byte(0x04);
    write_byte(0x03);
    write_byte(0x02);
    write_byte(0x01);
    delay(50);
  }

  int twoByteToInt(char firstByte, char secondByte)
  {
    return (int16_t)(secondByte << 8) + firstByte;
  }

  //处理传感器返回的周期性数据
  void handlePeriodicData(char *buffer, int len)
  {
    if (len < 5)
      return; // 1 frame start bytes + 0 length bytes + 1 data end byte + 2 crc byte + 1 frame end bytes
    if (buffer[0] != 0xF4 && buffer[0] != 0x6E ) 
      return; // 校验帧头数据
    if (buffer[len - 1] != 0x62 && buffer[len - 1] != 0xF5 ) 
      return;//校验帧尾数据
    char dataType = buffer[0];
    /*
      Target states: 9th byte
      0x00 = No target
      0x01 = No target
      0x02 = Still targets
      0x03 = Moving+Still targets
    */
    char stateByte = buffer[1];
    hasTarget->publish_state(stateByte != 0x00 && stateByte != 0x01);
    /*
      Reduce data update rate to prevent home assistant database size glow fast
    */
    long currentMillis = millis();
    if (currentMillis - lastPeriodicMillis < 1000)
      return;
    lastPeriodicMillis = currentMillis;

    /*
      Moving target distance: 10~11th bytes
      Moving target energy: 12th byte
      Still target distance: 13~14th bytes
      Still target energy: 15th byte
      Detect distance: 16~17th bytes
    */
    int newTargetDistance = twoByteToInt(buffer[2], buffer[3]);
    if (TargetDistance->get_state() != newTargetDistance)
      TargetDistance->publish_state(newTargetDistance);
    if (dataType == 0x6E)
    { 
      outputMode->publish_state("Simplified mode");
    } 
    if (dataType == 0xF4)
    {
      outputMode->publish_state("Nomal mode");
    }
  }


  //处理返回值
  void handleACKData(char *buffer, int len)
  {
    if (len < 10)
      return;
    if (buffer[0] != 0xFD || buffer[1] != 0xFC || buffer[2] != 0xFB || buffer[3] != 0xFA)
      return; // check 4 frame start bytes
    if (buffer[7] != 0x01)
      return;
    if (twoByteToInt(buffer[8], buffer[9]) != 0x00)
    {
      lastCommandSuccess->publish_state(false);
      return;
    }
    lastCommandSuccess->publish_state(true);
  }
  
  void readline(int readch, char *buffer, int len)
  {
    static int pos = 0;

    if (readch >= 0)
    {
      if (pos < len - 1)
      {
        buffer[pos++] = readch;
        buffer[pos] = 0;
      }
      else
      {
        pos = 0;
      }
      if (pos >= 4)
      {
        if (buffer[pos - 4] == 0xF8 && buffer[pos - 3] == 0xF7 && buffer[pos - 2] == 0xF6 && buffer[pos - 1] == 0xF5)
        {
          handlePeriodicData(buffer, pos);
          pos = 0; // Reset position index ready for next time
        }
        else if (buffer[pos - 4] == 0x04 && buffer[pos - 3] == 0x03 && buffer[pos - 2] == 0x02 && buffer[pos - 1] == 0x01)
        {
          handleACKData(buffer, pos);
          pos = 0; // Reset position index ready for next time
        }
        else if (buffer[pos-1] == 0x62)
        {
          handlePeriodicData(buffer, pos);
          pos = 0; // Reset position index ready for next time
        }
      }
    }
    return;
  }

  void setup() override
  {
    set_update_interval(15000);
  }

  void loop() override
  {
    const int max_line_length = 80;
    static char buffer[max_line_length];
    while (available())
    {
      readline(read(), buffer, max_line_length);
    }
  }
  
  void update()
  {
  }
};

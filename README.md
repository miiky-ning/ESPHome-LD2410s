# ESPHome-LD2410s
Hi-Link LD2410s For ESPHome

## 适用于HLK-LD2410S低功耗人体存在感应模组的驱动。<br>
设备型号：HLK-LD2410S<br>
固件版本：1.1.1<br>

## 使用方法<br>
将LD2410_uart.h文件放置在ESPHome的配置文件夹中即可使用<br>
example：/homeassistant/esphome/components<br>
yaml代码详见hlkld2410s


名称 | 功能 | 说明
---- | ---- | ----
3V3 | 电源输入 | 3.0V~3.6V，Typ.3.3V 
GND | 接地 | - 
OT1 | UART_TX | 0~3.3V 
RX | UART_RX | 0~3.3V 
OT2 | IO，用于上报检测状态：高电平为有人，低电平为无人 | 0~3.3 V

#  苏州德亚缴费机接入

## 修改记录
> 2021-05-10 初版
> 2021-05-13 增加状态上报部分关于是否使能扫码

## 主题定义
主题定义由 厂商/产品线/产品型号/{项目名}/{设备ID}/业务参数组成
其中缴费机参数关于 “厂商/产品线/产品型号/” 定义为 ts/spm/x5/

## 订阅定义

### 服务器订阅
服务器需需订阅qrcode主题、连接消息主题，设备状态主题，和相关的响应主题，既
ts/spm/x5/{ProjectName}/{DeviecId}/qrcode 
ts/spm/x5/{ProjectName}/{DeviecId}/status
ts/spm/x5/{ProjectName}/{DeviecId}/link

设备端需订阅如下主题
ts/spm/x5/{ProjectName}/{DeviecId}/ledshow
ts/spm/x5/{ProjectName}/{DeviecId}/soundplay
ts/spm/x5/{ProjectName}/{DeviecId}/sysctr


## 缴费机接入协议

|   功能       | 数据方向 |  Qos| 主题 | 内容 |
| -------  | ------------ | -------- | ------- | ----------- |
| 请求LED显示  | Server->MCU | 1| ts/spm/x5/{ProjectName}/{DeviecId}/ledshow      | 见代码段1| 
| 请求语音播报 | Server->MCU |1 | ts/spm/x5/{ProjectName}/{DeviecId}/soundplay    | 见代码段2| 
| 下发设备控制 | Server->MCU |1 | ts/spm/x5/{ProjectName}/{DeviecId}/sysctr       | 见代码段3| 
| 上报扫码信息 | MCU->Server |1 | ts/spm/x5/{ProjectName}/{DeviecId}/qrcode       | 见代码段4| 
| 设备状态状态消息 | MCU->Server | 0| ts/spm/x5/{ProjectName}/{DeviecId}/status   | 见代码段5| 
| 设备连接消息 | MCU->Server |0| ts/spm/x5/{ProjectName}/{DeviecId}/link          | 见代码段6|  

**代码段1**
```
{ 
    “line1”:”hello”,
    “line2”:”world”,
    “line3”:”this is test”
    “line4”:”this is test”
    "color":"red",
}
```
> line1-line4 表示四行LED显示内容
 
**代码段2**
```
{ 
    "text":"只是一段语音",
    "vol":4
}
```

**代码段3**
```
{ 
    "type":"reboot", 
    "param":""
}
```
> 参数以及类型如下

|   type       |  param |  说明 |
| ------  | ----- | ----- |
| sync_time  | "2021-05-12 12:12:12" | 系统对时 |
| enable_scan | "yes|no" | 使能扫码信息上报，设备默认开启 |
| reboot  | 无| 设备重启 |
| link_check   | 无| 检测设备状态，设备收到此指令后，立刻发送一个连接状态帧 |
 
**代码段4**
```
{  
    "id": "{ProJectName}/{DeviceId}",
    "time_stamp":"2021-05-12 12:12:12",
    "qrcode":"12312312313123123", 
}
``` 

**代码段5**
```
{ 
    "id": "{ProJectName}/{DeviceId}"
    "cpu":0,
    "en_scan":true,
    "mem_total":123123,
    "mem_free":123123,
    "disk_total":123123,
    "disk_free":123123
}
``` 

**代码段6**
```
{ 
    "id": "{ProJectName}/{DeviceId}",
    "type":"online"
}
```
> type有如下参数
1. online:  设备上线
2. offline: 设备离线 


#  苏州德亚缴费机接入

## 主题定义
主题定义由 厂商/产品线/产品型号/{项目名}/{设备ID}/业务参数组成
其中缴费机参数关于 “厂商/产品线/产品型号/” 定义为 ts/spm/x5/

## 订阅定义
### 服务器订阅
服务器需需订阅qrcode主题、连接消息主题，设备状态主题，和相关的响应主题，既
ts/spm/x5/{ProjectName}/{DeviecId}/request/qrcode 
ts/spm/x5/{ProjectName}/{DeviecId}/status
ts/spm/x5/{ProjectName}/{DeviecId}/link

设备端需订阅如下主题
ts/spm/x5/{ProjectName}/{DeviecId}/request/ledshow
ts/spm/x5/{ProjectName}/{DeviecId}/request/soundplay
ts/spm/x5/{ProjectName}/{DeviecId}/request/sysctr


## 缴费机接入协议

|   功能       | 数据方向 |  Qos| 主题 | 内容 |
| -------  | ------------ | -------- | ------- | ----------- |
| 请求LED显示  | Server->MCU | 1| ts/spm/x5/{ProjectName}/{DeviecId}/request/ledshow     | 见代码段1|
| 响应LED控制  | MCU->Server | 1| ts/spm/x5/{ProjectName}/{DeviecId}/response/ledshow    | 见代码段2| 
| 请求语音播报 | Server->MCU |1 | ts/spm/x5/{ProjectName}/{DeviecId}/request/soundplay    | 见代码段3|
| 响应语音播报 | MCU->Server |1 | ts/spm/x5/{ProjectName}/{DeviecId}/response/soundplay   | 见代码段4|
| 上报扫码信息 | MCU->Server |1 | ts/spm/x5/{ProjectName}/{DeviecId}/request/qrcode       | 见代码段5|
| 响应扫码信息 | Server->MCU |1 | ts/spm/x5/{ProjectName}/{DeviecId}/response/qrcode      | 见代码段6|
| 下发设备控制 | Server->MCU |1 | ts/spm/x5/{ProjectName}/{DeviecId}/request/sysctr       | 见代码段7|
| 响应设备控制 | MCU->Server |1 | ts/spm/x5/{ProjectName}/{DeviecId}/response/sysctr      | 见代码段8|
| 设备状态状态消息 | MCU->Server | 0| ts/spm/x5/{ProjectName}/{DeviecId}/status            | 见代码段9| 
| 设备连接消息 | MCU->Server |0| ts/spm/x5/{ProjectName}/{DeviecId}/link                  | 见代码段10|  

**代码段1**
```
{
    "task_id":"12341234",
    “line1”:”hello”,
    “line2”:”world”,
    “line3”:”this is test”
    “line4”:”this is test”
    "color":"red",
}
```

**代码段2**
```
{
    "task_id":"12341234",
    "code":0,
    "msg","success"
}
```

**代码段3**
```
{
    "task_id":"12341234",
    "text":"只是一段语音",
    "vol":4
}
```

**代码段4**
```
{
    "task_id":"12341234",
    "code":0,
    "msg","success"
}
```

**代码段5**
```
{
    "task_id":"12341234",
    "qrcode":"12312312313123123", 
}
```

**代码段6**
```
{
    "task_id":"12341234",
    "code":0,
    "msg","success"
}
```

**代码段7**
```
{
    "task_id":"12341234",
    "type":"reboot", 
    "param":""
}
```
> 参数以及类型如下

|   type       |  param |  说明 |
| ------  | ----- | ----- |
| sync_time  | "2021-05-12 12:12:12" | 系统对时 |
| reboot  | 无| 设备重启 |


**代码段8**
```
{
    "task_id":"12341234",
    "code":0,
    "msg","success"
}
```

**代码段9**
```
{ 
    "cpu":0,
    "mem_total":123123,
    "mem_free":123123,
    "disk_total":123123,
    "disk_free":123123
}
```
> 无需响应

**代码段10**
```
{ 
    "type":"online" 
}
```
> 消息无需响应，type有如下参数
1. online:  设备上线
2. offline: 设备离线 



#include <stdio.h>

const char *default_led = 
"# 默认情况下的显示内容对应关系，可以修改该配置达到修改默认数据的作用\r\n"
"#可配置参数有，LINEn,COLORn,LIGHT,FORMAT,SOUND其中n代表第几行也可以不使用表示所有行适用\r\n"
"#其中 \r\n"
"# LINE对应文字内容，其中文字要用\"\"包住\r\n"
"# COLOR对应文字颜色，可配置颜色有： RED,BLUE,GREEN,CYAN,YELLOW,WHITE,PURPLE\r\n"
"# LIGHT对应文字亮度，可配置1,2,3\r\n"
"# FORMAT对应对齐格式，常用的有1居中，5滚动\r\n"
"# SOUND对应语音播报内容\r\n"
"# 默认情况下的显示内容对应关系，可以修改该配置达到修改默认数据的作用\r\n"
"# 默认情况下的显示内容对应关系，可以修改该配置达到修改默认数据的作用\r\n"
"\r\n"
"0=COLOR=CYAN LINE1=\"欢迎使用\" LINE2=\"移动支付\" LINE3=\"支持\" LINE4=\"支付宝，微信付款码\" FORMAT1=1 FORMAT2=1 FORMAT3=1 FORMAT4=5 SPEED=4\r\n"
"1=COLOR=CYAN LINE1=\"请缴费\" LINE2=\"@money@元\" LINE3=\"请展示\" LINE4=\"付款码\" FORMAT=1  SOUND=\"请缴费@money@元,请展示付款二维码\" VOL=4  # 开始支付，上位机下发金额后，播报\r\n"
"2=COLOR=RED LINE1=\"缴费异常\" LINE2=\"网络请求\" LINE3=\"错误\" LINE4=\" \" FORMAT=1  SOUND=\"缴费失败,请重试\"  VOL=4  #向后台发起支付请求，但是网络处理异常\r\n"
"3=COLOR=CYAN LINE1=\"支付中\" LINE2=\"请稍后\" LINE3=\" \" LINE4=\" \" FORMAT=1 SOUND=\"请稍后\" VOL=4   #向后台请求支付，在轮询支付状态之前的播报内容\r\n"
"4=COLOR=RED LINE1=\"缴费失败\" LINE2=\"支付异常\" LINE3=\" \" LINE4=\" \" FORMAT=1  SOUND=\"缴费失败,支付异常\" VOL=4   #支付请求，网络正常，但是后台给的状态是失败，可能是一些错误（付款码错误等信息）\r\n"
"5=COLOR=CYAN LINE1=\"缴费成功\" LINE2=\"祝您\" LINE3=\"一路平安\" LINE4=\" \" FORMAT=1  SOUND=\"缴费成功,请拿好发票，祝您一路平安\"  VOL=4  # 请扫码支持语音\r\n"
"6=COLOR=CYAN LINE1=\"扫码成功\" LINE2=\"请稍后\" LINE3=\" \" LINE4=\" \" FORMAT=1  SOUND=\"扫码成功\"  VOL=4     # 扫到码返回状态，暂时弃用\r\n"
"7=COLOR=RED LINE1=\"网络连接异常\" LINE2=\"无法发起支付\" LINE3=\" \" LINE4=\" \" SOUND=\"网络异常，\"   VOL=4  # 上位机发起来支付请求时，网络异常LED显示\r\n"
"8=COLOR=RED LINE1=\"缴费失败\" LINE2=\"请交现金\" LINE3=\" \" LINE4=\" \" SOUND=\"缴费失败，请交现金\"  VOL=4  # 发起来支付请求时，网络异常提示\r\n"
"9=COLOR=RED SOUND=\"交易取消\" VOL=4   # 收到上位机交易取消时给提示音\r\n"
"10=COLOR=RED LINE1=\"网络连接\" LINE2=\"异常\" LINE3=\" \" LINE4=\" \" # 上位机发起来支付请求时，网络异常LED显示\r\n"
"11=COLOR=RED LINE1=\"扫码超时\" LINE2=\"请缴现金\" LINE3=\" \" LINE4=\" \" SOUND=\"扫码超时，交易失败，请缴现金\"  VOL=4  # 上位机发起来支付请求时，网络异常LED显示\r\n"
"12=COLOR=RED LINE1=\"交易超时\" LINE2=\"请缴现金\" LINE3=\" \" LINE4=\" \" SOUND=\"交易超时，交易失败，请缴现金\"  VOL=4  # 上位机发起来支付请求时，网络异常LED显示\r\n"
"13=SOUND=\"还未发起缴费操作，无需展示二维码\"  VOL=4  # 无交易流程中的语音扫码播报提示\r\n"
"14=SOUND=\"当前状态不支持扫码操作\"  VOL=4  # 流程异常，不允许扫码播报\r\n"
;

const char *default_lane_config = 
"pay_type=1 # 缴费系统对接类型 0: 不接缴费机 1: 新疆缴费机类型 2: 德亚缴费机，3: 四川缴费机\r\n"
"scan_timeout=40 # 扫码超时，当交易发起后，扫码时间内没有扫码，交易自动取消，单位秒\r\n"
"pay_timeout=120  #交易超时时间，超过该时间，交易自动取消，单位秒\r\n"
"ping_addr=www.baidu.com # ping检测网络状态的检测地址\r\n"
"has_led=yes # 是否存在费显\r\n"
"led=/dev/ttyAMA3\r\n"
"has_qr=no\r\n"
"time_zone=CST-8		# 时区配置\r\n"
"qr=127.0.0.1 #正常情况下，二维码数据通过费显转发，如果使用网络链接扫码枪，这里需要配置扫码枪IP\r\n"
;

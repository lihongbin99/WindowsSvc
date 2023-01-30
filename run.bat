chcp 65001

rc main.rc
cl.exe /EHsc /FeC:\app\WindowsSvc\MyWindowsSvc.exe main.cpp main.res /link advapi32.lib

sc create TestWindowsSvcInCreate binPath="C:\app\WindowsSvc\MyWindowsSvc.exe C:\Users\Lee\Documents\code\cpp\WindowsServer\test.exe" start=auto DisplayName=TestWindowsSvcInDisplay
sc start TestWindowsSvcInCreate

sc stop TestWindowsSvcInCreate
sc delete TestWindowsSvcInCreate

sc create ServerFirewallSvc binPath="C:\app\WindowsSvc\MyWindowsSvc.exe C:\app\server-firewall\server-firewall.exe" start=auto DisplayName=ServerFirewallSvc
sc create SipClientSvc      binPath="C:\app\WindowsSvc\MyWindowsSvc.exe C:\app\sip\sip_client.exe"                  start=auto DisplayName=SipClientSvc
sc create FrpcSvc           binPath="C:\app\WindowsSvc\MyWindowsSvc.exe C:\app\frp\frpc.exe"                        start=auto DisplayName=FrpcSvc

sc start ServerFirewallSvc
sc start SipClientSvc
sc start FrpcSvc

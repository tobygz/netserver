# netserver

a simple c++ socket server with package parse and deal
you can write a server with it easily.


you can run it with 

make;<br>
./nets<br>
go run cli.go<br>

you may get the output from nets as following

###### 1541826788.1 [QPS]:  [type: network count: 761831 size: 14474789(13.80)] [type: mainloop count: 66652 size: 116486] [type: readfd count: 1114 size: 19223] [online: 500]<br>
###### 1541826789.1 [QPS]:  [type: network count: 803722 size: 15270718(14.56)] [type: mainloop count: 104167 size: 196869] [type: readfd count: 2098 size: 22418] [online: 500]<br>
###### 1541826790.1 [QPS]:  [type: network count: 778230 size: 14786370(14.10)] [type: mainloop count: 116474 size: 222096] [type: readfd count: 2566 size: 21778] [online: 500]<br>
###### 1541826791.1 [QPS]:  [type: network count: 845161 size: 16058059(15.31)] [type: mainloop count: 106946 size: 212344] [type: readfd count: 499 size: 18172] [online: 500]<br>
###### 1541826792.1 [QPS]:  [type: network count: 751972 size: 14287468(13.62)] [type: mainloop count: 123172 size: 252910] [type: readfd count: 2491 size: 20188] [online: 500]<br>
###### 1541826793.1 [QPS]:  [type: network count: 788312 size: 14977928(14.28)] [type: mainloop count: 109820 size: 231850] [type: readfd count: 1817 size: 19799] [online: 500]<br>
###### 1541826794.1 [QPS]:  [type: network count: 808220 size: 15356180(14.64)] [type: mainloop count: 109827 size: 232455] [type: readfd count: 558 size: 15729] [online: 500]<br>
###### 1541826795.1 [QPS]:  [type: network count: 786706 size: 14947414(14.25)] [type: mainloop count: 116922 size: 251606] [type: readfd count: 1138 size: 16874] [online: 500]<br>
###### 1541826796.1 [QPS]:  [type: network count: 767243 size: 14577617(13.90)] [type: mainloop count: 118407 size: 249698] [type: readfd count: 422 size: 13846] [online: 500]<br>
###### 1541826797.1 [QPS]:  [type: network count: 739423 size: 14049037(13.40)] [type: mainloop count: 111320 size: 235483] [type: readfd count: 1846 size: 16364] [online: 500]<br>
###### 1541826798.1 [QPS]:  [type: network count: 761075 size: 14460425(13.79)] [type: mainloop count: 119399 size: 245286] [type: readfd count: 1302 size: 15812] [online: 500]<br>
###### 1541826799.1 [QPS]:  [type: network count: 757705 size: 14396395(13.73)] [type: mainloop count: 119046 size: 259174] [type: readfd count: 1657 size: 16656] [online: 500]<br>
###### 1541826800.1 [QPS]:  [type: network count: 735733 size: 13978927(13.33)] [type: mainloop count: 114592 size: 248598] [type: readfd count: 1424 size: 16423] [online: 500]<br>
###### 1541826801.1 [QPS]:  [type: network count: 770097 size: 14631843(13.95)] [type: mainloop count: 113059 size: 234780] [type: readfd count: 847 size: 14406] [online: 500]<br>
###### 1541826802.2 [QPS]:  [type: network count: 740591 size: 14071229(13.42)] [type: mainloop count: 121561 size: 265848] [type: readfd count: 1073 size: 15447] [online: 500]<br>
###### 1541826803.2 [QPS]:  [type: network count: 801837 size: 15234903(14.53)] [type: mainloop count: 117931 size: 251175] [type: readfd count: 30 size: 9010] [online: 500]<br>
###### 1541826804.2 [QPS]:  [type: network count: 750341 size: 14256479(13.60)] [type: mainloop count: 117411 size: 259821] [type: readfd count: 936 size: 14689] [online: 500]<br>
###### 1541826805.3 [QPS]:  [type: network count: 758174 size: 14405306(13.74)] [type: mainloop count: 152302 size: 325694] [type: readfd count: 2469 size: 16929] [online: 500]<br>


Intel(R) Xeon(R) CPU E5-2620 v3 @ 2.40GHz * 8， run both server & client.

ps:  someone said, it's impossible to deal a packet in 0.1ms, it's impossible.

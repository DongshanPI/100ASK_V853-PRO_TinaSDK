﻿A. Feature

The following steps demonstrate Realtek Wireless Adapter Linux RF HW/SW Tx Tool.
The HW Tx Mode only support the new VHT chipset,like as 8814A, 8822B, 8821B, .....
It had depend on with  linux wlan driver version,must be over the v4.3.24_r15693.
This is a simple User guide,
The rtwpriv V3 version is Integrate HW/SW Tx tool.
We use Linux utility “rtwpriv” to I/O control config WLAN driver,

B. Software Package
	1. Readme.txt
	2. Rtw_Config.txt
	3. rtwpriv.cpp/rtwpriv.h
	4. rtwpriv

C. Build the realtek RF Hw Tx tool.
	Q. How to build rtwpriv tool?
    A.[Linux]
        Just "make", and you will get executable file "rtwpriv".
			If you want to use another OS platform, please to modify the Makefile of toolchain.

D. Use Example:
1. insert the wlan.ko driver.
#insmod wlan.ko

2. enable wlan interface
#ifconfig wlan0 up

3. Enter wlan MP mode
#rtwpriv wlan0 mp_start

3. execute the rtwpriv tool to launch RF HW Tx.
 rtwpriv wlan0 [Channel] [Bandwidth] [ANT_PAH] [RateID] [TxMode] [Packet Interval] [PacketLength] [Packet Count] [Packet Pattern]
#rtwpriv wlan0 36 0 a HTMCS0 1		//Start Tx
#rtwpriv wlan0 stop		       //Stop Tx

4. To adjust the Tx power index ,
   if you want to adjust [CONTINUOUS Tx] power, please frist to stop Tx,then do adjust power index.
#rtwpriv wlan0 mp_txpower patha=30,pathb=30,pathc=30,pathd=30

Instruction Command format:

#####[ HW TX mode ] VHT IC Support Only(8822B, 8821C)#####
	 Input the parameters like this:rtwpriv wlan0 [Channel] [Bandwidth] [ANT_PAH] [RateID] [TxMode] [Packet Interval] [PacketLength] [Packet Count] [Packet Pattern]
	[Channel]: 1~167
	[BW]: 0 = 20M, 1 = 40M, 2 = 80M
	[ANT_PAH]: a: PATH A, b: PATH B, c: PATH C, d: PATH D, ab : PATH AB 2x2 ....
	[RateID]:
1M 2M 5.5M 11M 6M 9M 12M 18M 24M 36M 48M 54M
HTMCS0 HTMCS1 HTMCS2 HTMCS3 HTMCS4 HTMCS5 HTMCS6 HTMCS7 HTMCS8 HTMCS9 HTMCS10
HTMCS11 HTMCS12 HTMCS13 HTMCS14 HTMCS15 HTMCS16 HTMCS17 HTMCS18 HTMCS19 HTMCS20 HTMCS21
HTMCS22 HTMCS23 HTMCS24 HTMCS25 HTMCS26 HTMCS27 HTMCS28 HTMCS29 HTMCS30 HTMCS31 VHT1MCS0
VHT1MCS1 VHT1MCS2 VHT1MCS3 VHT1MCS4 VHT1MCS5 VHT1MCS6 VHT1MCS7 VHT1MCS8 VHT1MCS9 VHT2MCS0 VHT2MCS1
VHT2MCS2 VHT2MCS3 VHT2MCS4 VHT2MCS5 VHT2MCS6 VHT2MCS7 VHT2MCS8 VHT2MCS9 VHT3MCS0 VHT3MCS1 VHT3MCS2
VHT3MCS3 VHT3MCS4 VHT3MCS5 VHT3MCS6 VHT3MCS7 VHT3MCS8 VHT3MCS9 VHT4MCS0 VHT4MCS1 VHT4MCS2 VHT4MCS3
VHT4MCS4 VHT4MCS5 VHT4MCS6 VHT4MCS7 VHT4MCS8 VHT4MCS9
	[TxMode]: 1: PACKET Tx 2:CONTINUOUS TX 3:OFDM Single Tone TX
	[Packet Interval] (Option): 1~65535 us,default 100
	[PacketLength] (Option): Packet of payload data length, default 1500
	[Packet Count] (Option): count the number of packet to Tx , set 0 for CONTINUOUS Packet TX ,default 0
	[Packet Pattern] (Option): 00~ff(hex) ,default random hex.


#####[ SW TX mode ]#####
	 Input the parameters like this: rtwpriv wlan0 mp_XXX XXX
	1. mp_start				## enter to MP Mode
	2. mp_bandwidth 40M=[Num]		## input [Num], 0=20M, 1=40M, 2=80M
	3. mp_channel [Num]			## input [Num] = 1~167 channel number
	4. mp_rate [Rate Id Num] or [RateID]
	5. mp_ant_tx [Path]			## input Antenna [PARH] = a,b,c,d,ab,ac,ad...
	6. mp_txpower patha=[index],pathb=[index],pathc=[index],pathd=[index]	## input power index range [index] = 1~63
	7. mp_get_txpower [PATH Num]		## Input Antenna PATH Num 0~3 , A = 0, B = 1 , C = 2 , D = 3
	8. mp_ctx [Tx Mode]			##input [ Tx Mode ]
				[a].Continuous Packet Tx = 'background,pkt'
				[b].Continuous Tx = 'background'
				[c].Count Packet Tx = 'count=[num],pkt'
				[d].Carrier suppression testing = 'background,cs'
				[e].Single Tone TX testing = 'background,stone'
	   mp_ctx stop				## Stop Tx Mode
	9. mp_query				## Get Tx/Rx Packet Counter.

		#####[ RX mode ] #####
	1. mp_start				## enter to MP Mode
	2. mp_bandwidth 40M=[Num]		## input [Num], 0=20M, 1=40M, 2=80M
	3. mp_channel [Num]			## input [Num] = 1~167 channel number
	5. mp_ant_rx [Path]			## input Antenna [PARH] = a,b,c,d,ab,ac,ad...
	6. mp_arx start				## Enter Rx packet mode
	7. mp_arx phy				## Query Rx Phy Packet Count
	8. mp_query				## Get Tx/Rx Packet Counter.


		#####[ SW TX mode ]#####
	 Input the parameters like this: rtwpriv wlan0 mp_XXX XXX
	1. mp_start				## enter to MP Mode
	2. mp_bandwidth 40M=[Num]		## input [Num], 0=20M, 1=40M, 2=80M
	3. mp_channel [Num]			## input [Num] = 1~167 channel number
	4. mp_rate [Rate Id Num] or [RateID]
	5. mp_ant_tx [Path]			## input Antenna [PARH] = a,b,c,d,ab,ac,ad...
	6. mp_txpower patha=[index],pathb=[index],pathc=[index],pathd=[index]	## input power index range [index] = 1~63
	7. mp_get_txpower [PATH Num]		## Input Antenna PATH Num 0~3 , A = 0, B = 1 , C = 2 , D = 3
	8. mp_ctx [Tx Mode]			##input [ Tx Mode ]
				[a].Continuous Packet Tx = 'background,pkt'
				[b].Continuous Tx = 'background'
				[c].Count Packet Tx = 'count=[num],pkt'
				[d].Carrier suppression testing = 'background,cs'
				[e].Single Tone TX testing = 'background,stone'
	   mp_ctx stop				## Stop Tx Mode
	9. mp_query				## Get Tx/Rx Packet Counter.

		#####[ RX mode ] #####
	1. mp_start				## enter to MP Mode
	2. mp_bandwidth 40M=[Num]		## input [Num], 0=20M, 1=40M, 2=80M
	3. mp_channel [Num]			## input [Num] = 1~167 channel number
	5. mp_ant_rx [Path]			## input Antenna [PARH] = a,b,c,d,ab,ac,ad...
	6. mp_arx start				## Enter Rx packet mode
	7. mp_arx phy				## Query Rx Phy Packet Count
	8. mp_query				## Get Tx/Rx Packet Counter.
4. How to get tool version number
#./rtwpriv -v
rtwpriv version:V5.1_20180307

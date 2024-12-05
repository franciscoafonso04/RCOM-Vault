# EXP3 - Y = 12

## Dados finais

| TUX | IP             | MAC               | ETHER | ETH |
| --- | -------------- | ----------------- | ----- | --- |
| 2   | 172.16.121.1   | 00:c0:df:10:90:56 | 9     | 1   |
| 3   | 172.16.120.1   | 00:08:54:50:31:bc | 7     | 1   |
| 4   | 172.16.120.254 | 00:e0:7d:b4:b8:94 | 8     | 1   |
| 4   | 172.16.121.253 | 00:08:54:57:fa:89 | 10    | 2   |

## Procedimento

### Cabos 

1. Conectar `tuxY3 E1` a `Switch ether7`
2. Conectar `tuxY4 E1` a `Switch ether8`
3. Conectar `tuxY2 E1` a `Switch ether9`
4. Conectar `tuxY4 E2` a `Switch ether10`
5. Conectar `tuxY3 S0` a `RS232 -> cisco`
6. Conectar `Switch MTIK Console` a `cisco -> RS232`

### tuxY3

```
systemctl restart networking

ifconfig eth1 up

ifconfig eth1 172.16.Y0.1/24
```

### tuxY4

```
systemctl restart networking

ifconfig eth1 up

ifconfig eth1 172.16.Y0.254/24

ifconfig eth2 up

ifconfig eth2 172.16.Y1.253/24

sysctl net.ipv4.ip_forward=1

sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
```

### tuxY2

```
systemctl restart networking

ifconfig eth1 up

ifconfig eth1 172.16.Y1.1/24
```

### MicroTik Switch

1. Abrir GTKTerm no tuxY3
2. Configuration -> Port -> Baudrate = 115200
3. `ENTER`

```
/system reset-configuration

username: admin
pass: 

/interface bridge host print

/interface bridge add name=bridgeY0
/interface bridge add name=bridgeY1

/interface bridge port remove [find interface=ether7]
/interface bridge port remove [find interface=ether8]
/interface bridge port remove [find interface=ether9]
/interface bridge port remove [find interface=ether10]

/interface bridge port add bridge=bridgeY0 interface=ether7
/interface bridge port add bridge=bridgeY0 interface=ether8
/interface bridge port add bridge=bridgeY1 interface=ether9
/interface bridge port add bridge=bridgeY1 interface=ether10

/interface bridge host print
```

### tuxY2

```
route add -net 172.16.Y0.0/24 gw 172.16.Y1.253
```

### tuxY3

```
route add -net 172.16.Y1.0/24 gw 172.16.Y0.254
```

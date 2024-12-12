# EXP4 - Y = 10

## Dados finais

| TUX | IP             | MAC               | ETHER | ETH |
| --- | -------------- | ----------------- | ----- | --- |
| 2   | 172.16.101.1   | 00:c0:df:13:20:1d | 9     | 1   |
| 3   | 172.16.100.1   | 00:c0:df:13:20:10 | 5     | 1   |
| 4   | 172.16.100.254 | 00:c0:df:25:43:bc | 6     | 1   |
| 4   | 172.16.101.253 | 00:01:02:9f:7e:9c | 10    | 2   |
| RC  | 172.16.101.254 |                   | 11    |     |

## Procedimento

### Cabos 

1. Conectar `tuxY3 E1` a `Switch ether5`
2. Conectar `tuxY4 E1` a `Switch ether6`
3. Conectar `tuxY2 E1` a `Switch ether9`
4. Conectar `tuxY4 E2` a `Switch ether10`
5. Conectar `tuxY3 S0` a `RS232 -> cisco`
6. Conectar `MTIK Switch Console` a `cisco -> RS232`
7. Conectar `Router MTIK ether1` a `PY.12`
8. Conectar `Router MTIK ether2` a `Switch MTIK ether11`

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

/interface bridge port remove [find interface=ether5]
/interface bridge port remove [find interface=ether6]
/interface bridge port remove [find interface=ether9]
/interface bridge port remove [find interface=ether10]
/interface bridge port remove [find interface=ether11]

/interface bridge port add bridge=bridgeY0 interface=ether5
/interface bridge port add bridge=bridgeY0 interface=ether6
/interface bridge port add bridge=bridgeY1 interface=ether9
/interface bridge port add bridge=bridgeY1 interface=ether10
/interface bridge port add bridge=bridgeY1 interface=ether11

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

### Cabos

1. Desconectar `Switch MTIK Console` de `cisco -> RS232`
2. Conectar `Router MTIK` a `cisco -> RS232`


### MicroTik Router

1. Abrir GTKTerm no tuxY3
2. Configuration -> Port -> Baudrate = 115200
3. `ENTER`

```
/system reset-configuration

username: admin
pass: 

/ip address add address=172.16.1.Y9/24 interface=ether1
/ip address add address=172.16.Y1.254/24 interface=ether2
```

### tuxY2

```
route add default gw 172.16.Y1.254
```

### tuxY3

```
route add default gw 172.16.Y0.254
```

### tuxY4

```
route add default gw 172.16.Y1.254
```

### MicroTik Router

```
/ip route add dst-address=172.16.Y0.0/24 gateway=172.16.Y1.253
/ip route add dst-address=0.0.0.0/0 gateway=172.16.1.254
```
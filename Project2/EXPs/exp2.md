# EXP2 - Y = 11

## Dados finais

| TUX | IP             | MAC               | ETHER | ETH |
| --- | -------------- | ----------------- | ----- | --- |
| 2   | 172.16.111.1   | 00:c0:df:08:d5:98 | 12    | 1   |
| 3   | 172.16.110.1   | 00:08:54:50:35:0a | 6     | 1   |
| 4   | 172.16.110.254 | 00:08:54:71:73:ed | 5     | 1   |

## Procedimento

### Cabos 

1. Conectar `tuxY3 E1` a `Switch ether6`
2. Conectar `tuxY4 E1` a `Switch ether5`
3. Conectar `tuxY2 E1` a `Switch ether12`
4. Conectar `tuxY3 S0` a `RS232 -> cisco`
5. Conectar `Switch MTIK Console` a `cisco -> RS232`

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
/interface bridge port remove [find interface=ether12]

/interface bridge port add bridge=bridgeY0 interface=ether5
/interface bridge port add bridge=bridgeY0 interface=ether6
/interface bridge port add bridge=bridgeY1 interface=ether12

/interface bridge host print
```



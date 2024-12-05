# EXP1 - Y = 11

## Dados finais

| TUX | IP             | MAC               | ETHER | ETH |
| --- | -------------- | ----------------- | ----- | --- |
| 3   | 172.16.110.1   | 00:08:54:50:35:0a | 6     | 1   |
| 4   | 172.16.110.254 | 00:08:54:71:73:ed | 5     | 1   |

## Procedimento

### Cabos 

1. Conectar `tuxY3 E1` a `Switch ether6`
2. Conectar `tuxY4 E1` a `Switch ether5`

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

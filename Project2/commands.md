# Commands to run when we get to the lab - bancada 11

## Cabos

- gnu62 E1 -> ether12
- gnu63 E1 -> ether5
- gnu64 E1 -> ether6
- gnu64 E2 -> ether11

- gnu63 S0 -> `RS232 -> cisco`
- MicroTik Console -> `cisco -> RS232`

## TUX3

```c
// reset da net
systemctl restart networking

// configurar a conexão
sudo ifconfig eth1 172.16.110.1 netmask 255.255.255.0 up

sudo route add default gw 172.16.110.254 eth1

// verificar se correu bem
ifconfig eth1

route -n

ping 172.16.110.254

// ler ip e mac address
ifconfig eth1

// registar ip e mac address do TUX4
sudo arp -s 172.16.110.254 00:08:54:71:73:ed

// verificar se correu bem
arp -n

// depois das bridges configuradas

ping 172.16.100.254 // TUX 4
ping 172.16.101.1   // TUX 2
```

## TUX4

```c
// reset da net
systemctl restart networking

// configurar a conexão
ifconfig eth1 172.16.110.254 netmask 255.255.255.0 up

route add default gw 172.16.110.1 eth1

// verificar se correu bem
ifconfig eth1

route -n

ping 172.16.110.1

// ler ip e mac address
ifconfig eth1

// registar ip e mac address do TUX3
arp -s 172.16.110.1   00:08:54:50:35:0a

// verificar se correu bem
arp -n

//EXP3
// configurar a conexão
ifconfig eth2 172.16.111.253 netmask 255.255.255.0 up


```

## TUX2

```c
// reset da net
systemctl restart networking

// configurar a conexão
ifconfig eth1 172.16.111.1 netmask 255.255.255.0 up

// ler ip e mac address
ifconfig eth1

// registar ip e mac address
arping -I eth1 172.16.101.254 // sugestão do chat gpt once again
```

## GTKTERM

Para aceder o terminal do GTKTerm devemos:
1. `Configuration`
2. `Port` 
3. Baud Rate -> `115200` 
4. Fechar port configuration
5. `ENTER`

```c
// verificar as bridges que existem
/interface bridge print

// criar bridges
/interface bridge add name=bridge110
/interface bridge add name=bridge111

// ver a conexão mac address to ether (podemos ver fisicamente no switch)
/interface bridge host print

// remover os TUX's da default bridge
/interface bridge port remove [find interface=ether12] // TUX2
/interface bridge port remove [find interface=ether6]  // TUX3
/interface bridge port remove [find interface=ether5]  // TUX4

//adicionar à bridge correta
/interface bridge port add bridge=bridge111 interface=ether12 // TUX2
/interface bridge port add bridge=bridge110 interface=ether6
/interface bridge port add bridge=bridge110 interface=ether5
```

TUX2 172.16.111.1   00:c0:df:08:d5:98 ether12
TUX3 172.16.110.1   00:08:54:50:35:0a ether6
TUX4 172.16.110.254 00:08:54:71:73:ed ether5
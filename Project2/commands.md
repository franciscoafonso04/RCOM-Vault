# Commands to run when we get to the lab

## Cabos

- gnu22 -> ether4
- gnu23 -> ether5
- gnu24 -> ether6

## TUX3

```c
// reset da net
systemctl restart networking

// configurar a conexão
sudo ifconfig eth1 172.16.100.1 netmask 255.255.255.0 up

sudo route add default gw 172.16.100.254 eth1

// verificar se correu bem
ifconfig eth1

route -n

ping 172.16.100.254

// ler ip e mac address
ifconfig eth1

// registar ip e mac address to TUX4
sudo arp -s 172.16.100.254 <MAC:ADDR:TUX4>

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
sudo ifconfig eth1 172.16.100.254 netmask 255.255.255.0 up

sudo route add default gw 172.16.100.1 eth1

// verificar se correu bem
ifconfig eth1

route -n

ping 172.16.100.1

// ler ip e mac address
ifconfig eth1

// registar ip e mac address to TUX3
sudo arp -s 172.16.100.1 <MAC:ADDR:TUX3>

// verificar se correu bem
arp -n
```

## TUX2

```c
// reset da net
systemctl restart networking

// configurar a conexão
sudo ifconfig eth1 172.16.101.1 netmask 255.255.255.0 up

sudo route add default gw 172.16.101.254      // o chatgpt diz para fazer isto, not sure tho
sudo route add default gw 172.16.101.254 eth1 // talvez isto seja um pouco melhor?

// ler ip e mac address
ifconfig eth1

// registar ip e mac address
sudo arping -I eth1 172.16.101.254 // sugestão do chat gpt once again
```

## GTKTERM

Para aceder o terminal do GTKTerm (TUX2) devemos:
1. `Configuration`
2. `Port` 
3. Baud Rate -> `115200` 
4. Fechar port configuration
5. `ENTER`

```c
// verificar as brifges que existem
/interface bridge print

// criar bridges
/interface bridge add name=bridge100
/interface bridge add name=bridge101

// ver a conexão mac address to ether (podemos ver fisicamente no switch)
/interface bridge host print

// remover os TUX's da default bridge
/interface bridge port remove [find interface=ether4]
/interface bridge port remove [find interface=ether5]
/interface bridge port remove [find interface=ether6]

//adicionar à bridge correta
/interface bridge port add bridge=bridge101 interface=ether4 // TUX2
/interface bridge port add bridge=bridge100 interface=ether5
/interface bridge port add bridge=bridge100 interface=ether6
```
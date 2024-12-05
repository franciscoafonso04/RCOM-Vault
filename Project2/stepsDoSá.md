## Experience 1 - Configurate an IP Network

> 1. Conectar E0 do Tux103 e do Tux104 numa das entradas do switch;
> 2. Aceder a cada um dos computadores e configurar os seus IPs:
> 
>     ```bash
>     $ ifconfig eth1 up
>     $ ifconfig eth1 <IP>
>         - 172.16.100.1/24 para o Tux103
>         - 172.16.100.254/24 para o Tux104
>     ```
> 
> 3. Para ver o MAC address de cada computador, consultar o campo `ether` no comando `ipconfig`;
> 4. Verificar a conexão dos dois computadores na mesma rede. Em cada um tenta enviar pacotes de dados para outro. A conexão está correcta quando todos os pacotes são recebidos:
> 
>     ```bash
>     $ ping <IP>
>         - 172.16.100.254 para o Tux103
>         - 172.16.100.1 para o Tux104
>     ```
> 
> 5. Avaliar a tabela *Address Resolution Protocol* (ARP) do Tux103. Deverá ter uma única entrada com o IP e o MAC do Tux104. Para visualizar a tabela:
> 
>     ```bash
>     $ arp -a # ?(172.16.100.254) at 00:08:54:71:73:ed [ether] on eth1
>     ```
> 
> 6. Apagar a entrada da tabela ARP do Tux103:
> 
>     ```bash
>     $ arp -d 172.16.100.254/24
>     $ arp -a # empty
>     ```
> 
> 7. Abrir o WireShark no Tux53 e começar a capturar pacotes de rede;
> 8. Executar o seguinte comando no Tux53:
> 
>     ```bash
>     $ ping 172.16.100.254/24
>     ```
> 
> 9. Parar a captura dos resultados e avaliar os logs obtidos com o WireShark.

## Experience 2 - Implement two bridges in a switch

> 1. Conectar Tux102_S0 em T3;
> 2. Conectar o T4 no CONSOLE do switch;
> 3. Abrir o GKTerm no Tux103 e configurar a baudrate para 115200;
> 4. Resetar as configurações do switch com o seguinte comando:
> 
>     ```bash
>     > admin
>     > /system reset-configuration
>     > y
>     ```
> 
> 5. Conectar o Tux102_E0 ao switch (porta 3) e configurar a ligaçao com os seguintes comandos:
> 
>     ```bash
>     $ ifconfig eth1 up
>     $ ifconfig eth1 172.16.101.1/24
>     ```
> 
> 6. Criar 2 bridges no switch
> 
>     ```bash
>     > /interface bridge add name=bridge100
>     > /interface bridge add name=bridge101
>     ```
> 
> 7. Eliminar as portas as quais o Tux102, 103 e 104 estao ligados por defeito
> 
>     ```bash
>     > /interface bridge port remove [find interface=ether1] 
>     > /interface bridge port remove [find interface=ether2] 
>     > /interface bridge port remove [find interface=ether3] 
>     ```
> 
> 8. Adicionar as novas portas
> 
>     ```bash
>     > /interface bridge port add bridge=bridge100 interface=ether5
>     > /interface bridge port add bridge=bridge100 interface=ether6 
>     > /interface bridge port add bridge=bridge101 interface=ether11
>     ```
> 
> 9. Verificar que as portas foram adicionadas corretamente com o commando :
> 
>     ```bash
>     > /interface bridge port print
>     ```
> 
> 10. Começar a captura do eth1 do Tux103
> 
> 11. Desde o Tux103, começar a captura do eth0 e fazer ping do Tux104 e Tux102
>     ```bash
>     $ ping 172.16.100.254
>     #Tux55 -> Tudo ok
>     $ ping 172.16.101.1
>     #Tux52 -> connect: Network is unreachable
>     ``` 
> 
> 12. Começar capturas do eth0 no Tux52, Tux53 e Tux54. No Tux53, executar o seguinte comando e observar e guardar os resultados:
>     ```bash
>     $ ping -b 172.16.50.255
>     ``` 
> 
> 13. Começar capturas do eth0 no Tux52, Tux53 e Tux54. No Tux52, executar o seguinte comando e observar e guardar os resultados:
>     ```bash
>     $ ping -b 172.16.51.255
>  

## Experience 3 - Configure a Router in Linux

>## Steps
>
> 1. Ligar eth2 do Tux104 à porta 4 do switch. Configurar eth2 do Tux104
> ```bash
>     ifconfig eth2 up
>     ifconfig eth2 172.16.101.253/24
> ```
> 
> 2. Eliminar as portas as quais o Tux102 esta ligado por defeito e adicionar a nova porta
> ```bash
>     /interface bridge port remove [find interface=ether4]
>     /interface bridge port add bridge=bridge101 interface=ether4
> ```
> 
> 3. Ativar *ip forwarding* e desativar ICMP no Tux54
> ```bash
>     #3 Ip forwarding t4
>     sysctl net.ipv4.ip_forward=1
> 
>     #4 Disable ICMP echo ignore broadcast T4
>     sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
> ```
>
> 4. Observar MAC e Ip no Tux104eth1 e Tux104eth2
> ```bash
>     ifconfig
>     ##Mac eth1 -> a definir
>     ##ip eth1-> a definir
>     ##Mac eth2 -> a definir
>     ##ip eth2-> a definir
> ```
> 
> 5. Adicionar as seguintes rotas no Tux102 e Tux103 para que estes se consigam altançar um ou outro através do Tux54
> ```bash
>     route add -net  172.16.100.0/24 gw 172.16.101.253 # no Tux102
>     route add -net  172.16.101.0/24 gw 172.16.50.254 # no Tux103
> ```
> 
> 6. Verificar as rotas em todos os Tux com o seguinte comando:
> ```bash
>     route -n
> ```
> 
> 7. Começar captura do Tux103 no Wireshark
> 
> 8. Fazer ping aos seguintes endereços desde o Tux103
> ```bash
>     ping 172.16.100.254
>     ping 172.16.101.253
>     ping 172.16.101.1
>     ##All good, network is correct
> ```
> 
> 9. Parar a captura e guardar os resultados
> 
> 10. Começar captura do eth1 e eth2 do Tux104 no Wireshark
> 
> 11. Limpar as tabelas ARP em todos os Tux
> ```bash
>     arp -d 172.16.101.253 #Tux52
>     arp -d 172.16.100.254 #Tux53
>     arp -d 172.16.100.1 #Tux54
>     arp -d 172.16.101.1 #Tux54
> ```
> 
> 12. Fazer ping do Tux102 desde o Tux103
> 
> 13. Parar a captura e guardar os resultados
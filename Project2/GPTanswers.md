1. What are ARP packets, and what are they used for?
ARP (Address Resolution Protocol) packets are used to map IP addresses (logical) to MAC addresses (physical) within a Local Area Network (LAN). When a device wants to communicate with another device on the same network, it uses ARP to find out the MAC address associated with the IP address.

---

2. What are the MAC and IP addresses of ARP packets, and why?
ARP Request Packet:

MAC Address (Source): The MAC address of the device making the request.
MAC Address (Destination): Usually ff:ff:ff:ff:ff:ff (broadcast), as the sender does not know the recipient’s MAC.
IP Address (Source): The IP of the requesting device.
IP Address (Destination): The IP of the target device.
ARP Reply Packet:

MAC Address (Source): The MAC address of the responding device.
MAC Address (Destination): The MAC address of the requesting device.
IP Address (Source): The IP of the responding device.
IP Address (Destination): The IP of the requesting device.
This structure ensures the packet reaches the correct device and that ARP works efficiently.

---

3. What packets does the ping command generate?
The ping command generates ICMP (Internet Control Message Protocol) packets, specifically:

Echo Request: Sent from the source device to the target device.
Echo Reply: Sent from the target device back to the source device.

---

4. What are the MAC and IP addresses of ping packets?
Echo Request:

MAC Address (Source): The MAC of the device sending the ping.
MAC Address (Destination): The MAC of the target device.
IP Address (Source): The IP of the sending device.
IP Address (Destination): The IP of the target device.
Echo Reply:

The MAC and IP addresses are swapped between source and destination compared to the Echo Request.

---

5. How to determine if a receiving Ethernet frame is ARP, IP, or ICMP?
Check the Ethertype field in the Ethernet header:
0x0806: ARP packet.
0x0800: IP packet.
If it’s an IP packet, check the Protocol field in the IP header:
1: ICMP (used by ping).

---

6. How to determine the length of a receiving frame?
The length of an Ethernet frame can be determined by:

Checking the total capture size of the frame in Wireshark.
Looking at the "Length" field in the Ethernet frame header, which specifies the payload size.

---

7. What is the loopback interface, and why is it important?
The loopback interface (commonly associated with the IP address 127.0.0.1) is a virtual interface used for testing and diagnostics on the same device. It ensures that the IP stack is correctly configured and functioning without requiring external network connectivity.

---

## Experience 2

1. How to configure bridgeY0?

To configure bridgeY0 on the Mikrotik switch:

Create the bridge using the following command:

```/interface bridge add name=bridgeY0```

Add ports to the bridge:

```/interface bridge port add bridge=bridgeY0 interface=<port>```

Replace <port> with the actual port number, e.g., ether1, ether2.

Verify the bridge and its ports:

```
/interface bridge print
/interface bridge port print
```

2. How many broadcast domains are there? How can you conclude it from the logs?

Number of Broadcast Domains: The number of broadcast domains corresponds to the number of bridges configured. In this case, there are two broadcast domains: one for bridgeY0 and another for bridgeY1.

Conclusion from Logs:
Examine the logs captured during the broadcast ping test (ping -b) on both tuxY3 and tuxY2.
Packets in each domain will only reach devices connected to that specific bridge. This isolation confirms the presence of separate broadcast domains.
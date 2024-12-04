## Summary

The project revolves around developing a simple FTP client and configuring and analyzing a network. It is divided into two main parts:

### Part 1: FTP Client Development

- Implement an FTP client in C following RFC959 (FTP) and RFC1738 (URL syntax).
- The client connects to a server, downloads a file, and demonstrates understanding of TCP sockets, DNS, and FTP protocol behavior.

### Part 2: Network Configuration and Analysis

- Set up and analyze a computer network through practical experiments involving ARP, routing, packet capturing (Wireshark), and NAT.
- Key tasks include bridge configuration, Linux-based routing, packet inspection (ICMP, ARP, TCP), and understanding DNS and TCP congestion control.

The objective is to enhance understanding of network protocols, their interactions, and practical implementation in real-world scenarios.

# FTP Client Development

# Network COnfiguration and Analysis

## Experience 1 - Configuring an IP Network

This experiment focuses on configuring a simple IP network and analyzing how devices communicate using ARP and ICMP. The steps involve connecting two devices, configuring their network interfaces, verifying connectivity, and analyzing packet exchanges to understand fundamental network protocols.

### Procedures

1. Setting Up Connections:
The E0 interfaces of tuxY3 and tuxY4 are connected to a switch to establish physical connectivity.

2. Configuring IP Addresses:
The `eth0` interfaces are assigned IP addresses using `ifconfig`, e.g., 172.16.Y0.1/24 for tuxY3 and 172.16.Y0.254/24 for tuxY4.

3. Inspecting MAC and IP Mappings:
The `arp -a` command displays the ARP table, showing the mapping between IP and MAC addresses for both devices.

4. Testing Connectivity:
The `ping` command is used to verify that tuxY3 and tuxY4 can communicate. Successful ICMP Echo Request and Echo Reply messages confirm connectivity.

5. Clearing ARP Tables:
The ARP cache is cleared using `arp -d <ipaddress>` to observe ARP requests when the devices communicate again.

6. Packet Capture with Wireshark:
Wireshark captures ARP and ICMP packets on tuxY3, illustrating the packet exchange during communication.

### Key Questions

1. What are ARP packets, and what are they used for?

ARP (Address Resolution Protocol) packets are used to map IP addresses to MAC addresses within the same subnet. This mapping enables devices to communicate at the data link layer.

2. What are the MAC and IP addresses of ARP packets, and why?

- ARP Request:
    - Source MAC: tuxY3's MAC.
    - Destination MAC: Broadcast (ff:ff:ff:ff:ff:ff).
    - Source IP: tuxY3's IP.
    - Destination IP: tuxY4's IP.

- ARP Reply: 
    - Source MAC: tuxY4's MAC.
    - Destination MAC: tuxY3's MAC.
    - Source IP: tuxY4's IP.
    - Destination IP: tuxY3's IP.
    The broadcast ensures the ARP request reaches all devices  in the subnet, while the reply is directly addressed to the requester.

3. What packets does the ping command generate?

The ping command generates ICMP packets: Echo Request (from tuxY3 to tuxY4) and Echo Reply (from tuxY4 to tuxY3).

4. What are the MAC and IP addresses of the ping packets?

- Echo Request:
    - Source MAC/IP: tuxY3's MAC/IP.
    - Destination MAC/IP: tuxY4's MAC/IP.
- Echo Reply:
    - Source MAC/IP: tuxY4's MAC/IP.
    - Destination MAC/IP: tuxY3's MAC/IP.

5. How to determine if a receiving Ethernet frame is ARP, IP, or ICMP?

- Inspect the EtherType field in the Ethernet header:
    - 0x0806: ARP.
    - 0x0800: IP.
- For IP packets, check the Protocol field in the IP header:
    - 1: ICMP.

6. How to determine the length of a receiving frame?

The frame length is visible in Wireshark’s packet details or calculated from the Ethernet header.

7. What is the loopback interface, and why is it important?

The loopback interface (127.0.0.1) is a virtual network interface used for internal communication on a device. It’s crucial for testing and ensuring the TCP/IP stack functions correctly.

### Insights

This experiment highlights how ARP resolves addresses, ICMP verifies connectivity, and tools like Wireshark aid in understanding packet-level communication.

## Experience 2 - Implementing Two Bridges in a Switch

This experiment explores the creation and configuration of two distinct broadcast domains using bridges in a switch. It demonstrates how network segmentation isolates traffic between different subnets, enhancing security and reducing broadcast traffic.

### Procedures 

1. Connecting Devices:

tuxY3, tuxY4, and tuxY2 are connected to a switch. tuxY3 and tuxY4 remain in the same subnet (172.16.Y0.0/24), while tuxY2 is configured in a separate subnet (172.16.Y1.0/24).

2. Creating Bridges:

Two bridges, bridgeY0 and bridgeY1, are created on the switch using commands like `/interface bridge add name=<bridge_name>`.

- tuxY3 and tuxY4 are added to `bridgeY0`.
- tuxY2 is added to `bridgeY1`.
- Ports are removed from the default bridge and reassigned to the appropriate custom bridge using `/interface bridge port add bridge=<bridge_name> interface=<port>`.

3. Verifying Configuration:

Ping commands and Wireshark captures are used to confirm connectivity within each bridge while ensuring isolation between them.

4. Testing Broadcast Behavior:

Broadcast `ping` commands (`ping -b`) are sent from tuxY3 and tuxY2 to observe traffic reachability within each bridge.

### Key Questions

1. How to configure `bridgeY0`?

```
/interface bridge add name=bridgeY0
```

```
/interface bridge port add bridge=bridgeY0 interface=<port>
```

Verify the configuration:

```
/interface bridge print
/interface bridge port print
```

2. How many broadcast domains are there? How can you conclude it from the logs?

- Number of Broadcast Domains: Two distinct domains (bridgeY0 and bridgeY1).

- Conclusion from Logs:

Wireshark captures reveal that broadcasts sent from tuxY3 only reach tuxY4 (both in bridgeY0), while broadcasts from tuxY2 remain isolated within bridgeY1. No cross-bridge traffic is observed, confirming separate domains.

### Insights

This experiment demonstrates how bridges divide a network into isolated segments, creating separate broadcast domains. It highlights the importance of segmentation for traffic management and security in complex network environments. Wireshark provides visibility into the isolation and confirms correct bridge configurations.







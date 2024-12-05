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

## **Experience 1 - Configuring an IP Network**

This experiment focuses on configuring a simple IP network and analyzing how devices communicate using ARP and ICMP. The steps involve connecting two devices, configuring their network interfaces, verifying connectivity, and analyzing packet exchanges to understand fundamental network protocols.

---

### **Procedures**

1. **Setting Up Connections**:  
   The `E1` interfaces of tuxY3 and tuxY4 are connected to a switch to establish physical connectivity.

2. **Configuring IP Addresses**:  
   The `eth1` interfaces are assigned IP addresses using `ifconfig`, e.g., `172.16.Y0.1/24` for tuxY3 and `172.16.Y0.254/24` for tuxY4.

3. **Inspecting MAC and IP Mappings**:  
   The `arp -a` command displays the ARP table, showing the mapping between IP and MAC addresses for both devices.

4. **Testing Connectivity**:  
   The `ping` command is used to verify that tuxY3 and tuxY4 can communicate. Successful ICMP Echo Request and Echo Reply messages confirm connectivity.

5. **Clearing ARP Tables**:  
   The ARP cache is cleared using `arp -d <ipaddress>` to observe ARP requests when the devices communicate again.

6. **Packet Capture with Wireshark**:  
   Wireshark captures ARP and ICMP packets on tuxY3, illustrating the packet exchange during communication.

---

### **Key Questions**

1. **What are ARP packets, and what are they used for?**  
   ARP (Address Resolution Protocol) packets are used to map IP addresses to MAC addresses within the same subnet. This mapping enables devices to communicate at the data link layer.

2. **What are the MAC and IP addresses of ARP packets, and why?**  
   - **ARP Request**:
     - Source MAC: tuxY3's MAC.
     - Destination MAC: Broadcast (`ff:ff:ff:ff:ff:ff`).
     - Source IP: tuxY3's IP.
     - Destination IP: tuxY4's IP.
   - **ARP Reply**:
     - Source MAC: tuxY4's MAC.
     - Destination MAC: tuxY3's MAC.
     - Source IP: tuxY4's IP.
     - Destination IP: tuxY3's IP.  
   The broadcast ensures the ARP request reaches all devices in the subnet, while the reply is directly addressed to the requester.

3. **What packets does the `ping` command generate?**  
   The `ping` command generates ICMP packets: Echo Request (from tuxY3 to tuxY4) and Echo Reply (from tuxY4 to tuxY3).

4. **What are the MAC and IP addresses of the `ping` packets?**  
   - **Echo Request**:
     - Source MAC/IP: tuxY3's MAC/IP.
     - Destination MAC/IP: tuxY4's MAC/IP.
   - **Echo Reply**:
     - Source MAC/IP: tuxY4's MAC/IP.
     - Destination MAC/IP: tuxY3's MAC/IP.

5. **How to determine if a receiving Ethernet frame is ARP, IP, or ICMP?**  
   - Inspect the **EtherType** field in the Ethernet header:
     - `0x0806`: ARP.
     - `0x0800`: IP.
   - For IP packets, check the **Protocol** field in the IP header:
     - `1`: ICMP.

6. **How to determine the length of a receiving frame?**  
   The frame length is visible in Wireshark's packet details or calculated from the Ethernet header.

7. **What is the loopback interface, and why is it important?**  
   The loopback interface (`127.0.0.1`) is a virtual network interface used for internal communication on a device. It's crucial for testing and ensuring the TCP/IP stack functions correctly.

---

### **Key Insights**

This experiment highlights how ARP resolves addresses, ICMP verifies connectivity, and tools like Wireshark aid in understanding packet-level communication.

## **Experience 2 - Implementing Two Bridges in a Switch**

This experiment explores the creation and configuration of two distinct broadcast domains using bridges in a switch. It demonstrates how network segmentation isolates traffic between different subnets, enhancing security and reducing broadcast traffic.

---

### **Procedures**

1. **Connecting Devices**:  
   tuxY3, tuxY4, and tuxY2 are connected to a switch. tuxY3 and tuxY4 remain in the same subnet (`172.16.Y0.0/24`), while tuxY2 is configured in a separate subnet (`172.16.Y1.0/24`).

2. **Creating Bridges**:  
   Two bridges, `bridgeY0` and `bridgeY1`, are created on the switch using commands like `/interface bridge add name=<bridge_name>`.  
   - tuxY3 and tuxY4 are added to `bridgeY0`.  
   - tuxY2 is added to `bridgeY1`.  
   - Ports are removed from the default bridge and reassigned to the appropriate custom bridge using `/interface bridge port add bridge=<bridge_name> interface=<port>`.

3. **Verifying Configuration**:  
   Ping commands and Wireshark captures are used to confirm connectivity within each bridge while ensuring isolation between them.

4. **Testing Broadcast Behavior**:  
   Broadcast `ping` commands (`ping -b`) are sent from tuxY3 and tuxY2 to observe traffic reachability within each bridge.

---

### **Key Questions**

1. **How to configure `bridgeY0`?**  
   - Create the bridge:
     ```
     /interface bridge add name=bridgeY0
     ```
   - Assign the appropriate ports:
     ```
     /interface bridge port add bridge=bridgeY0 interface=<port>
     ```
   - Verify the configuration:
     ```
     /interface bridge print
     /interface bridge port print
     ```

2. **How many broadcast domains are there? How can you conclude it from the logs?**  
   - **Number of Broadcast Domains**: Two distinct domains (`bridgeY0` and `bridgeY1`).
   - **Conclusion from Logs**:  
     Wireshark captures reveal that broadcasts sent from tuxY3 only reach tuxY4 (both in `bridgeY0`), while broadcasts from tuxY2 remain isolated within `bridgeY1`. No cross-bridge traffic is observed, confirming separate domains.

---

### **Key Insights**

This experiment demonstrates how bridges divide a network into isolated segments, creating separate broadcast domains. It highlights the importance of segmentation for traffic management and security in complex network environments. Wireshark provides visibility into the isolation and confirms correct bridge configurations.

## **Experience 3 - Configuring a Router in Linux**

This experiment focuses on transforming a Linux-based machine (tuxY4) into a router, enabling communication between two isolated subnets. By configuring IP forwarding and appropriate routing, devices in different broadcast domains can communicate seamlessly.

---

### **Procedures**

1. **Connecting and Configuring tuxY4**:  
   - The `eth1` interface of tuxY4 is connected to the switch and configured with an IP address (e.g., `172.16.Y1.253/24`) to act as the gateway for the second subnet (`bridgeY1`).

2. **Enabling Routing on tuxY4**:  
   - IP forwarding is activated with:
     ```
     sysctl net.ipv4.ip_forward=1
     ```
   - ICMP echo broadcast ignore is disabled for testing:
     ```
     sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
     ```

3. **Updating Routing Tables**:  
   - Routes are added to tuxY3 and tuxY2 to specify tuxY4 as the gateway:
     - On tuxY3:  
       ```
       route add -net 172.16.Y1.0/24 gw 172.16.Y0.254
       ```
     - On tuxY2:  
       ```
       route add -net 172.16.Y0.0/24 gw 172.16.Y1.253
       ```

4. **Testing Connectivity**:  
   - The `ping` command is used to test communication between tuxY3 and tuxY2. Successful packet exchanges confirm routing functionality.

5. **Capturing and Analyzing Packets**:  
   - Wireshark is used on tuxY4 to capture ARP and ICMP packets, illustrating how it redirects traffic between the two subnets.

---

### **Key Questions**

1. **What routes are there in the tuxes? What is their meaning?**  
   - **Routes in tuxY3**:  
     - Default route through tuxY4 for packets destined for subnet `172.16.Y1.0/24`.
   - **Routes in tuxY2**:  
     - Default route through tuxY4 for packets destined for subnet `172.16.Y0.0/24`.
   - **Meaning**: These routes instruct tuxY3 and tuxY2 to send inter-subnet traffic to tuxY4, which handles forwarding.

2. **What information does a forwarding table entry contain?**  
   A forwarding table entry includes:
   - Destination network/subnet.
   - Gateway (next hop).
   - Network interface to use.

3. **What ARP messages and associated MAC addresses are observed, and why?**  
   - **ARP Requests**:  
     - Sent by tuxY4 to resolve the MAC address of the destination device when forwarding packets.
   - **ARP Replies**:  
     - Sent by the destination device to tuxY4.  
   These exchanges allow tuxY4 to maintain mappings between IP and MAC addresses for forwarding.

4. **What ICMP packets are observed and why?**  
   - **ICMP Echo Requests**: Sent by tuxY3 to tuxY2.  
   - **ICMP Echo Replies**: Sent by tuxY2 to tuxY3.  
   These packets confirm successful communication between the devices, with tuxY4 acting as the intermediary.

5. **What are the IP and MAC addresses associated with ICMP packets and why?**  
   - **Source IP/MAC**: The originating device (e.g., tuxY3).  
   - **Destination IP**: The final target (e.g., tuxY2).  
   - **Destination MAC**: tuxY4's MAC, as it handles packet forwarding between subnets.  
   TuxY4 rewrites the MAC addresses to route the packets correctly.

---

### **Key Insights**

This experiment demonstrates how a Linux machine can function as a router, bridging isolated subnets through IP forwarding and proper routing configuration. Packet captures confirm the router's role in ARP resolution and ICMP message handling, emphasizing its role as the intermediary for inter-subnet communication.

## **Experience 4 - Configuring a Commercial Router and Implementing NAT**

This experiment demonstrates the configuration of a commercial router to connect two isolated subnets and enable internet access using Network Address Translation (NAT). It highlights the role of NAT in translating private IP addresses to a single public IP address, enabling devices in a local network to communicate externally.

---

### **Procedures**

1. **Connecting the Router**:  
   - The router's `ether1` interface is connected to the laboratory's external network, and its `ether2` interface is connected to the switch.  
   - tuxY3 and tuxY2 remain in separate subnets connected via tuxY4 acting as an intermediate router.

2. **Configuring Router Interfaces**:  
   - IP addresses are assigned to the router's interfaces:
     - `ether1`: Public IP (e.g., `172.16.1.Y9/24`).
     - `ether2`: Gateway for the second subnet (e.g., `172.16.Y1.254/24`).

3. **Setting Up Routing**:  
   - Default and static routes are configured:
     - Route to the internet:
       ```
       /ip route add dst-address=0.0.0.0/0 gateway=172.16.1.254
       ```
     - Route to the local subnet:
       ```
       /ip route add dst-address=172.16.Y0.0/24 gateway=172.16.Y1.253
       ```

4. **Enabling NAT**:  
   - NAT is activated on the router to map internal IPs to the public IP:
     ```
     /ip firewall nat add chain=srcnat action=masquerade out-interface=ether1
     ```

5. **Testing Connectivity**:  
   - Devices in the local network (tuxY3 and tuxY2) are configured with the router as their default gateway.  
   - Pinging external addresses (e.g., the internet or the lab FTP server) verifies internet connectivity.  

6. **Observing Packet Behavior**:  
   - Wireshark captures on tuxY3 and tuxY2 reveal how NAT alters source IPs in outgoing packets and restores them in responses.

---

### **Key Questions**

1. **How to configure a static route in a commercial router?**  
   - Use the command:  
     ```
     /ip route add dst-address=<destination> gateway=<gateway>
     ```
   - Example:  
     ```
     /ip route add dst-address=172.16.Y0.0/24 gateway=172.16.Y1.253
     ```

2. **What are the paths followed by packets with and without ICMP redirect enabled? Why?**  
   - **Without ICMP Redirect**: Packets take the default route through the commercial router.  
   - **With ICMP Redirect**: Packets take a direct route through tuxY4 if available, as ICMP redirects optimize path selection.  
   - The behavior depends on whether the router or another device acts as the gateway.

3. **How to configure NAT in a commercial router? What does it do?**  
   - NAT is configured using the command:  
     ```
     /ip firewall nat add chain=srcnat action=masquerade out-interface=<interface>
     ```
   - **Function**: NAT translates private IP addresses in outgoing packets to the router's public IP. Incoming responses are translated back to the original private IPs. This reduces the need for unique public IPs and allows secure internet access.

4. **What happens when tuxY3 pings the FTP server with NAT disabled? Why?**  
   - Without NAT, outgoing packets retain their private IPs, which are not routable on the internet. Responses from the server fail to reach tuxY3, as there is no mechanism to map private addresses back to the originating device.

---

### **Key Insights**

This experiment highlights the role of commercial routers in connecting isolated subnets and providing internet access. NAT is crucial for translating private IP addresses to a public IP, enabling devices to communicate externally. Observing the routing behavior with and without ICMP redirects further illustrates how routers optimize traffic flow.

## **Experience 5 - Configuring DNS**

This experiment focuses on configuring DNS (Domain Name System) in a local network to resolve domain names into IP addresses. It demonstrates how DNS facilitates internet access and validates its functionality through packet inspection.

---

### **Procedures**

1. **Configuring DNS on Hosts**:  
   - The `/etc/resolv.conf` file is edited on tuxY3, tuxY4, and tuxY2 to include the DNS server provided by the lab:
     ```
     nameserver 10.227.20.3
     ```

2. **Verifying DNS Functionality**:  
   - Commands like `ping google.com` are executed on the hosts to test DNS resolution.  
   - Successful pings confirm that domain names are being resolved to IP addresses.

3. **Capturing DNS Packets**:  
   - Wireshark captures DNS query and response packets as tuxY3 or other hosts attempt to access domain names.

4. **Analyzing DNS Behavior**:  
   - The logs from Wireshark are studied to observe how DNS queries are sent to the server and how responses are received.

---

### **Key Questions**

1. **How to configure the DNS service in a host?**  
   - Edit the `/etc/resolv.conf` file and add the IP address of the DNS server:
     ```
     nameserver <DNS_server_IP>
     ```
   - Example:
     ```
     nameserver 10.227.20.3
     ```

2. **What packets are exchanged by DNS, and what information is transported?**  
   - **DNS Query**:  
     - Sent by the host to the DNS server, containing the domain name to resolve.  
     - Example: "What is the IP for `google.com`?"  
   - **DNS Response**:  
     - Sent by the DNS server, containing the resolved IP address.  
     - Example: "The IP for `google.com` is `142.250.185.14`."  

   These packets use UDP on port 53, but fallback to TCP if the response is too large.

---

### **Key Insights**

This experiment highlights the critical role of DNS in enabling user-friendly domain names instead of raw IP addresses for accessing resources on the internet. Configuring DNS on hosts and analyzing DNS packets in Wireshark provide a deeper understanding of how domain name resolution works.

## **Experience 6 - TCP Connections and FTP Analysis**

This experiment focuses on understanding the behavior of TCP (Transmission Control Protocol) during data transfers by using a custom FTP client. It explores TCP's connection management, flow control, congestion control, and its interaction with FTP.

---

### **Procedures**

1. **Compiling and Running the FTP Client**:  
   - The custom FTP client developed in earlier tasks is compiled and executed on tuxY3 to download a file from the lab FTP server.

2. **Capturing TCP Packets**:  
   - Wireshark is used to monitor TCP traffic during the file transfer, capturing control (e.g., SYN, ACK, FIN) and data packets.

3. **Analyzing TCP Phases**:  
   - The connection establishment (three-way handshake), data transfer, and connection termination are analyzed in Wireshark.  

4. **Observing ARQ and Congestion Control**:  
   - Automatic Repeat Request (ARQ) and congestion control mechanisms are observed by monitoring retransmissions and window size adjustments during varying network loads.

5. **Concurrent Transfers**:  
   - A second FTP download is initiated from tuxY2 during the tuxY3 transfer to analyze the impact of congestion and bandwidth sharing.

---

### **Key Questions**

1. **How many TCP connections are opened by your FTP application?**  
   - Two connections:  
     - One for control commands (e.g., login, file selection).  
     - One for data transfer (e.g., downloading the file).

2. **In what connection is FTP control information transported?**  
   - FTP control information is transported over the control connection, typically on port 21.

3. **What are the phases of a TCP connection?**  
   - **Connection Establishment**: Three-way handshake (SYN, SYN-ACK, ACK).  
   - **Data Transfer**: Reliable data delivery using sequence numbers, acknowledgments, and window sizes.  
   - **Connection Termination**: Graceful closure using FIN and ACK packets.

4. **How does the ARQ TCP mechanism work? What relevant TCP fields can be observed?**  
   - ARQ ensures reliable delivery by retransmitting lost packets based on acknowledgments (ACK) and timeouts.  
   - Relevant fields include:
     - **Sequence Number**: Identifies the position of data in the stream.  
     - **Acknowledgment Number**: Confirms receipt of data.  
     - **Window Size**: Indicates the receiver's buffer space.

5. **How does the TCP congestion control mechanism work? What fields are relevant?**  
   - TCP uses mechanisms like **Slow Start** and **Congestion Avoidance**:
     - **Slow Start**: Exponentially increases the number of packets sent until packet loss occurs.  
     - **Congestion Avoidance**: Gradually increases throughput using Additive Increase/Multiplicative Decrease (AIMD).  
   - Relevant fields include the **Congestion Window (CWND)** and **Duplicate ACKs** indicating lost packets.

6. **How did the throughput of the data connection evolve over time? Is it consistent with TCP congestion control?**  
   - The throughput initially increases rapidly during the slow start phase. After a loss event, it decreases and stabilizes as congestion avoidance takes over.  
   - This pattern matches the expected behavior of TCP congestion control.

7. **Is the throughput of a TCP data connection disturbed by a second TCP connection? How?**  
   - Yes, initiating a second transfer from tuxY2 reduces the throughput of tuxY3's connection.  
   - TCP's fairness mechanism ensures bandwidth is shared between the connections, leading to a reduction in throughput for both.

---

### **Key Insights**

This experiment illustrates the intricacies of TCP's behavior, including its reliability, flow control, and congestion management. Using Wireshark to analyze FTP traffic provides a clear understanding of how TCP adapts to network conditions and manages simultaneous connections.


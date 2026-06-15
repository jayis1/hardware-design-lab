# Aether-Link: Phase 1 — Conceptual Architecture

## 1. System Purpose and Vision

Aether-Link is a PCIe 4.0 x8 add-in card that provides hardware-accelerated NVMe-over-Fabric/TCP
(NVMe-oF/TCP) offload. It presents itself to the host OS as a standard NVMe controller while
internally translating NVMe commands into NVMe-oF/TCP protocol data units (PDUs) and transmitting
them over dual 100 Gigabit Ethernet (100GbE) QSFP28 ports to remote NVMe-oF storage targets.

The card eliminates the CPU overhead of kernel TCP/IP stack processing, NVMe command encapsulation,
and data copy operations that plague software-only NVMe-oF initiators. By offloading the entire
TCP/IP protocol stack, RDMA-style zero-copy data movement, and NVMe-oF PDU construction into
dedicated FPGA hardware, Aether-Link achieves line-rate 100GbE throughput with sub-10-microsecond
latency for 4KB random reads.

### Key Use Cases
- **Hyper-converged infrastructure (HCI)**: Connect compute nodes directly to NVMe-oF storage
  targets without CPU bottleneck
- **AI/ML training clusters**: High-throughput, low-latency access to shared NVMe storage pools
  for training data loading
- **Database acceleration**: Offload storage networking for distributed SQL/NoSQL databases
- **Cloud-native storage**: Disaggregated storage architectures where compute and storage are
  physically separated but connected via high-speed fabric

## 2. Performance Targets

| Metric                          | Target Value                    |
|---------------------------------|---------------------------------|
| Host Interface                  | PCIe 4.0 x8 (16 GT/s per lane) |
| Host Bandwidth                  | ~15.75 GB/s theoretical, ~14 GB/s achievable |
| Network Interface               | 2x 100GbE QSFP28 (IEEE 802.3bj) |
| Network Bandwidth               | 200 Gbps aggregate (100 Gbps per port) |
| 4KB Random Read IOPS            | >2,000,000 IOPS (single port)  |
| 4KB Random Read Latency         | <8 µs (PCIe-to-wire, no host stack) |
| 64KB Sequential Read Throughput | >12 GB/s (single port)         |
| TCP Connection Capacity         | 256 concurrent connections     |
| NVMe Namespace Capacity         | 128 namespaces per controller  |
| Power Consumption               | <75W (PCIe slot power only)    |
| Form Factor                     | Full-height, half-length (FHHL) PCIe card |
| Operating Temperature           | 0°C to 55°C ambient            |

## 3. High-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          AETHER-LINK PCIe CARD                           │
│                                                                          │
│  ┌──────────┐   PCIe 4.0 x8    ┌──────────────────────────────────────┐ │
│  │  HOST    │◄════════════════►│         XILINX KINTEX-7 FPGA          │ │
│  │  CPU     │   16 GT/s/lane   │         XC7K325T-2FFG900C             │ │
│  │          │                  │                                       │ │
│  │ NVMe     │                  │  ┌─────────────────────────────────┐  │ │
│  │ Driver   │                  │  │  PCIe Hard IP Block             │  │ │
│  │ (stock)  │                  │  │  - Gen4 x8 Endpoint             │  │ │
│  └──────────┘                  │  │  - 8 BARs                       │  │ │
│                                │  │  - MSI-X (2048 vectors)         │  │ │
│                                │  └───────────┬─────────────────────┘  │ │
│                                │              │ AXI4-Stream             │ │
│                                │  ┌───────────▼─────────────────────┐  │ │
│                                │  │  NVMe Command Processor         │  │ │
│                                │  │  - Admin SQ/CQ (BAR0)           │  │ │
│                                │  │  - I/O SQ/CQ (BAR0)             │  │ │
│                                │  │  - PRP/SGL List Walker          │  │ │
│                                │  │  - Namespace Translation Table  │  │ │
│                                │  └───────────┬─────────────────────┘  │ │
│                                │              │ Internal Command Bus   │ │
│                                │  ┌───────────▼─────────────────────┐  │ │
│                                │  │  NVMe-oF PDU Engine             │  │ │
│                                │  │  - Capsule Construction        │  │ │
│                                │  │  - Command Capsule (ICReq)      │  │ │
│                                │  │  - Response Capsule (ICResp)    │  │ │
│                                │  │  - Data Transfer (H2D/D2H PDU) │  │ │
│                                │  │  - CRC32C Generation/Check      │  │ │
│                                │  └───────────┬─────────────────────┘  │ │
│                                │              │ PDU Stream             │ │
│                                │  ┌───────────▼─────────────────────┐  │ │
│                                │  │  TCP/IP Offload Engine (TOE)    │  │ │
│                                │  │  - 256 Connection State Tables  │  │ │
│                                │  │  - TCP Segmentation (TSO)       │  │ │
│                                │  │  - TCP Receive Offload (LRO)    │  │ │
│                                │  │  - Retransmit Timers            │  │ │
│                                │  │  - Congestion Control (DCTCP)   │  │ │
│                                │  │  - IP/UDP/TCP Checksum          │  │ │
│                                │  │  - ARP/ICMP Responder          │  │ │
│                                │  └───────┬───────────┬─────────────┘  │ │
│                                │         │           │                 │ │
│                                │  ┌──────▼───┐  ┌───▼──────┐          │ │
│                                │  │ 100GbE   │  │ 100GbE   │          │ │
│                                │  │ MAC #0   │  │ MAC #1   │          │ │
│                                │  │ (CMAC)   │  │ (CMAC)   │          │ │
│                                │  └──────┬───┘  └───┬──────┘          │ │
│                                └─────────┼─────────┼──────────────────┘ │
│                                          │         │                    │
│                                ┌─────────▼──┐  ┌───▼─────────┐         │
│                                │ QSFP28 #0  │  │ QSFP28 #1   │         │
│                                │ 100GbE     │  │ 100GbE      │         │
│                                │ SR4/CR4    │  │ SR4/CR4     │         │
│                                └────────────┘  └─────────────┘         │
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  MEMORY SUBSYSTEM                                                 │   │
│  │  ┌─────────────────────┐  ┌─────────────────────┐                │   │
│  │  │ DDR4-3200 Ch A      │  │ DDR4-3200 Ch B      │                │   │
│  │  │ 4GB (MT40A512M16)   │  │ 4GB (MT40A512M16)   │                │   │
│  │  │ x16, 1 rank         │  │ x16, 1 rank         │                │   │
│  │  └─────────────────────┘  └─────────────────────┘                │   │
│  │  Total: 8GB DDR4-3200, 25.6 GB/s per channel                     │   │
│  │  Usage: PDU buffer pool, connection state, retransmit buffers    │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  SUPPORT PERIPHERALS                                              │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │   │
│  │  │ SPI Flash    │  │ I2C Temp     │  │ USB Debug     │           │   │
│  │  │ MT25QU512    │  │ TMP117       │  │ FT232H        │           │   │
│  │  │ 512Mb QSPI   │  │ ±0.1°C       │  │ UART/JTAG     │           │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘           │   │
│  │  ┌──────────────┐  ┌──────────────┐                              │   │
│  │  │ Fan Control  │  │ Power Mon    │                              │   │
│  │  │ EMC2301      │  │ INA226       │                              │   │
│  │  │ PWM Fan x2   │  │ V/I/P 12V    │                              │   │
│  │  └──────────────┘  └──────────────┘                              │   │
│  └──────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

## 4. Data Flow Description

### 4.1 Host Write (NVMe Write Command → NVMe-oF/TCP Target)

1. Host NVMe driver posts a Write command to an I/O Submission Queue (SQ) in BAR0 memory.
2. Host rings the SQ Tail Doorbell register (MMIO write to BAR0).
3. FPGA's NVMe Command Processor detects the doorbell, fetches the command from SQ.
4. Command Processor validates PRP/SGL entries, determines total transfer length.
5. Command Processor issues PCIe Memory Read TLPs to fetch host data via PRP list.
6. Data arrives via PCIe Read Completion TLPs, buffered in DDR4.
7. NVMe-oF PDU Engine constructs an NVMe-oF Command Capsule (ICReq) with the NVMe command.
8. TCP Offload Engine segments the capsule + data into TCP segments (TSO).
9. 100GbE MAC transmits Ethernet frames with CRC32.
10. Target processes command, returns NVMe-oF Response Capsule (ICResp).
11. FPGA receives response via 100GbE MAC → TCP Offload Engine → PDU Engine.
12. PDU Engine posts completion entry to host I/O Completion Queue (CQ).
13. FPGA generates MSI-X interrupt to host.
14. Host NVMe driver processes completion.

### 4.2 Host Read (NVMe Read Command → NVMe-oF/TCP Target)

1. Host posts Read command to SQ, rings doorbell.
2. FPGA fetches command, constructs NVMe-oF Command Capsule.
3. TCP Offload Engine transmits capsule to target.
4. Target processes read, returns Data Transfer PDU (D2H) with read data.
5. TCP Offload Engine receives segments, reassembles in DDR4 buffer.
6. PDU Engine validates CRC32C, extracts data payload.
7. FPGA issues PCIe Memory Write TLPs to deposit data directly into host memory
   (zero-copy via PRP list pre-fetched addresses).
8. PDU Engine posts completion to CQ, generates MSI-X.

### 4.3 Connection Establishment

1. Host driver issues Admin command to create a new NVMe-oF connection.
2. FPGA TCP Offload Engine performs TCP 3-way handshake with target.
3. NVMe-oF PDU Engine performs Fabric Connect (Property Get/Set exchange).
4. Connection state is stored in DDR4 connection table.
5. Completion posted to host with Connection ID (CID).

## 5. Bus Topology and Interconnect

### 5.1 Internal FPGA Interconnect (AXI4)

```
PCIe Hard IP ──AXI4-MM (256-bit @ 250MHz)──► NVMe Command Processor
NVMe Cmd Proc ──AXI4-Stream (256-bit)──────► NVMe-oF PDU Engine
NVMe-oF PDU   ──AXI4-Stream (256-bit)──────► TCP Offload Engine
TCP Offload   ──AXI4-Stream (256-bit)──────► 100GbE CMAC #0
TCP Offload   ──AXI4-Stream (256-bit)──────► 100GbE CMAC #1
All blocks    ──AXI4-MM (64-bit)───────────► DDR4 MIG Controller
MIG Ch A      ──DFI────────────────────────► DDR4 SDRAM Ch A
MIG Ch B      ──DFI────────────────────────► DDR4 SDRAM Ch B
MicroBlaze    ──AXI4-Lite─────────────────► All control/status registers
```

### 5.2 Clock Domains

| Domain        | Frequency | Source                          | Used By                        |
|---------------|-----------|---------------------------------|--------------------------------|
| PCIe refclk   | 100 MHz   | PCIe slot (HCSL)               | PCIe Hard IP, AXI4-MM bridge  |
| DDR4 refclk   | 200 MHz   | SiT9121 differential oscillator | MIG PHY, DDR4 interface       |
| CMAC refclk   | 161.1328125 MHz | SiT9365 ultra-low jitter   | 100GbE CMAC SerDes            |
| System clk    | 250 MHz   | MMCM from PCIe refclk          | NVMe engine, PDU engine, TOE  |
| User clk      | 200 MHz   | MMCM from DDR4 refclk          | MicroBlaze, peripherals       |
| QSFP28 refclk | 156.25 MHz | SiT9365 (each port)            | QSFP28 module reference       |

### 5.3 Reset Hierarchy

```
PCIe PERST# ──► FPGA global reset ──► PCIe Hard IP reset
                                  ──► MMCM lock wait
                                  ──► DDR4 MIG calibration
                                  ──► CMAC reset
                                  ──► MicroBlaze reset
                                  ──► All fabric logic reset
```

## 6. Power Architecture

```
PCIe Slot 12V ──┬── 12V rail (fans, QSFP28 modules)
                ├── TPS546D24A (12V→VCCINT 1.0V @ 30A) ──► FPGA Core
                ├── TPS546D24A (12V→VCCBRAM 1.0V @ 5A)  ──► FPGA BRAM
                ├── TPS546D24A (12V→VCCAUX 1.8V @ 8A)   ──► FPGA Aux
                ├── TPS546D24A (12V→VCCO 1.5V @ 10A)    ──► DDR4, FPGA I/O
                ├── TPS546D24A (12V→MGTAVCC 1.0V @ 12A) ──► GTX Transceivers
                ├── TPS546D24A (12V→MGTAVTT 1.2V @ 8A)  ──► GTX Termination
                ├── TPS7A92 (12V→3.3V @ 3A)             ──► SPI, I2C, USB, misc
                └── TPS7A92 (12V→2.5V @ 1A)             ──► CMAC I/O, config
```

Total budget: ~68W typical, ~75W maximum. All within PCIe x8 slot limit (75W).

## 7. Security Architecture

- **Secure Boot**: FPGA bitstream authenticated via HMAC-SHA256 using internal AES key
  stored in eFUSE. Bitstream encrypted with AES-GCM 256-bit.
- **Connection Authentication**: NVMe-oF DH-HMAC-CHAP per NVMe-oF 1.1 spec, computed
  in FPGA fabric (MicroBlaze-assisted for DH key exchange).
- **Data Integrity**: CRC32C per NVMe-oF PDU, TCP checksum, Ethernet FCS.
- **Memory Protection**: DDR4 ECC (SECDED) via FPGA MIG controller.
- **PCIe ACS**: Access Control Services enabled for IOMMU compatibility.

## 8. Thermal Management

- **Primary cooling**: Dual 40mm axial fans (Delta AFB0412VHB) controlled by EMC2301
  PWM fan controller based on TMP117 temperature readings.
- **Heatsink**: Custom aluminum heatsink with copper heat pipes covering FPGA, DDR4,
  and power stages. Thermal resistance <0.3°C/W.
- **Thermal throttling**: FPGA fabric monitors junction temperature via XADC.
  At 85°C, reduces TCP window size to lower throughput. At 95°C, signals host
  driver to reduce queue depth. At 100°C, triggers PCIe hot-remove request.

## 9. Compliance and Standards

- **PCI Express**: PCIe Base Specification 4.0, PCIe Card Electromechanical 4.0
- **NVMe**: NVM Express 1.4c, NVMe-oF 1.1
- **Ethernet**: IEEE 802.3bj (100GBASE-CR4/SR4/KR4), IEEE 802.3bs
- **I2C**: I2C-bus specification 3.0 (1 MHz Fast-mode Plus)
- **SPI**: JESD216D (SFDP) for flash discovery
- **EMC**: FCC Part 15 Class A, EN 55032 Class A
- **Safety**: IEC 62368-1, UL 62368-1
- **RoHS**: EU 2011/65/EU (RoHS 2), EU 2015/863 (RoHS 3)

## 10. Development and Debug Infrastructure

- **JTAG Chain**: FPGA (XC7K325T) ← FT232H (MPSSE) ← USB Micro-B connector
- **UART Console**: MicroBlaze UART via FT232H channel B, 115200 baud
- **In-System Debug**: Xilinx ILA (Integrated Logic Analyzer) cores for key buses
- **LED Indicators**: 4x bi-color LEDs (status, link, activity, error) via GPIO
- **Board Management**: INA226 power monitor accessible via I2C from MicroBlaze
- **Field Updates**: FPGA bitstream update via PCIe vendor-specific command
  (NVMe Admin command 0xC0-0xFF range), writing to SPI flash

## 11. Software Stack Overview

```
┌─────────────────────────────────────────────┐
│  Host Linux Kernel                          │
│  ┌───────────────────────────────────────┐  │
│  │  Standard NVMe Driver (nvme.ko)       │  │
│  │  - No modifications required         │  │
│  │  - Uses PCI class code 0x010802      │  │
│  └───────────────────────────────────────┘  │
│  ┌───────────────────────────────────────┐  │
│  │  Aether-Link Management Driver        │  │
│  │  (aether_mgmt.ko)                     │  │
│  │  - Connection lifecycle management   │  │
│  │  - Firmware update                    │  │
│  │  - Telemetry (temp, power, stats)    │  │
│  │  - sysfs interface                   │  │
│  └───────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
┌─────────────────────────────────────────────┐
│  FPGA Bitstream (Hardware)                  │
│  ┌───────────────────────────────────────┐  │
│  │  PCIe Endpoint + NVMe Controller      │  │
│  │  NVMe-oF PDU Engine                   │  │
│  │  TCP/IP Offload Engine                │  │
│  │  100GbE MAC + SerDes                  │  │
│  │  DDR4 MIG Controller                  │  │
│  │  MicroBlaze Management Processor      │  │
│  └───────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
┌─────────────────────────────────────────────┐
│  Companion Mobile App (React Native)        │
│  - Connection over host-side REST API       │
│  - Real-time telemetry dashboard           │
│  - Connection management                   │
│  - Performance graphs                      │
└─────────────────────────────────────────────┘
```

## 12. Key Design Decisions and Rationale

### Why Kintex-7 (XC7K325T) instead of Zynq UltraScale+?
- Kintex-7 provides sufficient logic (326K LUTs, 840 DSP slices, 16 GTX transceivers)
  for the TOE + NVMe-oF engine at lower cost than UltraScale+.
- PCIe Gen4 hard IP is available in Kintex-7 (-2 speed grade).
- 16 GTX transceivers at 12.5 Gbps each: 8 for PCIe Gen4 x8, 8 for dual 100GbE
  (4 lanes × 25.78125 Gbps per QSFP28 port).
- Lower power (~12W typical) vs UltraScale+ (~18W).

### Why dual 100GbE instead of single 200GbE?
- Redundancy and multipathing: two independent paths to storage fabric.
- Standard QSFP28 modules are widely available and cost-effective.
- 200GbE (QSFP56) would require 50 Gbps SerDes not available in Kintex-7 GTX.

### Why DDR4-3200 instead of HBM?
- HBM requires UltraScale+ or Versal, significantly increasing cost.
- 8GB DDR4 provides sufficient buffering for 256 connections × 256KB window.
- Two independent channels provide 51.2 GB/s aggregate bandwidth, exceeding
  PCIe Gen4 x8 (~16 GB/s) and dual 100GbE (~25 GB/s) requirements.

### Why NVMe-oF/TCP instead of NVMe-oF/RDMA (RoCEv2)?
- TCP runs over any Ethernet fabric; RoCEv2 requires DCB/PFC-capable switches.
- TCP is routable across subnets; RoCEv2 is L2 only without complex gateways.
- NVMe-oF/TCP is the NVMe-oF 1.1 standard transport with broad industry adoption.
- Hardware TCP offload achieves comparable latency to RDMA for NVMe workloads.

## 13. Risk Analysis and Mitigation

| Risk                              | Probability | Impact | Mitigation                                    |
|-----------------------------------|-------------|--------|-----------------------------------------------|
| FPGA timing closure at 250MHz     | Medium      | High   | Aggressive pipelining, multi-cycle paths       |
| DDR4 MIG calibration failure      | Low         | High   | Conservative PCB layout, simulation            |
| 100GbE SerDes signal integrity    | Medium      | High   | IBIS-AMI simulation, impedance control        |
| TCP retransmit buffer overflow    | Medium      | Medium | Adaptive window sizing, ECN marking           |
| PCIe Gen4 link training failure   | Low         | High   | Redriver consideration, strict layout rules   |
| Thermal throttling under load     | Medium      | Medium | Active cooling, heatsink design margin        |
| NVMe-oF spec compliance gaps      | Low         | Medium | UNH-IOL plugfest testing planned              |

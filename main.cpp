#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

time_t actTime;
static u_int32_t globalFlowSequence = 0;

struct flow_ver5_hdr {
    u_int16_t version; /* Current version=5*/
    u_int16_t count; /* The number of records in PDU. */
    u_int32_t sysUptime; /* Current time in msecs since router booted */
    u_int32_t unix_secs; /* Current seconds since 0000 UTC 1970 */
    u_int32_t unix_nsecs; /* Residual nanoseconds since 0000 UTC 1970 */
    u_int32_t flow_sequence; /* Sequence number of total flows seen */
    u_int8_t engine_type; /* Type of flow switching engine (RP,VIP,etc.)*/
    u_int8_t engine_id; /* Slot number of the flow switching engine */
};

struct flow_ver5_rec {
    u_int32_t srcaddr; /* Source IP Address */
    u_int32_t dstaddr; /* Destination IP Address */
    u_int32_t nexthop; /* Next hop router's IP Address */
    u_int16_t input; /* Input interface index */
    u_int16_t output; /* Output interface index */
    u_int32_t dPkts; /* Packets sent in Duration (milliseconds between 1st
& last packet in this flow)*/
    u_int32_t dOctets; /* Octets sent in Duration (milliseconds between 1st
& last packet in this flow)*/
    u_int32_t First; /* SysUptime at start of flow */
    u_int32_t Last; /* and of last packet of the flow */
    u_int16_t srcport; /* TCP/UDP source port number (.e.g, FTP, Telnet, etc.,or equivalent) */
    u_int16_t dstport; /* TCP/UDP destination port number (.e.g, FTP, Telnet, etc.,or equivalent) */
    u_int8_t pad1; /* pad to word boundary */
    u_int8_t tcp_flags; /* Cumulative OR of tcp flags */
    u_int8_t prot; /* IP protocol, e.g., 6=TCP, 17=UDP, etc... */
    u_int8_t tos; /* IP Type-of-Service */
    u_int16_t dst_as; /* dst peer/origin Autonomous System */
    u_int16_t src_as; /* source peer/origin Autonomous System */
    u_int8_t dst_mask; /* destination route's mask bits */
    u_int8_t src_mask; /* source route's mask bits */
    u_int16_t pad2; /* pad to word boundary */
};

const int RECORDS_COUNT = 20;

typedef struct single_flow_ver5_rec {
    flow_ver5_hdr flowHeader;
    flow_ver5_rec flowRecord[RECORDS_COUNT];
}NetFlow5Records;

typedef struct packetData {
    string srcIp;
    string srcPort;
    string destIp;
    string destPort;
    int bytesCount;
    int protocol;
}PacketData;

static void initFlowHeader(struct flow_ver5_hdr *flowHeader) {

    memset(flowHeader, 0, sizeof (flowHeader));
    flowHeader->version = htons(5);
    flowHeader->count = htons(RECORDS_COUNT);
    flowHeader->sysUptime = htonl((actTime - actTime - 1000)*1000);
    flowHeader->unix_secs = htonl(actTime);
    flowHeader->unix_nsecs = htonl(0);
    flowHeader->flow_sequence = htonl(globalFlowSequence);
    flowHeader->engine_type = 0;
    flowHeader->engine_id = 0;
    globalFlowSequence += RECORDS_COUNT;
}

in_addr_t make_addr(string aStr) {
    return (inet_addr(aStr.c_str()));
}

int make_ports(string aStr) {
    return atoi(aStr.c_str());
}

int parceHost(const string & aStr, const int aStartPos, string & ip, string & port) {

    int ipEndPos = 0;
    int PortEndPos = 0;

    const char* div = ":";
    const char* end = " ";

    ipEndPos = aStr.find(div, aStartPos);
    ip = aStr.substr(aStartPos, ipEndPos - aStartPos);

    PortEndPos = aStr.find(end, aStartPos);
    port = aStr.substr(ipEndPos + 1, PortEndPos - ipEndPos - 1);

    return PortEndPos + 3;
}

int parceParams(const string & aStr, const int aStartPos, string & res){

    const char* div = " ";
    int ipEndPos = 0;

    ipEndPos = aStr.find(div, aStartPos);
    res = aStr.substr(aStartPos, ipEndPos - aStartPos);

    return ipEndPos;
}

PacketData parceLine(const string & aStr) {

    PacketData packet;
//    memset(&packet, 0, sizeof (packet));

    cout << "Line: " << aStr << endl;
    int hostEndPos = parceHost(aStr, 0, packet.srcIp, packet.srcPort);
    int startNextParam = parceHost(aStr, hostEndPos, packet.destIp, packet.destPort);

    string size;
    startNextParam = parceParams(aStr, startNextParam - 2, size);
    packet.bytesCount = atoi(size.c_str());

    string packetType;
    startNextParam = parceParams(aStr, startNextParam + 1, packetType);

    if(packetType.compare("UDP") == 0){
        packet.protocol = 17;
    }
    else if(packetType.compare("TCP") == 0){
        packet.protocol = 6;
    }
    else{
        cout << "Unknow protocol type: " << packetType << endl;
    }

    cout << "packetType: " << packetType << endl;
    return packet;
}

bool sendData(const string sendIp, const void * packet, const int packetSize)
{
    struct sockaddr_in sin;
    int sock = 0;
    actTime = time(NULL);

    const int port = 7223;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(sendIp.c_str());
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    cout << "Send to: " << sendIp << ":" << port << " packet size: " << packetSize << endl;

    if (sendto(sock, packet, packetSize, 0, (struct sockaddr *) &sin, sizeof (sin)) == -1) {
        printf("sendto error\n");
        exit(0);
    }

    close(sock);
}

void fill(const PacketData & packet, flow_ver5_rec & theRecord)
{
    cout << "fill" << endl;
    memset(&theRecord, 0, sizeof (theRecord));
    
    theRecord.srcaddr = make_addr(packet.srcIp);
    theRecord.dstaddr = make_addr(packet.destIp);
    theRecord.srcport = htons(make_ports(packet.srcPort));
    theRecord.dstport = htons(make_ports(packet.destPort));

    theRecord.input = htonl(1);
    theRecord.output = htonl(1);
    theRecord.dPkts = htonl(1);
    theRecord.dOctets = htonl(packet.bytesCount);

    theRecord.First = htonl((actTime - 1000)*1000);
    theRecord.Last = htonl((actTime - 100)*1000);
    theRecord.prot = packet.protocol;

    cout << "fill" << endl;
}

void fromTube(char *argv)
{
    PacketData  pd;
    memset(&pd, 0, sizeof (pd));

    NetFlow5Records records;
    memset(&records, 0, sizeof (records));

    string str;
    int i = 0;

    while (true) {
        getline(cin, str);
        if (str.length() > 0) {
            cout << str << endl;
//            pd = parceLine(str);
            cout << "pre fill" << endl;
            fill(pd,records.flowRecord[i]);
            if(i == RECORDS_COUNT-1){            // Накопилось достаточно записей
                initFlowHeader(&records.flowHeader);
//                sendData(argv, (void*)&records, sizeof(records));
//                memset(&records, 0, sizeof(records));
                i = 0;
            }
            else
            {
                i++;
            }
        }
    }
}

int main(int argc, char *argv[]) {

    int i = 0;
    int count = 0;
    unsigned long int totsize = 0;
    int size = 0;

    if (argc < 2) {
        printf("Usage: %s ip_addres\n", argv[0]);
        exit(0);
    }

    fromTube(argv[1]);
    
    exit(0);
}
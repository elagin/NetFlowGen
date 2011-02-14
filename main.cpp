#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <iostream>
#include <fstream>
//#include "boost/spirit/include/classic.hpp"


using namespace std;//::string;

time_t actTime;
static u_int32_t globalFlowSequence=0;

struct flow_ver5_hdr {
  u_int16_t version;         /* Current version=5*/
  u_int16_t count;           /* The number of records in PDU. */
  u_int32_t sysUptime;       /* Current time in msecs since router booted */
  u_int32_t unix_secs;       /* Current seconds since 0000 UTC 1970 */
  u_int32_t unix_nsecs;      /* Residual nanoseconds since 0000 UTC 1970 */
  u_int32_t flow_sequence;   /* Sequence number of total flows seen */
  u_int8_t  engine_type;     /* Type of flow switching engine (RP,VIP,etc.)*/
  u_int8_t  engine_id;       /* Slot number of the flow switching engine */
};

 struct flow_ver5_rec {
   u_int32_t srcaddr;    /* Source IP Address */
   u_int32_t dstaddr;    /* Destination IP Address */
   u_int32_t nexthop;    /* Next hop router's IP Address */
   u_int16_t input;      /* Input interface index */
   u_int16_t output;     /* Output interface index */
   u_int32_t dPkts;      /* Packets sent in Duration (milliseconds between 1st
                           & last packet in this flow)*/
   u_int32_t dOctets;    /* Octets sent in Duration (milliseconds between 1st
                           & last packet in  this flow)*/
   u_int32_t First;      /* SysUptime at start of flow */
   u_int32_t Last;       /* and of last packet of the flow */
   u_int16_t srcport;    /* TCP/UDP source port number (.e.g, FTP, Telnet, etc.,or equivalent) */
   u_int16_t dstport;    /* TCP/UDP destination port number (.e.g, FTP, Telnet, etc.,or equivalent) */
   u_int8_t pad1;        /* pad to word boundary */
   u_int8_t tcp_flags;   /* Cumulative OR of tcp flags */
   u_int8_t prot;        /* IP protocol, e.g., 6=TCP, 17=UDP, etc... */
   u_int8_t tos;         /* IP Type-of-Service */
   u_int16_t dst_as;     /* dst peer/origin Autonomous System */
   u_int16_t src_as;     /* source peer/origin Autonomous System */
   u_int8_t dst_mask;    /* destination route's mask bits */
   u_int8_t src_mask;    /* source route's mask bits */
   u_int16_t pad2;       /* pad to word boundary */
};

typedef struct single_flow_ver5_rec {
  struct flow_ver5_hdr flowHeader;
  struct flow_ver5_rec flowRecord;
} SingleNetFlow5Record;


static void initFlowHeader(struct flow_ver5_hdr *flowHeader, int numCount) {

  flowHeader->version        = htons(5);
  flowHeader->count          = htons(numCount);
  flowHeader->sysUptime      = htonl((actTime-actTime-1000)*1000);
  flowHeader->unix_secs      = htonl(actTime);
  flowHeader->unix_nsecs     = htonl(0);
  flowHeader->flow_sequence  = htonl(globalFlowSequence);
  flowHeader->engine_type    = 0;
  flowHeader->engine_id      = 0;

  globalFlowSequence += numCount;

}

in_addr_t make_addr(int type)
{
	char tmp[20];
	int i;

	srand(time(NULL));
	i=1+(int) (200.0*rand()/(RAND_MAX+1.0));

	memset(&tmp, 0, sizeof(tmp));

	if(type == 1)
		sprintf(tmp, "10.10.10.1");
		else
		sprintf(tmp, "195.100.100.%d", i);

	return(inet_addr(tmp));
}

in_addr_t make_addr(string aStr)
{
	return(inet_addr(aStr.c_str()));
}

int make_ports(int type)
{

	srand(time(NULL));

	if(type == 1)
		return(1+(int) (110.0*rand()/(RAND_MAX+1.0)));
	else
		return(1024+(int) (55555.0*rand()/(RAND_MAX+1.0)));
}

int make_ports(string aStr)
{
    int res = atoi(aStr.c_str());
    return res;
}
//10.0.0.1:3000 > 10.0.0.8:4000 2000 UDP

//using namespace boost::spirit::classic;
//#include <boost/spirit/include/classic_push_back_actor.hpp>
//#include <iostream>
//#include <vector>
//#include <string>

//using namespace std;
//using namespace boost::spirit;
//
//// Парсер разделённых запятой чисел
//bool parse_numbers(const char* str, vector<double>& v)
//{
//    double one;
//    return parse(str,
//    // начало грамматики
//    (
////    real_p[push_back_a(v)] >> *(',' >> real_p[push_back_a(v)])
//    real_p[push_back_a(v)] >> *(',' >> real_p[push_back_a(v)])
//    )
//    ,
//    // конец грамматики
//    space_p).full;
//    }
//
////10.0.0.1:3000 > 10.0.0.8:4000 2000 UDP
//bool parseUrl(const string& str)
//{
//    rule<> word_p = +alpha_p;
//    rule<> scheme_p = word_p >> str_p("://");    // http://
//    rule<> host_p = word_p >> ch_p('.');             // www.
//    rule<> domain_p = word_p >> ch_p('.') >> word_p;   // google.com
//    rule<> port_p = ch_p(':') >> uint_p;               // :80
//    rule<> path_p = ch_p('/') % word_p;               // /path/to/file/
//    rule<> filename_p = word_p >> !(ch_p('.') >> word_p);   // logo.gif (extension optional)
//
//    // Optional scheme, optional host, domain, optional port, path and filename option. Can have path without filename, but not filename without path.
//    rule<> url_p = !scheme_p >> !host_p >> domain_p >> !port_p >> !(path_p >> !filename_p);
//
//    return parse(str.c_str(), url_p, space_p).full;
//}


int parceHost(const string & aStr, const int aStartPos, string & ip, string & port)
{
//    cout << "parceHost start: " << aStartPos << endl;
    int ipStartPos = 0;
    int ipEndPos = 0;

    int portStartPos = 0;
    int PortEndPos = 0;

    const char* div = ":";
    const char* end = " ";

    ipEndPos = aStr.find(div, aStartPos);
    ip = aStr.substr( aStartPos, ipEndPos - aStartPos );
//    cout << "ipEndPos: " << ipEndPos << "  size: " <<  ipEndPos - aStartPos << endl;

    PortEndPos = aStr.find(end, aStartPos);
//    cout << "PortEndPos: " << PortEndPos << endl;

    port = aStr.substr(ipEndPos + 1, PortEndPos - ipEndPos - 1);

//    cout << "ipEndPos - PortEndPos:" << ipEndPos - PortEndPos << endl;
    return PortEndPos + 3;

}

int parceParams(const string & aStr, const int aStartPos, string & res )
{
    cout << "parceParams start: " << aStartPos << endl;
    const char* div = " ";
    int ipEndPos = 0;
    ipEndPos = aStr.find(div, aStartPos);
    res = aStr.substr( aStartPos, ipEndPos - aStartPos );
    return ipEndPos;
}

//10.0.0.1:3000 > 10.0.0.8:4000 2000 UDP
void parceLine(const string & aStr)
{
    string srcIp;
    string srcPort;

    cout << "Line: " << aStr << endl;
    cout << "      01234567890123456789012345678901234567890" << endl;
    int hostEndPos = parceHost(aStr, 0, srcIp, srcPort);
    cout << "IP:[" << srcIp << "] port[" << srcPort << "]" << endl;
    cout << "=======================" << endl;
    
    int startNextParam = parceHost(aStr, hostEndPos, srcIp, srcPort);
    cout << "IP:[" << srcIp << "] port[" << srcPort << "]" << endl;
    cout << "=======================" << endl;

    string size;
    startNextParam = parceParams(aStr, startNextParam-2, size );
    cout << "size: [" << size << "]" << endl;

    string packetType;
    startNextParam = parceParams(aStr, startNextParam+1, packetType );
    cout << "packetType: [" << packetType << "]" << endl;
    
//   bool ok = parse(First, Last, (uint_ >> L"-" >> uint_), MinMax) && (First == Last);
//Spirit — одна из наиболее сложных частей Boost, предназначенная для написания парсеров напрямую в C++ тексте программы в виде близком к форме Бэкуса-Наура.
}

// cat file.txt | our_flow_gen -i:192.168.10.63 -p:7223
int main(int argc, char *argv[])
{
    SingleNetFlow5Record theRecord;
    struct sockaddr_in sin;
    int sock;
    int i = 0;
    int count=0;
    unsigned long int totsize=0;
    int size=0;


    if(argc < 3){
        printf("Usage: %s ip_addres port\n", argv[0]);
        exit(0);
    }

    string str;
    int pos = 0;

//    while(false)
//    {
//        getline(cin, str);
//        if( str.length() > 0){
//            cout << "Srt: " << str << endl;
//            parceLine(str);
//        }
/*
        ifstream In("file.txt", ios::in);
        while(! In.eof())
        {
//            int endPos = 0;
//            In.seekg (endPos, ios::end);
//            endPos = In.tellg();
//            cout << "ENDPos: " << endPos << endl;

            In.seekg (pos, ios::beg);
            getline (In, str);
            if( str.length() > 0){
//                cout << "STR: " << str << endl;
                pos = In.tellg();
//                cout << "Pos: " << pos << endl;
                parceLine(str);
            }
            else{
                break;
            }
        }
 */
//    }

    actTime = time(NULL);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(7223);
    sin.sin_addr.s_addr = inet_addr(argv[1]);

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&theRecord, 0, sizeof(theRecord));

    cout << "ready" << endl;
    while(1){

            if(atoi(argv[2]) != 0){
                    if(count == atoi(argv[2])){
                            printf("%d packets sended, total size: %ld\n", count, totsize);
                            exit(0);
                    }
            }

            memset(&theRecord, 0, sizeof(theRecord));
            srand(time(NULL));

            initFlowHeader(&theRecord.flowHeader, 1);
            if(i == 0){
                    theRecord.flowRecord.srcaddr   = make_addr(1);
                    theRecord.flowRecord.dstaddr   = make_addr(0);
                    theRecord.flowRecord.srcport   = htons(make_ports(0));
                    theRecord.flowRecord.dstport   = htons(make_ports(1));
                    i = 1;
            }else {
                    theRecord.flowRecord.srcaddr   = make_addr(0);
                    theRecord.flowRecord.dstaddr   = make_addr(1);
                    theRecord.flowRecord.srcport   = htons(make_ports(1));
                    theRecord.flowRecord.dstport   = htons(make_ports(0));
                    i = 0;
            }

            theRecord.flowRecord.input     = htonl(1);
            theRecord.flowRecord.output    = htonl(1);
            theRecord.flowRecord.dPkts     = htonl(1);
            if(atoi(argv[3]) == 0){
                    size = 1024+(int) (55555.0*rand()/(RAND_MAX+1.0));
                    theRecord.flowRecord.dOctets   = htonl(size);
            }
            else{
                    size = atoi(argv[3]);
                    theRecord.flowRecord.dOctets   = htonl(size);
            }
            theRecord.flowRecord.First     = htonl((actTime-1000)*1000);
            theRecord.flowRecord.Last      = htonl((actTime-100)*1000);
            theRecord.flowRecord.prot      = 17 /* UDP */;

            if(sendto(sock, (void*)&theRecord, sizeof(theRecord), 0, (struct sockaddr *)&sin, sizeof(sin)) == -1){
                    printf("sendto error\n");
                    exit(0);
            }
            count++;
            totsize += size;
            if(atoi(argv[4]) == -1)
                    usleep(100000+(int) (5000000.0*rand()/(RAND_MAX+1.0)));
            else
                    usleep(atoi(argv[4]));
    }

    close(sock);
    printf("1440 sec \n");
    exit(0);
}





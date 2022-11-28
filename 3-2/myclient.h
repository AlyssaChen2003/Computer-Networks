#ifndef MYCLIENT_H//�������ͷ�ļ�
#define MYCLIENT_H
#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <sstream>
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <io.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;


/*-----------------����ȫ�ֱ�������-----------------------------------*/
const int MAXSIZE = 2048;
SOCKADDR_IN server_addr;
SOCKET server;
//int SEQ = 0;
//int ACK = 0;
double TIMEOUT = 0.004;
double INTERACT_TIME = 1.0;
const int SEND_WIND_SIZE = 15;
int curAcked=-1;//��ǰ�Ѿ�ȷ�ϵ� ack 
int totalSeq;//�յ��İ�������
int totalPacket;//��Ҫ���͵İ�����
int nextseqnum = 0;
//const int nextseqnum = nextseqnum;//��ǰ���ݰ��� seq
int base = 0;//�����Ҳ��δȷ�ϵĵ�һ����


const int addrlen = sizeof(server_addr);

/*-------------------����֡�ײ�����-------------------------*/
class Header {
public:
    u_short datasize = 0;
    u_short sum = 0;
    u_char flag = 0;
    u_char ack = 0;//==seq
    u_char seq = 0;//seq==ack

    Header() { flag = 0; }
    //����header��ĸ�λ��ע�������seqû��ģ256����Ҫ����ʱת��
    void setHeader(u_short d, u_char f, u_short se) {
        this->datasize = d;
        this->seq = se;
        this->flag = f;

    }

    void show_header() {
        cout << "datasize: " << (int)this->datasize << " seq: " <<(int) this->seq << " flag: " << (int)this->flag << endl;
    }

};
/*--------------------------------���ݰ��ṹ����-----------------------------------------------*/
struct Packet {
    Header header;//UDP�ײ�
    char* Buffer;//������
    Packet() {
        Buffer = new char[MAXSIZE + sizeof(header)]();
    }
};
/*----------------------------��Ҫ�õĺ���--------------------------------------------------*/
void printsplit();

bool cksend(Header h);
bool ckend(Header h);
u_short cksum(u_short* buff, int size);
long long time(long long head);
void setSum(Header& header);
bool check_sign(Header header, u_char sign);
void init();
//void packet_send(SOCKET& clientsocket, SOCKADDR_IN& serveraddr, int& addrlen, char* message, int len, bool END);
void send(SOCKET& clientsocket, SOCKADDR_IN& serveraddr, int& addrlen, char* message, int len);
void start(int len);


/*-----------------------����������----------------------------------------------------------*/
void printsplit() {
    cout << "--------------------------------------------------------------------------" << endl;
}

bool cksend(Header h) {
    return (h.flag == 0x1);
}
bool ckend(Header h) {
    return (h.flag == 0x7);
}
u_short cksum(u_short* buff, int size) {
    int count = (size + 1) / 2;
    u_short* buf = new u_short[size + 1];
    memset(buf, 0, size + 1);
    memcpy(buf, buff, size);
    u_short sum = 0;
    while (count--) {//�����Ʒ������
        sum += *buf++;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++;
        }
    }
    return ~(sum & 0xFFFF);//���ȡ��
}
long long time(long long head) {
    long long tail, freq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    QueryPerformanceCounter((LARGE_INTEGER*)&tail);
    return (tail - head) / freq;
}
void setSum(Header& header) {
    header.sum = 0;
    u_short s = cksum((u_short*)&header, sizeof(header));
    header.sum = s;
}
bool check_sign(Header header, u_char sign) {
    if (header.flag == sign && cksum((u_short*)&header, sizeof(header)) == 0)
        return true;
    else
        return false;
}
void init() {
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    server_addr.sin_family = AF_INET;//ʹ��IPV4
    server_addr.sin_port = htons(4001);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server = socket(AF_INET, SOCK_DGRAM, 0);
    //bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr));//���׽��֣��������״̬
    //cout << "waiting client....." << endl;

}
bool interact(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen, string type) {
    //type ��hello��goodbye
    int sendsign = 0, recvsign = 0;
    if (type == "hello") {
        sendsign = 0x4;
        recvsign = 0x5;
    }
    else {
        sendsign = 0x2;
        recvsign = 0x3;
    }
    Header recvh;
    Header sendh;
    char* Buffer = new char[sizeof(recvh)];
    int res = 0;
    sendh.setHeader(0, sendsign, 0);//����flag
    setSum(sendh);//У�����0�����м���
    memcpy(Buffer, &sendh, sizeof(sendh));
    //���͵�һ��������Ϣ
    res = sendto(sockServ, Buffer, sizeof(recvh), 0, (sockaddr*)&ClientAddr, ClientAddrLen);

    if (res == -1) {
        cout << "first " << type << "----failed" << endl;
        return false;
    }
    else
        cout << "first " << type << " ----sended" << endl;
    long long head;
    QueryPerformanceCounter((LARGE_INTEGER*)&head);
    u_long mode = 1;
    ioctlsocket(server, FIONBIO, &mode);
    _sleep(300);
    while (recvfrom(sockServ, Buffer, sizeof(recvh), 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0) {
        if (time(head) > INTERACT_TIME) {
            memcpy(Buffer, &sendh, sizeof(sendh));
            sendto(sockServ, Buffer, sizeof(recvh), 0, (sockaddr*)&ClientAddr, ClientAddrLen);
            QueryPerformanceCounter((LARGE_INTEGER*)&head);
            cout << "time out for first " << type << ".resending......" << endl;
            continue;
        }
    }
    memcpy(&recvh, Buffer, sizeof(recvh));
    if (check_sign(recvh, recvsign)) {

        cout << "second " << type << " ----checked" << endl;
        cout << "----------------------------ok---------------------------------" << endl;
        return true;
    }
    else {
        cout << "----------------------------failed---------------------------------" << endl;
        return false;
    }
}
//����pkt���Ҵ�ӡpkt��header����Ϣ
void snd_pkt(Packet sendp) {
    sendto(server, sendp.Buffer, sizeof(sendp.header)+sendp.header.datasize, 0, (sockaddr*)&server_addr, addrlen);//�������ݰ�
    cout << "send " << sendp.header.datasize << " Byte��";
    sendp.header.show_header();
}
//�Ѷ�Ӧ��Header��databufд��pkt��buffer�У�����seqģ256�������У��͵ȷ���
void mk_pkt(Packet& sendp, bool END, char* databuf, int datatsize,int seq) {
    seq %= 256;
    sendp.header.seq = (u_char)seq;
    if (!END)
        sendp.header.flag = 0x0;
    else
        sendp.header.flag = 0x7;
    sendp.header.datasize = datatsize;
    memcpy(sendp.Buffer, &sendp.header, sizeof(sendp.header));
    memcpy(sendp.Buffer + sizeof(sendp.header), databuf, datatsize);
    u_short tempsum = cksum((u_short*)&sendp.Buffer, sizeof(sendp.Buffer));
    sendp.header.sum = tempsum;
    memcpy(sendp.Buffer, &sendp.header, sizeof(sendp.header));
}




void send(SOCKET& clientsocket, SOCKADDR_IN& serveraddr, int& addrlen, char* data, int len) {
    Header recvh;
    long long head;
    char* buf = new char[sizeof(recvh)];
    int num = 0;
    if (len % MAXSIZE == 0) {
        num = len / MAXSIZE;
    }
    else {
        num = len / MAXSIZE + 1;
    }
    cout << "packets:" << num << endl;
    Packet sendp;//�������seq��nextseqnum���ǵ�ǰҪ����
    //Packet* sndpkt = new Packet[num]; // there are num packets
    while (base < num-1) {//ǰ����maxsize�İ�����ô�����һ��������
        if (nextseqnum < base + SEND_WIND_SIZE ) {
            mk_pkt(sendp, nextseqnum == num - 1, data + nextseqnum * MAXSIZE, nextseqnum == num - 1 ? len - (num - 1) * MAXSIZE : MAXSIZE, nextseqnum % 256);
            snd_pkt(sendp);
            
            QueryPerformanceCounter((LARGE_INTEGER*)&head); 
            nextseqnum++;

        }
        u_long mode = 1;
        ioctlsocket(clientsocket, FIONBIO, &mode);

        if (recvfrom(clientsocket, buf, MAXSIZE, 0, (sockaddr*)&serveraddr, &addrlen)) {
            memcpy(&recvh, buf, sizeof(recvh));
            //������ackȫ�����ԣ�ֻ����base��ȵ�ack
            if (recvh.ack == base % 256) {
                //ȷ���˾��������������Ų��
                cout << "ack packet seq:" << nextseqnum << endl;
                //recvh.show_header();
                base ++;
                curAcked++;
                curAcked %= 256;
                

            }
        }
       
        if (time(head) > TIMEOUT) {
            nextseqnum = base ;

        }
        
        mode = 0;
        ioctlsocket(clientsocket, FIONBIO, &mode);

    }
    //�������һ����,����3-1����ͨ������
    sendp.header.setHeader(len - (num - 1) * MAXSIZE, 0x7, (nextseqnum % 256));
    mk_pkt(sendp, true, data + (num - 1) * MAXSIZE, len - (num - 1) * MAXSIZE, nextseqnum);
    snd_pkt(sendp);
    cout << "last send: ";
    QueryPerformanceCounter((LARGE_INTEGER*)&head);
    while (true) {
        u_long mode = 1;
        ioctlsocket(clientsocket, FIONBIO, &mode);
        while (recvfrom(clientsocket, buf, sizeof(recvh), 0, (sockaddr*)&serveraddr, &addrlen) <= 0) {
            if (time(head) > TIMEOUT) {//��ʱ�����´���
                cout << "time out! resend:" << endl;
                sendp.header.setHeader(len - (num - 1) * MAXSIZE, 0x7, (nextseqnum % 256));
                mk_pkt(sendp, true, data + (num - 1) * MAXSIZE, len - (num - 1) * MAXSIZE, nextseqnum);
                snd_pkt(sendp);
                QueryPerformanceCounter((LARGE_INTEGER*)&head);
                continue;
            }
        }
        memcpy(&recvh, buf, sizeof(recvh));
        if (recvh.flag) {
            curAcked = recvh.ack;
            //recvh.ack==
            if (ckend(recvh)) {
                cout << "server has received��" << endl;
                break;
            }
            else if (cksend(recvh)) {
                cout << "send ok!" << endl;
                break;
            }
            else {
                continue;
            }
        }

    }
    u_long mode = 0;
    ioctlsocket(clientsocket, FIONBIO, &mode);//�Ļ�����ģʽ
    nextseqnum = 0;
    base = 0;
    curAcked = -1;

    
}
void start(int len) {
    vector<string> fileNames;
    string path("D:\\111programs\\3-2client"); 	//�Լ�ѡ��Ŀ¼����

    vector<string> files;
    intptr_t hFile = 0;//�ļ����    
    struct _finddata_t fileinfo;//�ļ���Ϣ
    string p1, p2;
    if ((hFile = _findfirst(p1.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
    {
        do
        {
            if (!(fileinfo.attrib & _A_SUBDIR))
                files.push_back(fileinfo.name);

        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
    int k = 1;
    printsplit();
    cout << "files list��" << endl;
    for (auto f : files) {
        cout << k << ". ";
        cout << f << endl;
        k++;
    }
    int x;
    cout << "enter the number of the file" << endl;
    cin >> x;
    if (x) {
        string myfile = files[x - 1];
        cout << "starting��" << files[x - 1] << endl;
        ifstream fin(myfile.c_str(), ifstream::binary);//�Զ����Ʒ�ʽ���ļ�
        char* buffer = new char[100000000];
        int i = 0;
        u_short temp = fin.get();
        while (fin)
        {
            buffer[i++] = temp;
            temp = fin.get();
        }
        fin.close();
        long long head, tail, freq;
        send(server, server_addr, len, (char*)(myfile.c_str()), myfile.length());
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        QueryPerformanceCounter((LARGE_INTEGER*)&head);
        send(server, server_addr, len, buffer, i);
        QueryPerformanceCounter((LARGE_INTEGER*)&tail);
        cout << "������ʱ��Ϊ: " << (tail - head) * 1.0 / freq << " s" << endl;
        cout << "������Ϊ: " << ((double)i) / ((tail - head) * 1.0 / freq) << " byte/s" << endl;

    }
}



#endif

#pragma once


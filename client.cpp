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
/*/*��־λΪ�� |  2  |  1  |  0  |
*          ---------------------
*             | SYN | FIN | ACK |
*/


const int MAXSIZE = 1024;//���仺������󳤶�
const unsigned char SYN_1_ACK_0 = 0x4; //SYN = 1 ACK = 0
const unsigned char SYN_1_ACK_1 = 0x5;//SYN = 0, ACK = 1
const unsigned char SYN_0_ACK_1 = 0x1;
const unsigned char FIN_1_ACK_1 = 0x3;//SYN = 1, ACK = 1
const unsigned char FIN_1_ACK_0 = 0x2;//FIN = 1 ACK = 0
const unsigned char END = 0x7;//������־��SYN=1,FIN=1,ACK=1
double MAX_WAIT_TIME = 0.5 * CLOCKS_PER_SEC;
long long head, tail, freq;
void printsplit() {
    cout << "--------------------------------------------------------------------------" << endl;
}

/*
1.��α�ײ���ӵ�UDP�ϣ�
2.�����ʼʱ����Ҫ��������ֶ�����ģ�
3.������λ����Ϊ16λ��2�ֽڣ�����
4.������16λ������ӣ����������λ���򽫸���16�ֽڵĽ�λ���ֵ�ֵ�ӵ����λ�ϣ�������0xBB5E+0xFCED=0x1 B84B����1�ŵ����λ���õ������0xB84C
5.����������ӵõ��Ľ��Ӧ��Ϊһ��16λ������������ȡ������Եõ������checksum�� */
u_short cksum(u_short* mes, int size) {
    int count = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, mes, size);
    u_long sum = 0;
    while (count--) {
        sum += *buf++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}

struct HEADER
{
    u_short sum = 0;//У��� 16λ
    u_short datasize = 0;//���������ݳ��� 16λ
    unsigned char flag = 0;
    //��λ��ʹ�ú���λ��������FIN ACK SYN 
    unsigned char SEQ = 0;
    //��λ����������кţ�0~255��������mod
    HEADER() {
        sum = 0;//У��� 16λ
        datasize = 0;//���������ݳ��� 16λ
        flag = 0;
        //��λ��ʹ�ú���λ��������FIN ACK SYN 
        SEQ = 0;
    }
};

int Connect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)//�������ֽ�������
{
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    HEADER header;
    char* Buffer = new char[sizeof(header)];

    u_short sum;

    //���е�һ������
    header.flag = SYN_1_ACK_0;
    header.sum = 0;//У�����0
    u_short temp = cksum((u_short*)&header, sizeof(header));
    header.sum = temp;//����У���
    memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
    if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&head);
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);

    //���յڶ�������
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        QueryPerformanceCounter((LARGE_INTEGER*)&tail);
        if ((tail-head)/freq > MAX_WAIT_TIME)//��ʱ�����´����һ������
        {
            header.flag = SYN_1_ACK_0;
            header.sum = 0;//У�����0
            header.sum = cksum((u_short*)&header, sizeof(header));//����У���
            memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
            sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
            QueryPerformanceCounter((LARGE_INTEGER*)&head);
            cout << "time out for first hello. resending....." << endl;
        }
    }


    //����У��ͼ���
    memcpy(&header, Buffer, sizeof(header));
    if (header.flag == SYN_1_ACK_1 && cksum((u_short*)&header, sizeof(header) == 0))
    {
        cout << "second hello----check\nconnection succeeded" << endl;
    }
    else
    {
        cout << "error" << endl;
        return -1;
    }

    
}




void sendMessage(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* msg, int lenlen)
{
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    int packagenum = lenlen / MAXSIZE + (lenlen % MAXSIZE != 0);
    int seqnum = 0;
    for (int i = 0; i < packagenum; i++)
    {
       // send_package(socketClient, servAddr, servAddrlen, msg + i * MAXSIZE, i == packagenum - 1 ? lenlen - (packagenum - 1) * MAXSIZE : MAXSIZE, seqnum);
        char* message = msg + i * MAXSIZE;
        int len = (i == packagenum - 1 ? lenlen - (packagenum - 1) * MAXSIZE : MAXSIZE);
        int order = seqnum;
        HEADER header;
        char* buffer = new char[MAXSIZE + sizeof(header)];
        header.datasize = len;
        header.SEQ = unsigned char(order);//���к�
        memcpy(buffer, &header, sizeof(header));
        memcpy(buffer + sizeof(header), message, sizeof(header) + len);
        u_short check = cksum((u_short*)buffer, sizeof(header) + len);//����У���
        header.sum = check;
        memcpy(buffer, &header, sizeof(header));
        sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//����
        cout << "Send message " << len << " B!" << " flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << " SUM:" << int(header.sum) << endl;
        QueryPerformanceCounter((LARGE_INTEGER*)&head);
        //����ack����Ϣ
        while (1 == 1)
        {
            u_long mode = 1;
            ioctlsocket(socketClient, FIONBIO, &mode);
            while (recvfrom(socketClient, buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
            {
                QueryPerformanceCounter((LARGE_INTEGER*)&tail);
                if ((tail-head)/freq > MAX_WAIT_TIME)
                {
                    header.datasize = len;
                    header.SEQ = u_char(order);//���к�
                    header.flag = u_char(0x0);
                    memcpy(buffer, &header, sizeof(header));
                    memcpy(buffer + sizeof(header), message, sizeof(header) + len);
                    u_short check = cksum((u_short*)buffer, sizeof(header) + len);//����У���
                    header.sum = check;
                    memcpy(buffer, &header, sizeof(header));
                    sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//����
                    cout << "TIME OUT! ReSend message " << len << " bytes! Flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << endl;
                    QueryPerformanceCounter((LARGE_INTEGER*)&head);
                }
            }
            memcpy(&header, buffer, sizeof(header));//���������յ���Ϣ����ȡ
            u_short check = cksum((u_short*)&header, sizeof(header));
            if (header.SEQ == u_short(order) && header.flag == SYN_0_ACK_1)
            {
                cout << "Send has been confirmed! Flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << endl;
                break;
            }
            else
            {
                continue;
            }
        }
        u_long mode = 0;
        ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ
        seqnum++;
        if (seqnum > 255)
        {
            seqnum = seqnum - 256;
        }
    }
    //���ͽ�����Ϣ
    HEADER header;
    char* Buffer = new char[sizeof(header)];
    header.flag = END;
    header.sum = 0;
    u_short temp = cksum((u_short*)&header, sizeof(header));
    header.sum = temp;
    memcpy(Buffer, &header, sizeof(header));
    sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
    cout << "Send End!" << endl;
    QueryPerformanceCounter((LARGE_INTEGER*)&head);
    while (1 == 1)
    {
        u_long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
        while (recvfrom(socketClient, Buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
        {
            QueryPerformanceCounter((LARGE_INTEGER*)&tail);
            if ((tail-head)/freq > MAX_WAIT_TIME)
            {
                char* Buffer = new char[sizeof(header)];
                header.flag = END;
                header.sum = 0;
                u_short temp = cksum((u_short*)&header, sizeof(header));
                header.sum = temp;
                memcpy(Buffer, &header, sizeof(header));
                sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
                cout << "Time Out! ReSend End!" << endl;
                QueryPerformanceCounter((LARGE_INTEGER*)&head);
            }
        }
        memcpy(&header, Buffer, sizeof(header));//���������յ���Ϣ����ȡ
        u_short check = cksum((u_short*)&header, sizeof(header));
        if (header.flag == END)
        {
            cout << "�Է��ѳɹ������ļ�!" << endl;
            break;
        }
        else
        {
            continue;
        }
    }
    u_long mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ
}



int Disconnect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)
{
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    HEADER header;
    char* Buffer = new char[sizeof(header)];

    u_short sum;

    //���е�һ������
    header.flag = FIN_1_ACK_0;
    header.sum = 0;//У�����0
    u_short temp = cksum((u_short*)&header, sizeof(header));
    header.sum = temp;//����У���
    memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
    if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&head);
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);

    //���յڶ��λ���
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        QueryPerformanceCounter((LARGE_INTEGER*)&tail);
        if ((tail-head)/freq > MAX_WAIT_TIME)//��ʱ�����´����һ�λ���
        {
            header.flag = FIN_1_ACK_1;
            header.sum = 0;//У�����0
            header.sum = cksum((u_short*)&header, sizeof(header));//����У���
            memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
            sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
            QueryPerformanceCounter((LARGE_INTEGER*)&head);
            cout << "time out for first goodbye. resending....." << endl;
        }
    }


    //����У��ͼ���
    memcpy(&header, Buffer, sizeof(header));
    if (header.flag == FIN_1_ACK_1 && cksum((u_short*)&header, sizeof(header) == 0))
    {
        cout << "second goodbye----check" << endl;
    }
    else
    {
        cout << "error" << endl;
        return -1;
    }

    
    return 1;
}


int main()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN server_addr;
    SOCKET server;

    server_addr.sin_family = AF_INET;//ʹ��IPV4
    server_addr.sin_port = htons(2456);
    server_addr.sin_addr.s_addr = htonl(2130706433);

    server = socket(AF_INET, SOCK_DGRAM, 0);
    int len = sizeof(server_addr);
    //��������
    if (!Connect(server, server_addr, len) )
    {
        return 0;
    }
    vector<string> fileNames;
    string path("D:\\111programs\\����lab3-1_client"); 	//�Լ�ѡ��Ŀ¼����
    
    vector<string> files;
    intptr_t hFile = 0;//�ļ����    
    struct _finddata_t fileinfo;//�ļ���Ϣ
    string p1,p2;
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
    int x = 0;   
    cout << "enter the number of the file" << endl;
    cin >> x;
    if (x) {
        string myfile = files[x - 1];
        cout << "starting��" << files[x - 1] << endl;
        ifstream fin(myfile.c_str(), ifstream::binary);//�Զ����Ʒ�ʽ���ļ�
        char* buffer = new char[10000000];
        int i = 0;
        unsigned char temp = fin.get();
        while (fin)
        {
            buffer[i++] = temp;
            temp = fin.get();
        }
        fin.close();
        sendMessage(server, server_addr, len, (char*)(myfile.c_str()), myfile.length());
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        QueryPerformanceCounter((LARGE_INTEGER*)&head);
        sendMessage(server, server_addr, len, buffer, i);
        QueryPerformanceCounter((LARGE_INTEGER*)&tail);
        cout << "������ʱ��Ϊ: " << (tail - head) * 1000.0 / freq << " ms" << endl;
        cout << "������Ϊ: " << ((double)i) / ((tail - head) * 1000.0 / freq) << " byte/ms" << endl;

    }      
    Disconnect(server, server_addr, len);
}

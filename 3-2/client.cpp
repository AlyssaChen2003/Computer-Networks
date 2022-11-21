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


const int MAXSIZE = 2048;//���仺������󳤶�
const u_short SYN_1_ACK_0 = 0x4; //SYN = 1 ACK = 0
const u_short SYN_1_ACK_1 = 0x5;//SYN = 0, ACK = 1
const u_short SYN_0_ACK_1 = 0x1;
const u_short FIN_1_ACK_1 = 0x3;//SYN = 1, ACK = 1
const u_short FIN_1_ACK_0 = 0x2;//FIN = 1 ACK = 0
const u_short END = 0x7;//������־��SYN=1,FIN=1,ACK=1
double MAX_WAIT_TIME = 0.5 * CLOCKS_PER_SEC;
int wds = 10;
void printsplit() {
    cout << "--------------------------------------------------------------------------" << endl;
}
u_short cksum(u_short* mes, int size) {
    int count = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1); //fill with 0
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

struct HEADER {
    u_short datasize = 0;//���������ݳ��� 16λ
    u_short sum = 0;//У��� 16λ
    u_short flag = 0;//��־λ 16λ
    u_short SEQ = 0; //���к� 16λ
    HEADER() {
        sum = 0;
        datasize = 0;
        flag = 0;
        SEQ = 0;
    }
};

bool Connect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen) {//�������ֽ�������

    HEADER header;
    char* buff = new char[sizeof(header)];
    u_short sum;
    //���е�һ������
    header.flag = SYN_1_ACK_0;
    header.sum = 0;//У�����0
    header.sum = cksum((u_short*)&header, sizeof(header));
    memcpy(buff, &header, sizeof(header));//���ײ����뻺����
    if (sendto(socketClient, buff, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        return false;
    }
    clock_t start = clock();
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);//��ֹ�߳�����

    //���յڶ�������
    while (recvfrom(socketClient, buff, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {

        if (clock() - start > MAX_WAIT_TIME) {//��ʱ�����´����һ������
            header.flag = SYN_1_ACK_0;
            header.sum = 0;//У�����0
            header.sum = cksum((u_short*)&header, sizeof(header));//����У���
            memcpy(buff, &header, sizeof(header));//���ײ����뻺����
            sendto(socketClient, buff, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "time out for first hello. resending....." << endl;
        }
    }


    //����У��ͼ���
    memcpy(&header, buff, sizeof(header));
    if (header.flag == SYN_1_ACK_1 && cksum((u_short*)&header, sizeof(header) == 0)) {
        cout << "second hello----check\nconnection succeeded" << endl;
    }
    else {
        cout << "error" << endl;
        return false;
    }
    return true;

}




void sendMessage(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* msg, int lenlen)
{

    HEADER header;
    char* Buffer = new char[sizeof(header)];
    int packagenum = lenlen / MAXSIZE + (lenlen % MAXSIZE != 0);
    int head = -1;//������ͷ����ǰ��Ϊ�Ѿ���ȷ�ϵı���
    int tail = 0;//������β��
    int index = 0;
    clock_t start;
    cout << packagenum << endl;
    while (head < packagenum - 1){
        if (tail - head < wds && tail != packagenum){
            //cout << msg + tail * MAXSIZE << endl;
            //send_package(socketClient, servAddr, servAddrlen, msg + tail * MAXSIZE, tail == packagenum - 1 ? lenlen - (packagenum - 1) * MAXSIZE : MAXSIZE, tail % 256);
            int len = (tail == packagenum - 1 ? lenlen - (packagenum - 1) * MAXSIZE : MAXSIZE);
            int seq = tail % 256;
            char* message = msg + tail * MAXSIZE;
            HEADER header;
            char* buffer = new char[MAXSIZE + sizeof(header)];
            header.datasize = len;
            header.SEQ = u_short(seq);//���к�
            memcpy(buffer, &header, sizeof(header));
            memcpy(buffer + sizeof(header), message, sizeof(header) + len);
            u_short check = cksum((u_short*)buffer, sizeof(header) + len);//����У���
            header.sum = check;
            memcpy(buffer, &header, sizeof(header));
            sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//����
            printsplit();
            cout << "Send message " << len << " B!" << " flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << " SUM:" << int(header.sum) << endl;
            start = clock();//��¼����ʱ��
            tail++;
        }

        //��Ϊ������ģʽ
        u_long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
        if (recvfrom(socketClient, Buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen))
        {
            //cout << seqnum<<" " <<package_send << endl;
            memcpy(&header, Buffer, sizeof(header));//���������յ���Ϣ����ȡ
            u_short check = cksum((u_short*)&header, sizeof(header));
            if (int(check) != 0 || header.flag != SYN_0_ACK_1)
            {
                tail = head + 1;
                cout << " hah" << endl;
                continue;
            }
            else
            {
                if (int(header.SEQ) >= head % 256)
                {
                    head = head + int(header.SEQ) - head % 256;
                    printsplit();
                    cout << "Send has been confirmed! Flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << endl;
                }
                else if (head % 256 > 256 - wds - 1 && int(header.SEQ) < wds)
                {
                    head = head + 256 - head % 256 + int(header.SEQ);
                    printsplit();
                    cout << "Send has been confirmed! Flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << endl;
                }
            }
        }
        else
        {
            if (clock() - start > MAX_WAIT_TIME)
            {
                tail = head + 1;
                cout << "Re";
            }
        }
        mode = 0;
        ioctlsocket(socketClient, FIONBIO, &mode);
    }
    //���ͽ�����Ϣ

    header.flag = END;
    header.sum = 0;
    header.sum = cksum((u_short*)&header, sizeof(header));
    memcpy(Buffer, &header, sizeof(header));
    sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
    cout << "Send End!" << endl;
    start = clock();
    while (1) {
        u_long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
        while (recvfrom(socketClient, Buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
        {

            if (clock() - start > MAX_WAIT_TIME)
            {
                char* Buffer = new char[sizeof(header)];
                header.flag = END;
                header.sum = 0;
                header.sum = cksum((u_short*)&header, sizeof(header));
                memcpy(Buffer, &header, sizeof(header));
                sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
                printsplit();
                cout << "Time Out! ReSend End!" << endl;
                start = clock();
            }
        }
        memcpy(&header, Buffer, sizeof(header));//���������յ���Ϣ����ȡ
        u_short check = cksum((u_short*)&header, sizeof(header));
        if (header.flag == END) {
            cout << "server has received the file!" << endl;
            break;
        }
        else {
            continue;
        }
    }
    u_long mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ
}



bool Disconnect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen) {

    HEADER header;
    char* Buffer = new char[sizeof(header)];

    u_short sum;

    //���е�һ�λ���
    header.flag = FIN_1_ACK_0;
    header.sum = 0;//У�����0
    header.sum = cksum((u_short*)&header, sizeof(header));
    memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
    if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        return false;
    }
    clock_t start = clock();
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);

    //���յڶ��λ���
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
        if (clock() - start > MAX_WAIT_TIME) {//��ʱ�����´����һ�λ���
            header.flag = FIN_1_ACK_1;
            header.sum = 0;//У�����0
            header.sum = cksum((u_short*)&header, sizeof(header));//����У���
            memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
            sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
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
        return false;
    }


    return true;
}


int main() {
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN server_addr;
    SOCKET server;

    server_addr.sin_family = AF_INET;//ʹ��IPV4
    server_addr.sin_port = htons(4001);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    server = socket(AF_INET, SOCK_DGRAM, 0);
    int len = sizeof(server_addr);
    //��������
    if (!Connect(server, server_addr, len))
    {
        return 0;
    }
    vector<string> fileNames;
    string path("D:\\111programs\\����lab3-1_client"); 	//�Լ�ѡ��Ŀ¼����

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
    int x = 0;
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
        sendMessage(server, server_addr, len, (char*)(myfile.c_str()), myfile.length());
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        QueryPerformanceCounter((LARGE_INTEGER*)&head);
        sendMessage(server, server_addr, len, buffer, i);
        QueryPerformanceCounter((LARGE_INTEGER*)&tail);
        cout << "������ʱ��Ϊ: " << (tail - head) * 1.0 / freq << " s" << endl;
        cout << "������Ϊ: " << ((double)i) / ((tail - head) * 1.0 / freq) << " byte/s" << endl;

    }
    Disconnect(server, server_addr, len);
}

#include<winsock2.h>//winsock2��ͷ�ļ�
#include<iostream>
#include<time.h> 
#include <sstream>
using  namespace std;
#pragma comment(lib, "ws2_32.lib")
string TimeNow() {
	time_t now = time(NULL);
	tm* tm_t = localtime(&now);
	std::stringstream ss; //��ʱ��ת��Ϊ�ַ����Ž�ss��
	ss << "[";
	if (tm_t->tm_hour < 10)
		ss << "0";
	ss << tm_t->tm_hour << ":";
	if (tm_t->tm_min < 10)
		ss << "0";
	ss << tm_t->tm_min << ":";
	if (tm_t->tm_sec < 10)
		ss << "0";
	ss << tm_t->tm_sec << "]";
	string str;
	ss >> str; //��stringstream�Ž�str��
	//char* p = const_cast<char*>(str.c_str()); //��strת��Ϊchar*����
	return str;
}
int  main(){
	//����winsock2�Ļ���
	WSADATA  wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0){
		cout << TimeNow();
		cout << "WSAStartup����" << GetLastError() << endl;
		return 0;
	}

	//������ʽ�׽���
	SOCKET  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET){
		cout << TimeNow();
		cout << "socket����" << GetLastError() << endl;
		return 0;
	}

	//���ӷ�����
	sockaddr_in   addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int len = sizeof(sockaddr_in);
	if (connect(s, (SOCKADDR*)&addr, len) == SOCKET_ERROR){//�����˷��ͽ�������
		cout << TimeNow();
		cout << "���Ӵ���" << GetLastError() << endl;
		return 0;
	}

	//���շ���˵���Ϣ
	char buf[1000] = { 0 };
	recv(s, buf, 1000, 0);
	cout << buf << endl;

	//��ʱ������˷���Ϣ
	int  ret = 0;
	int flag = 1; //�Ƿ��˳��ı��
	do{
		char client_buf[1000] = { 0 };
		cout << "��������Ϣ����:";
		cin.getline(client_buf,1000);
		if (client_buf[0] == 'q') {
			flag = 0; //�������q������ѭ��
		}
		if(flag==1)
		ret = send(s, client_buf, 1000, 0);
	} while (ret != SOCKET_ERROR && ret != 0 && flag==1);

	//�رռ����׽��ֲ�����winsock2�Ļ���
	closesocket(s);
	WSACleanup();
	return 0;
}

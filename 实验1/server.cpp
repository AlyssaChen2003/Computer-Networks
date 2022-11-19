#include <winsock2.h> // winsock2��ͷ�ļ�
#include <iostream>
#include<time.h> 
#include<string>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")
using namespace std; 
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

DWORD WINAPI ThreadFun(LPVOID lpThreadParameter) {
	// ��ͻ���ͨѶ�����ͻ��߽�������
	SOCKET c = (SOCKET)lpThreadParameter;//���ܴ������̲߳�����ʵ�����������е�client���socket
	// ������ͻ������ӳɹ�����Ϣ
	char buf[1000] = { 0 };
	sprintf_s(buf, "���ӳɹ�����ӭ�û� %d ���������ң����˳������밴q��", c);
	send(c, buf, 1000, 0);

	// ѭ�����տͻ�������
	int ret = 0;
	do {
		char server_buf[1000] = { 0 };
		ret = recv(c, server_buf, 1000, 0);//���ܿͻ��˷������ַ���Ϣ

		cout << "================================================" << endl;
		cout << TimeNow();
		cout << "�û�" << c << ":    " << server_buf << endl;//��ӡ��Ϣ����

	} while (ret != SOCKET_ERROR && ret != 0);
	cout << "================================================" << endl;
	cout << endl;
	cout << endl;
	cout << endl;
	cout << TimeNow();
	cout << "�û�" << c << "�뿪�������ң�" << endl;;

	return 0;
}

int main(){
	WSADATA wsaData;//��ʼ��Socket DLL��Э��ʹ�õ�Socket�汾
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){ //MAKEWORD(2, 2)���� WSAStartup �����İ汾�����֡�
		cout << TimeNow();
		cout << "WSAStartup Error:" << WSAGetLastError() << endl;
		return 0;
	}
	else{
		cout << TimeNow();
		cout << "��ʼ��SocketDLL�ɹ�!" << endl;
	}

	// ������ʽ�׽���
	cout << TimeNow();
	cout << "���ڴ�����ʽ�׽���......" << endl;
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//ָ��������socket�ĵ�ַ���͡��������ͼ�TCPЭ��
	
	if (s == INVALID_SOCKET){
		cout << TimeNow();
		cout << "socket����:" << WSAGetLastError() << endl;
		return 0;
	}
	else {
		cout << TimeNow();
		cout << "�����ɹ�" << endl;
	}

	// �󶨶˿ں�ip
	cout << TimeNow();
	cout << "���ڰ󶨶˿ں�IP" << endl;
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;//��ַ����
	addr.sin_port = htons(8000);//�˿ں�
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP��ַ

	int len = sizeof(sockaddr_in);
	if (bind(s, (SOCKADDR*)&addr, len) == SOCKET_ERROR){//�����ص�ַ�󶨵�s���socket
		cout << TimeNow();
		cout << "�󶨴���:" << WSAGetLastError() << endl;
		return 0;
	}
	else{
		cout << TimeNow();
		cout << "�˿ںţ�8000 IP��127.0.0.1�󶨳ɹ���" << endl;

	}
	
	// ����Զ�������Ƿ���
	listen(s, 5);
	cout << TimeNow();
	cout << "�ȴ��û�����������......" << endl;
	// ���߳�ѭ�����տͻ��˵�����
	while (true){
		sockaddr_in addrClient; //Զ�̶˵ĵ�ַ
		len = sizeof(sockaddr_in);
		// ���ܳɹ�������clientͨѶ��Socket
		SOCKET client = accept(s, (SOCKADDR*)&addrClient, &len); //����server����ȴ������е���������client
		if (client != INVALID_SOCKET)
		{
			cout << TimeNow();
			cout << "�û�" << client << "���ӳɹ���" << endl;
			// �����̣߳����Ҵ�����clientͨѶ���׽���
			HANDLE hThread = CreateThread(NULL, 0, ThreadFun, (LPVOID)client, 0, NULL);
			if(!hThread)
				CloseHandle(hThread); // �رն��̵߳�����
		}

	}

	// �رռ����׽��ֲ�����winsock2�Ļ���
	closesocket(s);
	WSACleanup();
	return 0;
}



#include "myclient.h"
using namespace std;
int main() {
    init();
    int len = sizeof(server_addr);
    /*��������*/
    if (!interact(server, server_addr, len, "hello"))//����ʧ�����˳�
    {
        return 0;
    }
    start(len);//��ʼ�����ļ�
    _sleep(5 * 1000);
    interact(server, server_addr, len, "goodbye");//�����˳�
}




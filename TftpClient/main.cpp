#include<winsock.h>
#include<Windows.h>
#include<iostream>
#include<cstring>
#include<ctime>
#include<vector>
#include<string>
#include<sstream>
#include<fstream>
#include<memory>
#pragma comment(lib,"ws2_32.lib")



//����
const int NETASCII = 0;
const int OCTET = 1;
const int OP_RRQ = 0x01;
const int OP_WRQ = 0x02;
const int OP_DATA = 0x03;
const int OP_ACK = 0x04;
const int OP_ERROR = 0x05;
const int UNWANTED_MESSAGE = 2;
const int RETRANS_RRQ = 2;
const int RETRANS_WRQ = 2;
const int SEND_DATA_SIZE = 100*1024*1024;
const int OVERTIME_SEC = 1000;		//1000=1s
const int OVERTIME_USEC = 0;
const int LOCAL_PORT = 5055;

//����ԭ��
int Upload(char* ip,int port,int mode);
int sendWRQ(SOCKET & sock,SOCKADDR_IN & addr,char* filename,int filenamelen,int mode);
int revACK(SOCKET& sock, SOCKADDR_IN& addr, int expected_ack);
int sendData(SOCKET& sock, SOCKADDR_IN& addr, char* sendbuf, int length);
int Download(char* ip, int port, int mode);
int sendRRQ(SOCKET& sock, SOCKADDR_IN& addr, char* filename, int filenamelen, int mode);
int sendACK(SOCKET& sock, SOCKADDR_IN& addr, int ack);
int revDATA(SOCKET& sock, SOCKADDR_IN& addr, int& file_size,std::string & filename);

//������־
struct log {
	struct tm t;
	std::string action;
};
std::vector<struct log> Log;

int main(void)
{
	using std::cout;
	using std::cin;
	using std::endl;
	//��ʼ��winsock
	WSADATA wsadata;
	int nRc = WSAStartup(0x0101, &wsadata);
	if (nRc) {
		cout << "Winsock��ʼ��ʧ�ܣ�" << endl;
		system("pause");
		return 0;
	}
	int result;
	//��ӡ����
	while (1)
	{
		system("cls");
		cout << "***********************************TFTP�ͻ���***********************************" << endl << endl;
		cout << "*******************************�����Ӧ����ѡ����*****************************" << endl << endl;
		cout << "************0.�˳�" << endl;
		cout << "************1.�ϴ���netascii��" << endl;
		cout << "************2.���أ�netascii��" << endl;
		cout << "************3.�ϴ���octet��" << endl;
		cout << "************4.���أ�octet��" << endl;
		cout << "************5.��ʾ��־" << endl;
		cout << "************6.������־" << endl;
		int func;
		cin >> func;
		switch (func)
		{
			case 0:
			{
				//�˳�����
				return 0;
			}
			case 1:
			{
				//�ϴ�
				system("cls");
				cout << "*****�ϴ���netascii��*****" << endl;
				char ip[] = "127.0.0.1";
				result = Upload(ip, 69, NETASCII);
				//test();
				cout << endl;
				system("pause");
				break;
			}
			case 2:
			{
				//����
				system("cls");
				cout << "*****���أ�netascii��*****" << endl;
				char ip[] = "127.0.0.1";
				result = Download(ip, 69, NETASCII);
				cout << endl;
				system("pause");
				break;
			}
			case 3:
			{
				//�ϴ�
				system("cls");
				cout << "*****�ϴ���octet��*****" << endl;
				char ip[] = "127.0.0.1";
				result = Upload(ip, 69, OCTET);
				cout << endl;
				system("pause");
				break;
			}
			case 4:
			{
				//����
				system("cls");
				cout << "*****���أ�octet��*****" << endl;
				char ip[] = "127.0.0.1";
				result = Download(ip, 69, OCTET);
				cout << endl;
				system("pause");
				break;
			}
			case 5:
			{
				//��ӡ��־
				system("cls");
				cout << "*****��־*****" << endl;
				int n=1;
				for (struct log x : Log)
				{
					cout << n << "��" << endl;;
					cout << "  ʱ�䣺" << x.t.tm_year + 1900 << "��" << x.t.tm_mon + 1 << "��" << x.t.tm_mday << "��" << x.t.tm_hour << "ʱ" << x.t.tm_min << "��" << x.t.tm_sec << "��" << endl;
					cout << "  ������" << x.action << endl<<endl;
					n++;
				}
				cout << endl;
				system("pause");
				break;
			}
			case 6:
			{
				//������־
				system("cls");
				cout << "*****������־*****" << endl << endl << endl;
				std::ofstream fout("tftpclient_log.txt", std::ios_base::out | std::ios_base::app);
				int n=1;
				fout << endl << endl << endl << endl << endl;
				for (struct log x : Log)
				{
					fout << n << "��" << endl;;
					fout << "  ʱ�䣺" << x.t.tm_year + 1900 << "��" << x.t.tm_mon + 1 << "��" << x.t.tm_mday << "��" << x.t.tm_hour << "ʱ" << x.t.tm_min << "��" << x.t.tm_sec << "��" << endl;
					fout << "  ������" << x.action << endl << endl;
					n++;
				}
				cout << "*****������־�ɹ���*****" << endl;
				system("pause");
				break;
			}
			default:
			{
				//�����Ϸ�������
				cout << "���벻�Ϸ���" << endl;
				cout << endl;
				system("pause");
				continue;
			}
		}
	}
}

//�ϴ���������
int Upload(char* ip,int port,int mode)
{
	using std::cout;
	using std::endl;
	using std::cin;

	//׼����־
	auto t = time(nullptr);
	auto now = localtime(&t);
	struct log newlog;
	newlog.t = *now;
	std::string& log_content = newlog.action;
	std::ostringstream logstr;
	logstr << "�ϴ�";
	if (mode == NETASCII)
		logstr << "    ģʽ��netascii    ";
	else
		logstr << "    ģʽ��octet    ";

	//׼��Ŀ�ĵ�ַ���ݽṹ
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	//�����׽���
	SOCKET sServSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sServSock == INVALID_SOCKET)
	{
		cout << "*�׽��ִ���ʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError()<<endl;
		logstr << "�����ʧ��    ԭ���׽��ִ���ʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		return 1;
	}
	else
	{
		cout << "*�׽��ִ����ɹ���" << endl;
	}

	//��������ģʽ
	unsigned long socket_mode = 0;		//0������1������
	if (ioctlsocket(sServSock, FIONBIO, &socket_mode) == SOCKET_ERROR)
	{
		cout << "*�׽�����������ģʽʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ���׽�����������ģʽʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*�׽�����������ģʽ�ɹ���" << endl;
	}
	
	//���ó�ʱʱ��
	timeval tv{ OVERTIME_SEC,OVERTIME_USEC };
	if (setsockopt(sServSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(timeval)) == SOCKET_ERROR)
	{
		cout << "*recvfrom���ó�ʱʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ��recvfrom���ó�ʱʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*recvfrom���ó�ʱ�ɹ���" << endl;
	}
	

	//�󶨱�����ַ
	SOCKADDR_IN addr_local;
	addr_local.sin_family = AF_INET;
	addr_local.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr_local.sin_port = htons(LOCAL_PORT);
	if (bind(sServSock, (sockaddr*)&addr_local, sizeof(addr_local)) == SOCKET_ERROR)
	{
		cout << "*��������ַʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ�򣺰�������ַʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*��������ַ�ɹ���" << endl;
	}

	//��ȡ�ļ�����
	std::string name;
	cout << "����Ҫ�ϴ����ļ���·����";
	cin >> name;
	std::ifstream fin(name, std::ios_base::in | std::ios_base::binary);
	struct stat statbuf;
	stat(name.c_str(), &statbuf);
	int length_of_sendbuf = statbuf.st_size;
	std::unique_ptr<char[]> sendbuf(new char[statbuf.st_size]);		//ʹ������ָ��
	cout << length_of_sendbuf << endl;
	cout << statbuf.st_size << endl;
	fin.read(sendbuf.get(), length_of_sendbuf);


	//�������ڷ��������ļ���
	std::ostringstream filenamestr;
	filenamestr << (*now).tm_mday << (*now).tm_hour << (*now).tm_min << (*now).tm_sec << "_upload";
	char filename[40];
	strcpy(filename, filenamestr.str().c_str());
	logstr << "�ļ�����" << filenamestr.str() << "    ";
	cout << "*�ϴ��ļ�" << filenamestr.str() << endl;

	//������ʱ��
	auto start = clock();

	//����WRQ����
	int send_outcome = sendWRQ(sServSock, addr, filename, strlen(filename), mode);
	if (send_outcome == SOCKET_ERROR)
	{
		cout << "*WRQ���ķ���ʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ��WRQ���ķ���ʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*WRQ���ķ��ͳɹ���" << endl;
	}

	//����ACK=0
	int outcome;
	while (1)
	{
		outcome = revACK(sServSock, addr, 0);
		if (outcome == UNWANTED_MESSAGE)
		{
			continue;
		}
		else if ((outcome == 1)&&(WSAGetLastError() == 10060))
		{
			//�ش�WRQ
			cout << "*����ACK=0��ʱ��" << endl;
			cout << "*�ش�WRQ����" << endl;
			send_outcome = sendWRQ(sServSock, addr, filename, strlen(filename), mode);
		}
		else if (outcome==1)
		{
			cout << "*����ACK=0ʧ�ܣ�" << endl;
			cout << "*���һ�εĴ�����룺";
			cout << WSAGetLastError() << endl;
			logstr << "�����ʧ��    ԭ�򣺽���ACK=0ʧ��";
			log_content = logstr.str();
			Log.push_back(newlog);
			closesocket(sServSock);
			return 1;
		}
		else
		{
			cout << "*�ɹ�����ACK=0��" << endl;
			break;
		}
	}

	//�������ݿ�
	if (sendData(sServSock, addr, sendbuf.get(), length_of_sendbuf) == 0)
	{
		cout << "*���ݷ��ͳɹ���" << endl;
	}
	else
	{
		cout << "*���ݷ���ʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ�����ݷ���ʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}

	//������ʱ��
	auto end = clock();
	double duration = double(end - start) / CLOCKS_PER_SEC;
	double speed = SEND_DATA_SIZE / 1024 / duration;
	cout << "**����ʱ��Ϊ��" << duration << "��" << endl;
	cout << "**����" << length_of_sendbuf << "��������" << endl;
	cout << "**��������Ϊ��" << speed << "KBytes/sec" << endl;
	logstr << "������ɹ�    �ļ���С��" << length_of_sendbuf << "Bytes    ���ʣ�" << speed << "KB / s";
	log_content = logstr.str();
	Log.push_back(newlog);
	closesocket(sServSock);
	return 0;
}

//����WRQ
int sendWRQ(SOCKET & sock, SOCKADDR_IN & addr,char* filename,int filenamelen,int mode)
{
	//��ʼ�����Ļ�����
	char data[50];
	memset(data, 0, sizeof(data));
	//ǰ���ֽ�Ϊ�����룬2����WRQ
	data[1] = OP_WRQ;
	//֮��Ϊ�ļ���
	strcpy(data + 2, filename);
	//֮��Ϊ����ģʽ
	if (mode == NETASCII)
	{
		strcpy(data + 2 + filenamelen+1, "netascii");				//Ҫ���ļ���֮��Ŀ��ַ������ȥ
	}
	else
	{
		strcpy(data + 2 + filenamelen + 1, "octet");
	}
	return sendto(sock, data, sizeof(data), 0, (LPSOCKADDR)&addr, sizeof(addr));
}

//����ACK
int revACK(SOCKET & sock,SOCKADDR_IN & addr,int expected_ack)
{
	using std::cout;
	using std::endl;
	char data[40];				//ACK����4�ֽڣ�ΪERROR���յ������౨�������ռ�
	int sizeof_addr = sizeof(addr);
	int outcome = recvfrom(sock, data, sizeof(data), 0, (LPSOCKADDR)&addr, &sizeof_addr);
	if (outcome == SOCKET_ERROR)
	{
		return 1;
	}
	unsigned short n;
	memcpy(&n, data + 2, 2);		//ȡ������ack�ֶ�
	if ((data[1] == OP_ACK)&&(ntohs(n) == unsigned short(expected_ack)))

	{
		return 0;
	}
	else if (data[1] == OP_ERROR)
	{
		//�յ�ERROR����
		cout << "�յ�ERROR���ģ�������Ϊ";
		cout << int(data[3]) << endl;
		return 1;
	}
	else
	{
		//�յ��˷�ACK��ERROR���ģ��������ܺ�������
		return UNWANTED_MESSAGE;
	}
}

//�������ݲ�����ACK
int sendData(SOCKET& sock, SOCKADDR_IN& addr,char* sendbuf,int length)
{
	using std::cout;
	using std::endl;
	char data[516];		//ǰ���ֽ�Ϊ������3������ֽ�Ϊ���ݿ��ţ����512�ֽ�Ϊ���ݿ�
	memset(data, 0, sizeof(data));
	data[1] = OP_DATA;
	int blockNum = 1;
	int rounds = length / 512 + 1;
	int lastround = length % 512;
	int send_outcome;
	int rev_outcome;
	unsigned short num;
	for (int i = 1; i <= rounds; i++)
	{
		//��������
		num = htons(i);
		memcpy(data + 2, &num, 2);
		//data[3] = i;
		if (i == rounds)
		{
			//���һ��
			memcpy(data + 4, sendbuf+(i-1)*512, lastround);
			send_outcome = sendto(sock, data, lastround + 4, 0, (LPSOCKADDR)&addr, sizeof(addr));
		}
		else
		{
			memcpy(data + 4, sendbuf + (i - 1) * 512, 512);
			send_outcome = sendto(sock, data, 516, 0, (LPSOCKADDR)&addr, sizeof(addr));
		}
		if (send_outcome == SOCKET_ERROR)
		{
			cout << "���͵�" << i << "�����ݿ�ʧ��!" << endl;
			return 1;
		}
		else
		{
			cout<< "���͵�" << i << "�����ݿ�ɹ�!" << endl;
		}

		//����ACK=i
		rev_outcome = revACK(sock, addr, i);
		//����
		while ((rev_outcome == UNWANTED_MESSAGE) || (rev_outcome == 1))
		{
			if (rev_outcome == UNWANTED_MESSAGE)
			{
				rev_outcome = revACK(sock, addr, i);
				continue;
			}
			else if ((i == rounds) && (WSAGetLastError() == 10054))
			{
				//���һ��ACK���ճ�ʱ���������ӱ�������ǿ�йر�
				cout << "�������һ����" << i << "��ACK��ʱ�����ӱ�������ǿ�йرգ������������Ѿ����յ������һ�����ݿ飡" << endl;
				break;
			}
			else if (WSAGetLastError() == 10060)
			{
				//��ʱ
				cout << "���յ�" << i << "�����ݿ��ACK��ʱ��" << endl;
				//��������һ���ش�����
				i--;
				break;
			}
			else
			{
				cout << "���յ�" << i << "�����ݿ��ACKʧ�ܣ�" << endl;
				return 1;
			}
		}
		//���յ���ӦACK
		if (rev_outcome == 0)
		{
			cout << "���յ��Ե�" << i << "�����ݿ��ACK��" << endl;
		}
	}
	return 0;
	
}

//�ӷ���������
int Download(char* ip, int port, int mode)
{
	using std::cout;
	using std::endl;
	using std::cin;

	//׼����־
	auto t = time(nullptr);
	auto now = localtime(&t);
	struct log newlog;
	newlog.t = *now;
	std::string& log_content = newlog.action;
	std::ostringstream logstr;
	logstr << "����    ";
	if (mode == NETASCII)
		logstr << "ģʽ��netascii    ";
	else
		logstr << "ģʽ��octet    ";

	//׼����������ַ���ݽṹ
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	//�����׽���
	SOCKET sServSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sServSock == INVALID_SOCKET)
	{
		cout << "*�׽��ִ���ʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ���׽��ִ���ʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		return 1;
	}
	else
	{
		cout << "*�׽��ִ����ɹ���" << endl;
	}

	//��������ģʽ
	unsigned long socket_mode = 0;		//0������1������
	if (ioctlsocket(sServSock, FIONBIO, &socket_mode) == SOCKET_ERROR)
	{
		cout << "*�׽�����������ģʽʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ���׽�����������ģʽʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*�׽�����������ģʽ�ɹ���" << endl;
	}

	//���ó�ʱʱ��
	timeval tv{ OVERTIME_SEC,OVERTIME_USEC };
	if (setsockopt(sServSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(timeval)) == SOCKET_ERROR)
	{
		cout << "*recvfrom���ó�ʱʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ��recvfrom���ó�ʱʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*recvfrom���ó�ʱ�ɹ���" << endl;
	}
	

	//�󶨱�����ַ
	SOCKADDR_IN addr_local;
	addr_local.sin_family = AF_INET;
	addr_local.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr_local.sin_port = htons(LOCAL_PORT);
	if (bind(sServSock, (sockaddr*)&addr_local, sizeof(addr_local)) == SOCKET_ERROR)
	{
		cout << "*��������ַʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ�򣺰�������ַʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*��������ַ�ɹ���" << endl;
	}

	std::string filenamestr;
	cout << "����Ҫ���ص��ļ�����";
	cin >> filenamestr;
	char filename[20];
	strcpy(filename, filenamestr.c_str());
	logstr << "�ļ�����" << filenamestr << "    ";
	cout << "*�����ļ�" << filenamestr << endl;

	//������ʱ��
	auto start = clock();

	//����RRQ����
	if (sendRRQ(sServSock, addr, filename, strlen(filename), mode) == SOCKET_ERROR)
	{
		cout << "*����RRQ����ʧ�ܣ�" << endl;
		cout << "*���һ�εĴ�����룺";
		cout << WSAGetLastError() << endl;
		logstr << "�����ʧ��    ԭ�򣺷���RRQ����ʧ��";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout<< "*����RRQ���ĳɹ���" << endl;
	}

	//����DATA=i��������ACK=i
	int file_size = 0;
	int rev_outcome = revDATA(sServSock, addr, file_size, filenamestr);
	while (rev_outcome != 0)
	{
		if (rev_outcome == RETRANS_RRQ)
		{
			//�ط�RRQ
			if (sendRRQ(sServSock, addr, filename, strlen(filename), mode) == SOCKET_ERROR)
			{
				cout << "*�ط�RRQ����ʧ�ܣ�" << endl;
				cout << "*���һ�εĴ�����룺";
				cout << WSAGetLastError() << endl;
				logstr << "�����ʧ��    ԭ���ط�RRQ����ʧ��";
				log_content = logstr.str();
				Log.push_back(newlog);
				closesocket(sServSock);
				return 1;
			}
			else
			{
				cout << "*�ط�RRQ���ĳɹ���" << endl;
				rev_outcome = revDATA(sServSock, addr, file_size, filenamestr);
				continue;
			}
		}
		else
		{
			cout << "*��������ʧ�ܣ�" << endl;
			cout << "*���һ�εĴ�����룺";
			cout << WSAGetLastError() << endl;
			logstr << "�����ʧ��    ԭ�򣺽�������ʧ��";
			log_content = logstr.str();
			Log.push_back(newlog);
			closesocket(sServSock);
			return 1;
		}
	}
	//outcome==0,���ճɹ�
	//ֹͣ��ʱ��
	auto end = clock();
	double duration = double(end - start) / CLOCKS_PER_SEC;
	double speed = file_size / 1024 / duration;

	cout << "*�������ݳɹ���" << endl;
	cout << "**���յ��ı�������" << file_size << "Bytes" << endl;
	cout << "**�������ʣ�" << speed << "KB/s" << endl;
	logstr << "������ɹ�    �ļ���С��" << file_size << "Bytes    �������ʣ�" << speed << "KB/s";
	log_content = logstr.str();
	Log.push_back(newlog);
	closesocket(sServSock);

	return 0;
}

//RRQ
int sendRRQ(SOCKET& sock, SOCKADDR_IN& addr, char* filename, int filenamelen, int mode)
{
	//��ʼ�����Ļ�����
	char data[50];
	memset(data, 0, sizeof(data));
	//ǰ���ֽ�Ϊ�����룬2����WRQ
	data[1] = OP_RRQ;
	//֮��Ϊ�ļ���
	strcpy(data + 2, filename);
	//֮��Ϊ����ģʽ
	if (mode == NETASCII)
	{
		strcpy(data + 2 + filenamelen + 1, "netascii");				//Ҫ���ļ���֮��Ŀ��ַ������ȥ
	}
	else
	{
		strcpy(data + 2 + filenamelen + 1, "octet");
	}
	return sendto(sock, data, sizeof(data), 0, (LPSOCKADDR)&addr, sizeof(addr));
}

//����ACK
int sendACK(SOCKET& sock, SOCKADDR_IN& addr, int ack)
{
	char data[4];
	memset(data, 0, sizeof(data));
	data[1] = OP_ACK;
	//data[3] = ack;
	auto n = htons(ack);
	memcpy(data + 2, &n, 2);
	return sendto(sock, data, sizeof(data), 0, (LPSOCKADDR)&addr, sizeof(addr));
}

//�������ݿ�
int revDATA(SOCKET& sock, SOCKADDR_IN& addr, int & file_size,std::string & filename)
{
	using std::cout;
	using std::endl;
	std::ofstream fout(filename, std::ios_base::out | std::ios_base::binary);
	char data[516];		//ǰ���ֽ�Ϊ������3������ֽ�Ϊ���ݿ��ţ����512�ֽ�Ϊ���ݿ�
	char outputbuf[512];
	int size_of_addr = sizeof(addr);
	int rounds=1;			//���ݿ���
	int rev_outcome;
	unsigned short n;
	while (1)
	{

		//��������
		rev_outcome = recvfrom(sock, data, sizeof(data), 0, (LPSOCKADDR)&addr, &size_of_addr);
		if (rev_outcome == SOCKET_ERROR)
		{
			//����ʱ
			if (WSAGetLastError() == 10060)
			{
				cout << "���յ�" << rounds << "�����ݿ鳬ʱ��" << endl;
				//����һ���ݿ�ͳ�ʱ���ش�ACK�����ش�RRQ
				//�ӵڶ����ݿ鿪ʼ������ʱ����Ϊ��һ��ACK��ʧ���ش�ACK=rounds-1
				if (rounds > 1)
				{
					//�ش�ACK
					if (sendACK(sock, addr, rounds-1) == SOCKET_ERROR)
					{
						cout << "�ش�ACK=" << rounds - 1<< "ʧ�ܣ�" << endl;
						return 1;
					}
					else
					{
						cout << "�ش�ACK=" << rounds - 1 << "�ɹ���" << endl;
						//�ش�ACK��ֱ�ӻص�ѭ����ͷ��׼�����½��յ�rounds�����ݿ�
						continue;
					}
				}
				else
				{
					//rounds=1�������ط�RRQ����
					return RETRANS_RRQ;
				}
			}
			else
			{
				cout << "���յ�" << rounds << "�����ݿ�ʧ�ܣ�" << endl;
				return 1;
			}
		}
		memcpy(&n, data + 2, 2);		//ȡ���������ݿ�����ֶ�
		if ((data[1] == OP_DATA) && (htons(n) == unsigned short(rounds)))
		{
			file_size += rev_outcome;		//�յ��ı�����
			cout << "���յ���" << rounds << "�����ݿ飡" << endl;
			memcpy(outputbuf, data + 4, rev_outcome-4);
			fout.write(outputbuf, rev_outcome - 4);
		}
		else if ((data[1] == OP_DATA) && (htons(n) != unsigned short(rounds)))
		{
			//���յ��˷������ط������ݿ�
			cout << "���յ��ط��ĵ�" << htons(n) << "�����ݿ飡" << endl;
			cout << "�ط�ACK" << htons(n) << endl;
			rounds = htons(n);
		}
		else if (data[1] == OP_ERROR)
		{
			cout << "���յ�ERROR���ģ�������Ϊ";
			cout << htons(n) << endl;
			return 1;
		}
		else
		{
			//û���յ���Ҫ�ı��ģ���������
			continue;
		}

		//����ACK
		if (sendACK(sock, addr, rounds) == SOCKET_ERROR)
		{
			cout << "����ACK=" << rounds << "ʧ�ܣ�" << endl;
			return 1;
		}
		else
		{
			cout << "����ACK=" << rounds << "�ɹ���" << endl;
		}

		//�ж����ݿ��С�Ƿ���512�ֽ�
		if (rev_outcome == 516)
		{
			rounds++;
		}
		else
		{
			break;
		}
	}
	return 0;
}
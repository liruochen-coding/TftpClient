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



//常量
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

//函数原型
int Upload(char* ip,int port,int mode);
int sendWRQ(SOCKET & sock,SOCKADDR_IN & addr,char* filename,int filenamelen,int mode);
int revACK(SOCKET& sock, SOCKADDR_IN& addr, int expected_ack);
int sendData(SOCKET& sock, SOCKADDR_IN& addr, char* sendbuf, int length);
int Download(char* ip, int port, int mode);
int sendRRQ(SOCKET& sock, SOCKADDR_IN& addr, char* filename, int filenamelen, int mode);
int sendACK(SOCKET& sock, SOCKADDR_IN& addr, int ack);
int revDATA(SOCKET& sock, SOCKADDR_IN& addr, int& file_size,std::string & filename);

//定义日志
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
	//初始化winsock
	WSADATA wsadata;
	int nRc = WSAStartup(0x0101, &wsadata);
	if (nRc) {
		cout << "Winsock初始化失败！" << endl;
		system("pause");
		return 0;
	}
	int result;
	//打印界面
	while (1)
	{
		system("cls");
		cout << "***********************************TFTP客户端***********************************" << endl << endl;
		cout << "*******************************输入对应数字选择功能*****************************" << endl << endl;
		cout << "************0.退出" << endl;
		cout << "************1.上传（netascii）" << endl;
		cout << "************2.下载（netascii）" << endl;
		cout << "************3.上传（octet）" << endl;
		cout << "************4.下载（octet）" << endl;
		cout << "************5.显示日志" << endl;
		cout << "************6.保存日志" << endl;
		int func;
		cin >> func;
		switch (func)
		{
			case 0:
			{
				//退出程序
				return 0;
			}
			case 1:
			{
				//上传
				system("cls");
				cout << "*****上传（netascii）*****" << endl;
				char ip[] = "127.0.0.1";
				result = Upload(ip, 69, NETASCII);
				//test();
				cout << endl;
				system("pause");
				break;
			}
			case 2:
			{
				//下载
				system("cls");
				cout << "*****下载（netascii）*****" << endl;
				char ip[] = "127.0.0.1";
				result = Download(ip, 69, NETASCII);
				cout << endl;
				system("pause");
				break;
			}
			case 3:
			{
				//上传
				system("cls");
				cout << "*****上传（octet）*****" << endl;
				char ip[] = "127.0.0.1";
				result = Upload(ip, 69, OCTET);
				cout << endl;
				system("pause");
				break;
			}
			case 4:
			{
				//下载
				system("cls");
				cout << "*****下载（octet）*****" << endl;
				char ip[] = "127.0.0.1";
				result = Download(ip, 69, OCTET);
				cout << endl;
				system("pause");
				break;
			}
			case 5:
			{
				//打印日志
				system("cls");
				cout << "*****日志*****" << endl;
				int n=1;
				for (struct log x : Log)
				{
					cout << n << "：" << endl;;
					cout << "  时间：" << x.t.tm_year + 1900 << "年" << x.t.tm_mon + 1 << "月" << x.t.tm_mday << "日" << x.t.tm_hour << "时" << x.t.tm_min << "分" << x.t.tm_sec << "秒" << endl;
					cout << "  操作：" << x.action << endl<<endl;
					n++;
				}
				cout << endl;
				system("pause");
				break;
			}
			case 6:
			{
				//保存日志
				system("cls");
				cout << "*****保存日志*****" << endl << endl << endl;
				std::ofstream fout("tftpclient_log.txt", std::ios_base::out | std::ios_base::app);
				int n=1;
				fout << endl << endl << endl << endl << endl;
				for (struct log x : Log)
				{
					fout << n << "：" << endl;;
					fout << "  时间：" << x.t.tm_year + 1900 << "年" << x.t.tm_mon + 1 << "月" << x.t.tm_mday << "日" << x.t.tm_hour << "时" << x.t.tm_min << "分" << x.t.tm_sec << "秒" << endl;
					fout << "  操作：" << x.action << endl << endl;
					n++;
				}
				cout << "*****保存日志成功！*****" << endl;
				system("pause");
				break;
			}
			default:
			{
				//处理不合法的输入
				cout << "输入不合法！" << endl;
				cout << endl;
				system("pause");
				continue;
			}
		}
	}
}

//上传至服务器
int Upload(char* ip,int port,int mode)
{
	using std::cout;
	using std::endl;
	using std::cin;

	//准备日志
	auto t = time(nullptr);
	auto now = localtime(&t);
	struct log newlog;
	newlog.t = *now;
	std::string& log_content = newlog.action;
	std::ostringstream logstr;
	logstr << "上传";
	if (mode == NETASCII)
		logstr << "    模式：netascii    ";
	else
		logstr << "    模式：octet    ";

	//准备目的地址数据结构
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	//创建套接字
	SOCKET sServSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sServSock == INVALID_SOCKET)
	{
		cout << "*套接字创建失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError()<<endl;
		logstr << "结果：失败    原因：套接字创建失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		return 1;
	}
	else
	{
		cout << "*套接字创建成功！" << endl;
	}

	//设置阻塞模式
	unsigned long socket_mode = 0;		//0阻塞，1非阻塞
	if (ioctlsocket(sServSock, FIONBIO, &socket_mode) == SOCKET_ERROR)
	{
		cout << "*套接字设置阻塞模式失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：套接字设置阻塞模式失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*套接字设置阻塞模式成功！" << endl;
	}
	
	//设置超时时间
	timeval tv{ OVERTIME_SEC,OVERTIME_USEC };
	if (setsockopt(sServSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(timeval)) == SOCKET_ERROR)
	{
		cout << "*recvfrom设置超时失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：recvfrom设置超时失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*recvfrom设置超时成功！" << endl;
	}
	

	//绑定本机地址
	SOCKADDR_IN addr_local;
	addr_local.sin_family = AF_INET;
	addr_local.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr_local.sin_port = htons(LOCAL_PORT);
	if (bind(sServSock, (sockaddr*)&addr_local, sizeof(addr_local)) == SOCKET_ERROR)
	{
		cout << "*绑定主机地址失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：绑定主机地址失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*绑定主机地址成功！" << endl;
	}

	//读取文件内容
	std::string name;
	cout << "输入要上传的文件名路径：";
	cin >> name;
	std::ifstream fin(name, std::ios_base::in | std::ios_base::binary);
	struct stat statbuf;
	stat(name.c_str(), &statbuf);
	int length_of_sendbuf = statbuf.st_size;
	std::unique_ptr<char[]> sendbuf(new char[statbuf.st_size]);		//使用智能指针
	cout << length_of_sendbuf << endl;
	cout << statbuf.st_size << endl;
	fin.read(sendbuf.get(), length_of_sendbuf);


	//计算存放在服务器的文件名
	std::ostringstream filenamestr;
	filenamestr << (*now).tm_mday << (*now).tm_hour << (*now).tm_min << (*now).tm_sec << "_upload";
	char filename[40];
	strcpy(filename, filenamestr.str().c_str());
	logstr << "文件名：" << filenamestr.str() << "    ";
	cout << "*上传文件" << filenamestr.str() << endl;

	//启动计时器
	auto start = clock();

	//发送WRQ报文
	int send_outcome = sendWRQ(sServSock, addr, filename, strlen(filename), mode);
	if (send_outcome == SOCKET_ERROR)
	{
		cout << "*WRQ报文发送失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：WRQ报文发送失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*WRQ报文发送成功！" << endl;
	}

	//接受ACK=0
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
			//重传WRQ
			cout << "*接受ACK=0超时！" << endl;
			cout << "*重传WRQ报文" << endl;
			send_outcome = sendWRQ(sServSock, addr, filename, strlen(filename), mode);
		}
		else if (outcome==1)
		{
			cout << "*接受ACK=0失败！" << endl;
			cout << "*最近一次的错误代码：";
			cout << WSAGetLastError() << endl;
			logstr << "结果：失败    原因：接受ACK=0失败";
			log_content = logstr.str();
			Log.push_back(newlog);
			closesocket(sServSock);
			return 1;
		}
		else
		{
			cout << "*成功接受ACK=0！" << endl;
			break;
		}
	}

	//发送数据块
	if (sendData(sServSock, addr, sendbuf.get(), length_of_sendbuf) == 0)
	{
		cout << "*数据发送成功！" << endl;
	}
	else
	{
		cout << "*数据发送失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：数据发送失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}

	//结束计时器
	auto end = clock();
	double duration = double(end - start) / CLOCKS_PER_SEC;
	double speed = SEND_DATA_SIZE / 1024 / duration;
	cout << "**发送时间为：" << duration << "秒" << endl;
	cout << "**发送" << length_of_sendbuf << "比特数据" << endl;
	cout << "**发送速率为：" << speed << "KBytes/sec" << endl;
	logstr << "结果：成功    文件大小：" << length_of_sendbuf << "Bytes    速率：" << speed << "KB / s";
	log_content = logstr.str();
	Log.push_back(newlog);
	closesocket(sServSock);
	return 0;
}

//发送WRQ
int sendWRQ(SOCKET & sock, SOCKADDR_IN & addr,char* filename,int filenamelen,int mode)
{
	//初始化报文缓冲区
	char data[50];
	memset(data, 0, sizeof(data));
	//前两字节为操作码，2代表WRQ
	data[1] = OP_WRQ;
	//之后为文件名
	strcpy(data + 2, filename);
	//之后为传输模式
	if (mode == NETASCII)
	{
		strcpy(data + 2 + filenamelen+1, "netascii");				//要把文件名之后的空字符计算进去
	}
	else
	{
		strcpy(data + 2 + filenamelen + 1, "octet");
	}
	return sendto(sock, data, sizeof(data), 0, (LPSOCKADDR)&addr, sizeof(addr));
}

//接受ACK
int revACK(SOCKET & sock,SOCKADDR_IN & addr,int expected_ack)
{
	using std::cout;
	using std::endl;
	char data[40];				//ACK报文4字节，为ERROR和收到的其余报文留出空间
	int sizeof_addr = sizeof(addr);
	int outcome = recvfrom(sock, data, sizeof(data), 0, (LPSOCKADDR)&addr, &sizeof_addr);
	if (outcome == SOCKET_ERROR)
	{
		return 1;
	}
	unsigned short n;
	memcpy(&n, data + 2, 2);		//取报文中ack字段
	if ((data[1] == OP_ACK)&&(ntohs(n) == unsigned short(expected_ack)))

	{
		return 0;
	}
	else if (data[1] == OP_ERROR)
	{
		//收到ERROR报文
		cout << "收到ERROR报文，错误码为";
		cout << int(data[3]) << endl;
		return 1;
	}
	else
	{
		//收到了非ACK和ERROR报文，继续接受后续报文
		return UNWANTED_MESSAGE;
	}
}

//发送数据并接受ACK
int sendData(SOCKET& sock, SOCKADDR_IN& addr,char* sendbuf,int length)
{
	using std::cout;
	using std::endl;
	char data[516];		//前二字节为操作码3，后二字节为数据块编号，最后512字节为数据块
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
		//发送数据
		num = htons(i);
		memcpy(data + 2, &num, 2);
		//data[3] = i;
		if (i == rounds)
		{
			//最后一轮
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
			cout << "发送第" << i << "个数据块失败!" << endl;
			return 1;
		}
		else
		{
			cout<< "发送第" << i << "个数据块成功!" << endl;
		}

		//接受ACK=i
		rev_outcome = revACK(sock, addr, i);
		//出错
		while ((rev_outcome == UNWANTED_MESSAGE) || (rev_outcome == 1))
		{
			if (rev_outcome == UNWANTED_MESSAGE)
			{
				rev_outcome = revACK(sock, addr, i);
				continue;
			}
			else if ((i == rounds) && (WSAGetLastError() == 10054))
			{
				//最后一个ACK接收超时，并且连接被服务器强行关闭
				cout << "接收最后一个（" << i << "）ACK超时，连接被服务器强行关闭，服务器可能已经接收到了最后一个数据块！" << endl;
				break;
			}
			else if (WSAGetLastError() == 10060)
			{
				//超时
				cout << "接收第" << i << "个数据块的ACK超时！" << endl;
				//计数器减一，重传数据
				i--;
				break;
			}
			else
			{
				cout << "接收第" << i << "个数据块的ACK失败！" << endl;
				return 1;
			}
		}
		//接收到对应ACK
		if (rev_outcome == 0)
		{
			cout << "接收到对第" << i << "个数据块的ACK！" << endl;
		}
	}
	return 0;
	
}

//从服务器下载
int Download(char* ip, int port, int mode)
{
	using std::cout;
	using std::endl;
	using std::cin;

	//准备日志
	auto t = time(nullptr);
	auto now = localtime(&t);
	struct log newlog;
	newlog.t = *now;
	std::string& log_content = newlog.action;
	std::ostringstream logstr;
	logstr << "下载    ";
	if (mode == NETASCII)
		logstr << "模式：netascii    ";
	else
		logstr << "模式：octet    ";

	//准备服务器地址数据结构
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	//创建套接字
	SOCKET sServSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sServSock == INVALID_SOCKET)
	{
		cout << "*套接字创建失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：套接字创建失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		return 1;
	}
	else
	{
		cout << "*套接字创建成功！" << endl;
	}

	//设置阻塞模式
	unsigned long socket_mode = 0;		//0阻塞，1非阻塞
	if (ioctlsocket(sServSock, FIONBIO, &socket_mode) == SOCKET_ERROR)
	{
		cout << "*套接字设置阻塞模式失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：套接字设置阻塞模式失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*套接字设置阻塞模式成功！" << endl;
	}

	//设置超时时间
	timeval tv{ OVERTIME_SEC,OVERTIME_USEC };
	if (setsockopt(sServSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(timeval)) == SOCKET_ERROR)
	{
		cout << "*recvfrom设置超时失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：recvfrom设置超时失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*recvfrom设置超时成功！" << endl;
	}
	

	//绑定本机地址
	SOCKADDR_IN addr_local;
	addr_local.sin_family = AF_INET;
	addr_local.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr_local.sin_port = htons(LOCAL_PORT);
	if (bind(sServSock, (sockaddr*)&addr_local, sizeof(addr_local)) == SOCKET_ERROR)
	{
		cout << "*绑定主机地址失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：绑定主机地址失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout << "*绑定主机地址成功！" << endl;
	}

	std::string filenamestr;
	cout << "输入要下载的文件名：";
	cin >> filenamestr;
	char filename[20];
	strcpy(filename, filenamestr.c_str());
	logstr << "文件名：" << filenamestr << "    ";
	cout << "*下载文件" << filenamestr << endl;

	//启动计时器
	auto start = clock();

	//发送RRQ报文
	if (sendRRQ(sServSock, addr, filename, strlen(filename), mode) == SOCKET_ERROR)
	{
		cout << "*发送RRQ报文失败！" << endl;
		cout << "*最近一次的错误代码：";
		cout << WSAGetLastError() << endl;
		logstr << "结果：失败    原因：发送RRQ报文失败";
		log_content = logstr.str();
		Log.push_back(newlog);
		closesocket(sServSock);
		return 1;
	}
	else
	{
		cout<< "*发送RRQ报文成功！" << endl;
	}

	//接受DATA=i，并发送ACK=i
	int file_size = 0;
	int rev_outcome = revDATA(sServSock, addr, file_size, filenamestr);
	while (rev_outcome != 0)
	{
		if (rev_outcome == RETRANS_RRQ)
		{
			//重发RRQ
			if (sendRRQ(sServSock, addr, filename, strlen(filename), mode) == SOCKET_ERROR)
			{
				cout << "*重发RRQ报文失败！" << endl;
				cout << "*最近一次的错误代码：";
				cout << WSAGetLastError() << endl;
				logstr << "结果：失败    原因：重发RRQ报文失败";
				log_content = logstr.str();
				Log.push_back(newlog);
				closesocket(sServSock);
				return 1;
			}
			else
			{
				cout << "*重发RRQ报文成功！" << endl;
				rev_outcome = revDATA(sServSock, addr, file_size, filenamestr);
				continue;
			}
		}
		else
		{
			cout << "*接收数据失败！" << endl;
			cout << "*最近一次的错误代码：";
			cout << WSAGetLastError() << endl;
			logstr << "结果：失败    原因：接收数据失败";
			log_content = logstr.str();
			Log.push_back(newlog);
			closesocket(sServSock);
			return 1;
		}
	}
	//outcome==0,接收成功
	//停止计时器
	auto end = clock();
	double duration = double(end - start) / CLOCKS_PER_SEC;
	double speed = file_size / 1024 / duration;

	cout << "*接收数据成功！" << endl;
	cout << "**接收到的比特数：" << file_size << "Bytes" << endl;
	cout << "**传输速率：" << speed << "KB/s" << endl;
	logstr << "结果：成功    文件大小：" << file_size << "Bytes    传输速率：" << speed << "KB/s";
	log_content = logstr.str();
	Log.push_back(newlog);
	closesocket(sServSock);

	return 0;
}

//RRQ
int sendRRQ(SOCKET& sock, SOCKADDR_IN& addr, char* filename, int filenamelen, int mode)
{
	//初始化报文缓冲区
	char data[50];
	memset(data, 0, sizeof(data));
	//前两字节为操作码，2代表WRQ
	data[1] = OP_RRQ;
	//之后为文件名
	strcpy(data + 2, filename);
	//之后为传输模式
	if (mode == NETASCII)
	{
		strcpy(data + 2 + filenamelen + 1, "netascii");				//要把文件名之后的空字符计算进去
	}
	else
	{
		strcpy(data + 2 + filenamelen + 1, "octet");
	}
	return sendto(sock, data, sizeof(data), 0, (LPSOCKADDR)&addr, sizeof(addr));
}

//发送ACK
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

//接收数据块
int revDATA(SOCKET& sock, SOCKADDR_IN& addr, int & file_size,std::string & filename)
{
	using std::cout;
	using std::endl;
	std::ofstream fout(filename, std::ios_base::out | std::ios_base::binary);
	char data[516];		//前二字节为操作码3，后二字节为数据块编号，最后512字节为数据块
	char outputbuf[512];
	int size_of_addr = sizeof(addr);
	int rounds=1;			//数据块编号
	int rev_outcome;
	unsigned short n;
	while (1)
	{

		//接收数据
		rev_outcome = recvfrom(sock, data, sizeof(data), 0, (LPSOCKADDR)&addr, &size_of_addr);
		if (rev_outcome == SOCKET_ERROR)
		{
			//处理超时
			if (WSAGetLastError() == 10060)
			{
				cout << "接收第" << rounds << "个数据块超时！" << endl;
				//若第一数据块就超时则不重传ACK，需重传RRQ
				//从第二数据块开始，若超时就认为上一个ACK丢失，重传ACK=rounds-1
				if (rounds > 1)
				{
					//重传ACK
					if (sendACK(sock, addr, rounds-1) == SOCKET_ERROR)
					{
						cout << "重传ACK=" << rounds - 1<< "失败！" << endl;
						return 1;
					}
					else
					{
						cout << "重传ACK=" << rounds - 1 << "成功！" << endl;
						//重传ACK后，直接回到循坏开头，准备重新接收第rounds个数据块
						continue;
					}
				}
				else
				{
					//rounds=1，返回重发RRQ报文
					return RETRANS_RRQ;
				}
			}
			else
			{
				cout << "接收第" << rounds << "个数据块失败！" << endl;
				return 1;
			}
		}
		memcpy(&n, data + 2, 2);		//取报文中数据块序号字段
		if ((data[1] == OP_DATA) && (htons(n) == unsigned short(rounds)))
		{
			file_size += rev_outcome;		//收到的比特数
			cout << "接收到第" << rounds << "个数据块！" << endl;
			memcpy(outputbuf, data + 4, rev_outcome-4);
			fout.write(outputbuf, rev_outcome - 4);
		}
		else if ((data[1] == OP_DATA) && (htons(n) != unsigned short(rounds)))
		{
			//接收到了服务器重发的数据块
			cout << "接收到重发的第" << htons(n) << "个数据块！" << endl;
			cout << "重发ACK" << htons(n) << endl;
			rounds = htons(n);
		}
		else if (data[1] == OP_ERROR)
		{
			cout << "接收到ERROR报文，错误码为";
			cout << htons(n) << endl;
			return 1;
		}
		else
		{
			//没有收到想要的报文，继续接收
			continue;
		}

		//发送ACK
		if (sendACK(sock, addr, rounds) == SOCKET_ERROR)
		{
			cout << "发送ACK=" << rounds << "失败！" << endl;
			return 1;
		}
		else
		{
			cout << "发送ACK=" << rounds << "成功！" << endl;
		}

		//判断数据块大小是否不足512字节
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
#define _SCL_SECURE_NO_WARNINGS
#include <cstdio>
#include <windows.h>
#include <conio.h>
#include <ctime>
#include <iostream>
#include <string>
#include <cstdlib>

#define BUFFER_SIZE 1000
#define CONSTANT_TIMEOUT 1000
#define STR_BUFFER_SIZE 20

using namespace std;

void Server(char* path);
void Client();

int main(int argc, char* argv[])
{
	switch (argc)
	{
	case 1:
		Server(argv[0]);
		break;
	default:
		Client();
		break;
	}
}

void Server(char* path)
{
	string portName = "COM1";

	HANDLE COM1port;
	HANDLE SemaphoreSet[3];

	string write_str;
	char buffer[STR_BUFFER_SIZE];
	
	SemaphoreSet[0] = CreateSemaphore(
		NULL,// ������� �������
		0,// ������������������ ��������� ��������� ��������
		1,// ������������ ���������� ���������
		"READ"// ��� �������
	);
	SemaphoreSet[1] = CreateSemaphore(NULL, 0, 1, "WRITE");
	SemaphoreSet[2] = CreateSemaphore(NULL, 0, 1, "EXIT");

	cout << "Server start working (COM1)\n";

	COM1port = CreateFile(
		(LPSTR)portName.c_str(),//��� �����
		GENERIC_READ | GENERIC_WRITE,//��� ������ � ������
		0,//���������� ������
		NULL,//�������� ������
		OPEN_EXISTING,//������� ������������
		FILE_ATTRIBUTE_NORMAL,
		NULL//���������� ������� �����
	);

	if (COM1port == INVALID_HANDLE_VALUE) {
		cout << "Error during opening COM1" << endl;
		return;
	}

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(
		(LPSTR)path,//���� ��� �������� ������ ��������
		(LPSTR)" Client",//��� ��������
		NULL,// �������� ������ ��� ������ ��������
		NULL,//�������� ������ ��� ���������� ������
		FALSE,//���� ������������
		CREATE_NEW_CONSOLE,//������� �������� � ����� ������� 
		NULL,//���������� ���������
		NULL,//������� ���� � �������.
		&si,
		&pi
	)) {
		CloseHandle(COM1port);
		cout << "Error during creating client process" << endl;
		return;
	}

	SetCommMask(COM1port, EV_RXCHAR);//����������� ������������� �������(������ ��� ������ � ������� � ����� ����� ������)
	SetupComm(COM1port, BUFFER_SIZE, BUFFER_SIZE);//���������������� ���-��(������ ������� �����/������)

	DCB dcb;//��������� ����������
	memset(&dcb, 0, sizeof(dcb));//��������� ������ ��� dcb
	dcb.DCBlength = sizeof(DCB);//����� ��������� � ������
	if (!GetCommState(COM1port, &dcb)) {
		CloseHandle(COM1port);
		cout << "Error during DCB initialize" << endl;
		return;
	}

	dcb.BaudRate = CBR_14400;//�������� �������� ������ � �����
	dcb.fBinary = TRUE;//�������� �����
	dcb.fParity = FALSE;//����� ��������
	dcb.fOutxCtsFlow = FALSE;// ����������� ������ ���������� � ������ (CTS)
	dcb.fOutxDsrFlow = FALSE;// ����������� ������ ���������� ������ (DSR)
	dcb.fDtrControl = DTR_CONTROL_DISABLE;//������  DTR (���������� ��������� � �������� ������)
	dcb.fInX = FALSE;//������������ �� XON/XOFF ���������� ������� ������ � ���� ������
	dcb.fOutX = FALSE;//������������ �� XON/XOFF ���������� ������� ������ � ���� ��������	
	dcb.fNull = FALSE;// ��� ������ ������ ����� �� ������������
	dcb.fRtsControl = RTS_CONTROL_DISABLE;//������ RTS ( ���������� � ��������) 
	dcb.fAbortOnError = TRUE;//������� ��������� ��� �������� ������ � ������ � ���������� ������, ���� ���������� ������
	dcb.XonLim = 255;//����������� ����� ������, ������� ��������� � ������ ����� ������
	dcb.XoffLim = 255;//������������ ����� ������, ���������� � ������ ����� ������
	dcb.ByteSize = 8; //����� �������������� ��� � ������������ � ����������� ������.
	dcb.Parity = NOPARITY;//��� �������� ��������
	dcb.StopBits = ONESTOPBIT;//1 �������� ���	
	dcb.XonChar = 0; // �������� XON 
	dcb.XoffChar = 255;// �������� XOFF

	if (!SetCommState(COM1port, &dcb)) {//��������� ������������
		CloseHandle(COM1port);
		cout << "Error during setting configuration" << endl;
		return;
	}

	COMMTIMEOUTS timeouts;//��������� ��� ��������� ���������� �������
	timeouts.ReadIntervalTimeout = 0xFFFFFFFF;// M����������� �������� ������� ����� ������������ ���� �������� �� ����� �����
	timeouts.ReadTotalTimeoutMultiplier = 0;//���������, ������������, ����� ��������� ������ ������ ������� ������� ��� �������� ������
	timeouts.ReadTotalTimeoutConstant = CONSTANT_TIMEOUT;//���������, ������������, ����� ��������� ������ ������ ������� ������� ��� �������� ������
	timeouts.WriteTotalTimeoutMultiplier = 0;// ���������, ������������, ����� ��������� ������ ������ ������� ������� ��� �������� ������
	timeouts.WriteTotalTimeoutConstant = CONSTANT_TIMEOUT;// ���������, ������������, ����� ��������� ������ ������ ������� ������� ��� �������� ������

	if (!SetCommTimeouts(COM1port, &timeouts)) {//��������� ���������� �������
		CloseHandle(COM1port);
		cout << "Error during setting timeouts" << endl;
		return;
	}

	while (true)
	{
		DWORD writtenBytes = 0;

		cout << "Enter string to pass: ";
		cin.clear();
		getline(cin, write_str);

		if (write_str == "q") {
			ReleaseSemaphore(SemaphoreSet[2], 1, NULL);
			WaitForSingleObject(pi.hProcess, INFINITE);
			break;
		}

		ReleaseSemaphore(SemaphoreSet[0], 1, NULL);//������������ ��������

		int portionNumber = write_str.size() / STR_BUFFER_SIZE + 1;//������� ��� ����� ������������ ������ �� ������
		//���������� 2 �����
		if (!WriteFile(COM1port, &portionNumber, sizeof(portionNumber), &writtenBytes, NULL)) {
			cout << "Error during writing number of data portions" << endl;
			break;
		}

		int size = write_str.size();
		//���������� 2 ����� ������ �������� ������
		if (!WriteFile(COM1port, &size, sizeof(size), &writtenBytes, NULL)) {
			cout << "Error during writing entered string size" << endl;
			break;
		}

		for (int i = 0; i < portionNumber; i++) {
			write_str.copy(buffer, STR_BUFFER_SIZE, i * STR_BUFFER_SIZE);//�������� � ����� ����� �������� ������
			if (!WriteFile(COM1port, buffer, STR_BUFFER_SIZE, &writtenBytes, NULL)) {
				cout << "Error during writing in COM1";
				CloseHandle(COM1port);
				CloseHandle(SemaphoreSet[0]);
				CloseHandle(SemaphoreSet[1]);
			}
		}

		WaitForSingleObject(SemaphoreSet[1], INFINITE);//��� ������� �� ������
	}

	CloseHandle(COM1port);
	CloseHandle(SemaphoreSet[0]);
	CloseHandle(SemaphoreSet[1]);
	CloseHandle(SemaphoreSet[2]);

	return;
}

void Client()
{
	string portName = "COM2";

	HANDLE COM2port;
	HANDLE SemaphoreSet[3];

	string read_str;
	char buffer[STR_BUFFER_SIZE];

	SemaphoreSet[0] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, "READ");
	SemaphoreSet[1] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, "WRITE");
	SemaphoreSet[2] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, "EXIT");

	cout << "Client start working(COM2)\n";

	COM2port = CreateFile(
		(LPSTR)portName.c_str(),//��� �����
		GENERIC_READ | GENERIC_WRITE,//��� ������ � ������
		0,//���������� ������
		NULL,//�������� ������
		OPEN_EXISTING,//������� ������������
		FILE_ATTRIBUTE_NORMAL,
		NULL//���������� ������� �����
	);

	while (true)
	{
		DWORD readBytes;
		read_str.clear();

		int semaphoreIndex = WaitForMultipleObjects(3, SemaphoreSet, FALSE, INFINITE);
		if (semaphoreIndex - WAIT_OBJECT_0 == 2)//������������ ��� 'q'
		{
			break;
		}

		int portionNumber;
		if (!ReadFile(COM2port, &portionNumber, sizeof(portionNumber), &readBytes, NULL)) {//��������� ���������� ������ ��� ������
			cout << "Error during reading number of data portions" << endl;
			break;
		}

		int size;
		if (!ReadFile(COM2port, &size, sizeof(size), &readBytes, NULL)) {//��������� ������ �������� ������
			cout << "Error during reading entered string size" << endl;
			break;
		}

		for (int i = 0; i < portionNumber; i++)
		{
			if (!ReadFile(COM2port, buffer, STR_BUFFER_SIZE, &readBytes, NULL)) {//��������� � ����� ����� �������� ������
				cout << "Error during reading from COM2" << endl;
				CloseHandle(COM2port);
				CloseHandle(SemaphoreSet[0]);
				CloseHandle(SemaphoreSet[1]);
				return;
			}

			read_str.append(buffer, STR_BUFFER_SIZE);//��������� ������
		}


		read_str.resize(size);//�������������� ������ �����

		cout << "COM2 get: " << endl;
		for (int i = 0; i < size; i++)
		{
			cout <<read_str[i];
		}
		cout << endl;
		
		ReleaseSemaphore(SemaphoreSet[1], 1, NULL);//������������ ��������, ������ ���������
	}

	CloseHandle(COM2port);
	CloseHandle(SemaphoreSet[0]);
	CloseHandle(SemaphoreSet[1]);
	CloseHandle(SemaphoreSet[2]);
	return;
}
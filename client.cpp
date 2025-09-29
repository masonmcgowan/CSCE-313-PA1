/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Mason McGowan
	UIN: 633007071
	Date: 9/27/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>

using namespace std;

int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m = 256;
	bool newChannel = false;
	vector<FIFORequestChannel*> channels;

	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'c':
				newChannel = true;
				break;
		}
	}

	//child process
	int child = fork();
	if(child == 0)
	{
		char* args[] = {(char*) "./server", (char*) "-m", (char*) to_string(m).c_str(), NULL};
		execvp("./server", args);
	}

    FIFORequestChannel controlChannel("control", FIFORequestChannel::CLIENT_SIDE);
	FIFORequestChannel* nChannel;
	channels.push_back(&controlChannel);

	if(newChannel)
	{
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
    	controlChannel.cwrite(&nc, sizeof(MESSAGE_TYPE));
		char* ncName = new char[30];
		controlChannel.cread(ncName, 30);
		nChannel = new FIFORequestChannel(ncName, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(nChannel);
		delete[] ncName;
	}
	
	FIFORequestChannel chan = *(channels.back());

	// example data point request
    if (p != -1 && t != -1 && e != -1)
	{
		char buf[MAX_MESSAGE]; // 256 bytes
    	datamsg x(p, t, e);
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg));
		double resp;
		chan.cread(&resp, sizeof(double));
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << resp << endl;
	}
	else if (p != -1)
	{
		ofstream ofs;
		string file = "received/x1.csv";
		ofs.open(file);

		double seconds = 0.0;
		double val1;
		double val2;
		char buf1[MAX_MESSAGE];
		char buf2[MAX_MESSAGE];

		for(int i = 1; i <= 1000; i++)
		{
			//ecg1 request
    		datamsg y(p, seconds, 1);
			memcpy(buf1, &y, sizeof(datamsg));
			chan.cwrite(buf1, sizeof(datamsg));
			chan.cread(&val1, sizeof(double));

			//ecg2 request
    		datamsg z(p, seconds, 2);
			memcpy(buf2, &z, sizeof(datamsg));
			chan.cwrite(buf2, sizeof(datamsg));
			chan.cread(&val2, sizeof(double));

			ofs << seconds << "," << val1 << "," << val2 << endl;
			seconds += .004;
		}
		ofs.close();
	}

	if(filename != "")
	{
		//getting size of file
		filemsg fm(0, 0);
	
		int len = sizeof(filemsg) + (filename.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), filename.c_str());
		chan.cwrite(buf2, len);
		int64_t filesize = 0;
		chan.cread(&filesize, sizeof(int64_t));
		cout << "File length is: " << filesize << " bytes" << endl;
	
		ofstream outputFile;
		outputFile.open("received/" + filename);

		char* respBuf = new char[m];
		int64_t offset = 0;
		int64_t len2 = m;

		while(filesize > 0)
		{
			if(filesize < m)
			{
				len2 = filesize;
			}
			filemsg fm2(offset, len2);
			memcpy(buf2, &fm2, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), filename.c_str());
			chan.cwrite(buf2, len);
			chan.cread(respBuf, len2);
			outputFile.write(respBuf, len2);
			offset += len2;
			filesize -= len2;
		}
		
		outputFile.close();
		delete[] respBuf;
		delete[] buf2;
	}
	
	if(newChannel)
	{
		MESSAGE_TYPE qm = QUIT_MSG;
		chan.cwrite(&qm, sizeof(MESSAGE_TYPE));
		channels.pop_back();
		delete nChannel;
		FIFORequestChannel chan = *(channels.back());
	}

	// closing the channel    
    MESSAGE_TYPE quitMsg = QUIT_MSG;
    chan.cwrite(&quitMsg, sizeof(MESSAGE_TYPE));
	wait(NULL);
}
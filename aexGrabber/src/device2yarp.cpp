// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * This class use the USB retina driver wrote by
 * Martin Ebner, IGI / TU Graz (ebner at igi.tugraz.at)
 *
 *  The term of the contract of the used source is :
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 * This driver is based on the 2.6.3 version of drivers/usb/usb-skeleton.c
 * (Copyright (C) 2001-2004 Greg Kroah-Hartman (greg@kroah.com))
 *
 */

#include <iCub/device2yarp.h>
//#include <sys/types.h>

using namespace std;
using namespace yarp::os;

#define timestep 10000 //10 ms OneSecond/100
#define OneSecond 1000000 //1sec
#define latchexpand 100
#define countBias 12
#define LATCH_KEEP 1
#define LATCH_TRANSPARENT 0

#define CLOCK_LO 0
#define CLOCK_HI 1



device2yarp::device2yarp(string portDeviceName, bool i_bool, string i_fileName):RateThread(10), save(i_bool) {
    len=0;
    sz=0;
    ec = 0;
    memset(buffer, 0, SIZE_OF_DATA);
    const u32 seqAllocChunk_b = 8192 * sizeof(struct aer); //allocating the right dimension for biases
    monBufSize_b = 8192 * sizeof(struct aer);

    pseq = (aer *) malloc(seqAllocChunk_b);
    if ( pseq == NULL ) {
        printf("pseq malloc failed \n");
    }
    memset(pseq,0,seqAllocChunk_b);
    seqAlloced_b = seqAllocChunk_b;
    pmon = (aer *)  malloc(monBufSize_b);
    if ( pmon == NULL ) {
        printf("pmon malloc failed \n");
    }
    memset(pmon,0,monBufSize_b);

    seqEvents = 0;
    seqSize_b = 0;
    int deviceNum=0;
    str_buf << "/icub/retina" << deviceNum << ":o";
    port.open(str_buf.str().c_str());

    cout <<"name of the file buffer:" <<portDeviceName.c_str()<< endl;
    file_desc = open(portDeviceName.c_str(), O_RDWR | O_NONBLOCK);
    if (file_desc < 0) {
        printf("Cannot open device file: %s \n",portDeviceName.c_str());
    }
    else {
        int err;
    }

    if(!strcmp(portDeviceName.c_str(),"/dev/aerfx2_0")) {
        printf("sending biases as events to the device ... \n");
        /*
        *    biasvalues = {
        *    "11" "cas": 1966, 7AE
        *    "10" "injGnd": 22703, 58AF
        *    "9" "reqPd": 16777215, FFFFFF
        *    "puX": 4368853, 42A9D5
        *    "diffOff": 3207, C87
        *    "req": 111347, 1B2F3
        *    "refr": 0, 0
        *    "puY": 16777215, FFFFFF
        *    "diffOn": 483231, 75F9F
        *    "diff": 28995, 7143
        *    "foll": 19, 13
        *    "Pr": 8, 8
        *}
        */

        int biasValues[]={1966,        // cas
                          22703,       // injGnd
                          16777215,    // reqPd
                          4368853,     // puX
                          3207,        // diffOff
                          111347,      // req
                          0,           // refr
                          16777215,    // puY
                          483231,      // diffOn
                          28995,       // diff 
                          19,          // foll
                          8            //Pr 
        };

        string biasNames[] = {
                            "cas",
                            "injGnd",
                            "reqPd",
                            "puX",
                            "diffOff",
                            "req",
                            "refr",
                            "puY",
                            "diffOn",
                            "diff",
                            "foll",
                            "Pr"
                            };

            
        //int err = write(file_desc,bias,41); //5+36 
        seqEvents = 0;
        seqSize_b = 0;
        for(int j=0;j<countBias;j++) {
            progBias(biasNames[j],24,biasValues[j]);
        }
        latchCommit();
        monitor(10);
        sendingBias();
    }
}

device2yarp::~device2yarp() {
    port.close();
}


void device2yarp::setDeviceName(string deviceName) {
    portDeviceName=deviceName;
}



void  device2yarp::run() {
    printf("reading from the device %s \n",portDeviceName.c_str());
    if(!strcmp(portDeviceName.c_str(),"/dev/aerfx2_0")) {
        // read address events from device file which is not /dev/aerfx2_0
        //sz = read(file_desc,buffer,SIZE_OF_DATA);
        sz = pread(file_desc,buffer,SIZE_OF_DATA,0);
        cout << "Size of the buffer : " << sz << endl;
        for(int i=0; i<sz; i+=4)
            {
                unsigned int part_1 = 0x00FF&buffer[i];
                unsigned int part_2 = 0x00FF&buffer[i+1];
                unsigned int part_3 = 0x00FF&buffer[i+2];
                unsigned int part_4 = 0x00FF&buffer[i+3];
                unsigned int blob = (part_1)|(part_2<<8);
                unsigned int timestamp = ((part_3)|(part_4<<8));
                //printf("%x : %x\n", blob, timestamp);
            }
        
    }
    else {    
        printf("reading /dev/aerfx2_0 ");
        int r = read(file_desc, pmon, monBufSize_b);
        monBufEvents = r / sizeof(struct aer);        
        ec += monBufEvents;
        int k=0;
        u32 a, t;
        for (int i = 0; i < monBufEvents; i++) {
            a = pmon[i].address;
            t = pmon[i].timestamp * 0.128;
            char c;
            c=(char)(a&0x000000FF);buffer[k]=c;
            long int part_1 = 0x000000FF&buffer[k];k++;  //extracting the 1 byte
            c=(char)((a&0x0000FF00)>>8); buffer[k]=c;
            long int part_2 = 0x000000FF&buffer[k];k++;  //extracting the 2 byte
            c=(char)((a&0x00FF0000)>>16); buffer[k]=c;
            long int part_3 = 0x000000FF&buffer[k];k++;  //extracting the 3 byte
            c=(char)((a&0xFF000000)>>24); buffer[k]=c;                 
            long int part_4 = 0x000000FF&buffer[k];k++;  //extracting the 4 byte
            long int blob = (part_1)|(part_2<<8)|(part_3<<16)|(part_4<<24);

            c=(char)(t&0x000000FF); buffer[k]=c;
            long int part_5 = 0x000000FF&buffer[k];k++;  //extracting the 1 byte
            c=(char)((t&0x0000FF00)>>8); buffer[k]=c;
            long int part_6 = 0x000000FF&buffer[k];k++;  //extracting the 2 byte
            c=(char)((t&0x00FF0000)>>16); buffer[k]=c;
            long int part_7 = 0x000000FF&buffer[k];k++;  //extracting the 3 byte
            c=(char)((t&0xFF000000)>>24); buffer[k]=c;                 
            long int part_8 = 0x000000FF&buffer[k];k++;  //extracting the 4 byte
            long int timestamp = ((part_5)|(part_6<<8)|(part_7<<16)|(part_8<<24));
            //printf("%d : %d\n", blob, timestamp)
        }
        sz=monBufEvents*32; //32bits(4*8bits) for every event
    }
    if(port.getOutputCount()) {
        sendingBuffer data2send(buffer, sz);
        sendingBuffer& tmp = port.prepare();
        tmp = data2send;
        port.write();
    }
    memset(buffer, 0, SIZE_OF_DATA);
}


void device2yarp::sendingBias() {
    printf("-------------------------------------------- \n");
    printf("trying to write to kernel driver %d %d \n", seqDone_b,seqSize_b);
    while (seqDone_b < seqSize_b) {      
        // try writing to kernel driver 
        printf( "calling write fd: sS: %d  sD: %d \n", seqSize_b, seqDone_b);
        int w = write(file_desc, seqDone_b + ((u8*)pseq), seqSize_b - seqDone_b);
        if (w > 0) {
           seqDone_b += w;
           printf("writing accumulator %d \n",seqDone_b);
        } else {
           printf("writing error \n");   
        }
    }
    printf("writing successfully ended \n");

    ////////////////////////////////////////////////////////////


    double TmaxSeqTimeEstimate = 
        seqTime * 0.128 * 1.10 +   // summed up seq intervals in us plus 10%
        seqEvents * 1.0           // plus one us per Event
    ;

    printf("seqEvents: %d \n", seqEvents);
    printf("seqTime * 0.128: %d \n", (u64)(seqTime * 0.128));
    //printf("TmaxSeqTimeEstima PRIu64  %f %f\n", 
    //  (TmaxSeqTimeEstimate / 1000000), (TmaxSeqTimeEstimate % 1000000));
}

void device2yarp::progBias(string name,int bits,int value) {
    int bitvalue;
    for (int i=bits-1;i>=0;i--) {
        int mask=1;
        for (int j=0; j<i; j++) {
            mask*=2;
        }
        printf("mask %d ",mask);
        if (mask & value) {
            bitvalue = 1;
        }
        else {
            bitvalue = 0;
        }
        //printf("---------------------------- %d \n",bitvalue);
        progBit(bitvalue);
    }
}

void device2yarp::latchCommit() {
    printf("entering latch_commit \n");
    biasprogtx(timestep * latchexpand, LATCH_TRANSPARENT, CLOCK_LO, 0);
    biasprogtx(timestep * latchexpand, LATCH_KEEP, CLOCK_LO, 0);
    biasprogtx(timestep * latchexpand, LATCH_KEEP, CLOCK_LO, 0);
    printf("exiting latch_commit \n");
}

void device2yarp::progBit(int bitvalue) {
    //set data
    biasprogtx(timestep, LATCH_KEEP, CLOCK_LO, bitvalue);
    //toggle clock
    biasprogtx(timestep, LATCH_KEEP, CLOCK_HI, bitvalue);
    biasprogtx(timestep, LATCH_KEEP, CLOCK_LO, bitvalue);
}

void device2yarp::monitor (int secs) {
    printf("entering monitor \n");
    biasprogtx(secs * OneSecond, LATCH_KEEP, CLOCK_LO, 0);
    printf("exiting monitor \n");
} 

void device2yarp::biasprogtx(int time,int latch,int clock,int data) {
    unsigned char addr[4];
    unsigned char t[4];
    int err;
    //setting the time
    //printf("biasProgramming following time %d \n",time);
    t[0]= time & 0xFF000000;
    t[1]= time & 0x00FF0000;
    t[2]= time & 0x0000FF00;
    t[3]= time & 0x000000FF;
   
    
    //setting the addr
    addr[0] = 0xFF;
    addr[1] = 0x00;
    addr[2] = 0x00;
    if(data) {
        addr[3] += 0x01;
    }
    if(clock) {
        addr[3] += 0x02;
    }
    if (latch) {
        addr[3] += 0x04;
    }
    //printf("data:0x%x, 0x%x, 0x%x, 0x%x \n",addr[0],addr[1],addr[2],addr[3]);
    
    
    //u32 seqSize_b = sizeof(struct aer);
    u32 timeToSend, addressToSend;
    timeToSend=time;
    addressToSend=0;
    if(data) {
        addressToSend += 0x01;
    }
    if(clock) {
        addressToSend += 0x02;
    }
    if (latch) {
        addressToSend += 0x04;
    }
    addressToSend+=0xFF000000;

    u32 hwival = (u32)(timeToSend * 7.8125);
    //printf("saving the aer.address %x \n", addressToSend);
    pseq[seqEvents].address = addressToSend;
    //printf("saving the aer.time  \n");
    pseq[seqEvents].timestamp = hwival;

    seqEvents++;
    printf("number of saved events %d as tot %d ",seqEvents,seqSize_b);
    seqTime += hwival;
    seqSize_b += sizeof(struct aer);
    

    //printf("writing the event");
    //int w = write(file_desc, ((u8*)pseq), seqSize_b);
    //addr = int(sys.argv[1], 16)
    //err = write(file_desc,t,4); //4 byte time: 1 integer
    //err = write(file_desc,addr,4); //4 byte time: 1 integer
}

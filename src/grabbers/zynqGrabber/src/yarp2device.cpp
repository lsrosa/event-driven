/*
 * Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
 * Author: chiara.bartolozzi@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

#include "iCub/yarp2device.h"
//#include <unistd.h>

/******************************************************************************/
//yarp2device
/******************************************************************************/
yarp2device::yarp2device()
{
    flagStart = false;
    countAEs = 0;
    writtenAEs = 0;
    //mask = 0x000FFFFF;
    mask = 0x000003FF;
}

bool yarp2device::open(std::string moduleName)
{
    //setFileDesc(devDesc);

    this->useCallback();
    this->setStrict();

    fprintf(stdout,"opening port for receiving the events from yarp \n");

    std::string inPortName = "/" + moduleName + "/vBottle:i";
    return yarp::os::BufferedPort< yarp::os::Bottle >::open(inPortName);

}

/******************************************************************************/
void yarp2device::close()
{
    std::cout << "Y2D: received " << countAEs << " events from yarp" << std::endl;

    std::cout << "Y2D: written " << writtenAEs << " events to device"
              << std::endl;
    yarp::os::BufferedPort<yarp::os::Bottle>::close();
}

/******************************************************************************/
void yarp2device::interrupt()
{
    yarp::os::BufferedPort<yarp::os::Bottle>::interrupt();
}

/******************************************************************************/

void yarp2device::setWritingMask(int mask)
{
    this->mask = mask;
    std::cout << "Mask is: " << this->mask << std::endl;
}

void yarp2device::onRead(yarp::os::Bottle &bot)
{

    yarp::os::Bottle *content = bot.get(1).asList();
    int nints = content->size();
    //const int * directaccess = (const int *)(content->toBinary());
    //directaccess++; //the first int is the size -> so skip it!!

    if(deviceData.size() < nints)
        deviceData.resize(nints, 0);

    countAEs += nints / 2;

    for(size_t i = 1; i < nints; i += 2) 
        deviceData[i] = content->get(i).asInt() & mask;

    //int datawritten = nints;
    int datawritten = devManager->writeDevice((unsigned char *)deviceData.data(), nints*sizeof(int));
    datawritten /= sizeof(int);

    if(datawritten <= 0)
        yError() << "Could not write to device";
    else {
        writtenAEs += datawritten / 2;
        if(datawritten != nints)
            yWarning() << "Did not write all data: " << datawritten << " / " << deviceData.size();
    }

    static double previous_time = yarp::os::Time::now();
    double dt = yarp::os::Time::now() - previous_time;
    if(dt > 3.0) {
        yInfo() << "[WRITE] " << 0.000032 * writtenAEs / dt << " mbps (" 
            << getPendingReads() << "). Example:" << deviceData[2] <<  deviceData[3];
        writtenAEs = 0;
        previous_time += dt;
    }

}

bool  yarp2device::attachDeviceManager(deviceManager* devManager) {

    this->devManager = dynamic_cast<aerDevManager*>(devManager);

    if (!devManager){
        return false;
    }

    clockScale = this->devManager->getUsToTick();
    return true;



}


//empty line to make gcc happy

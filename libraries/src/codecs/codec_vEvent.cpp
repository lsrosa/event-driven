#ifndef __VCODEC_VEVENT__
#define __VCODEC_VEVENT__

#include <algorithm>
#include <yarp/os/Bottle.h>
#include "iCub/eventdriven/vCodec.h"
#include "iCub/eventdriven/vtsHelper.h"

namespace ev {

vEvent::vEvent() : stamp(0) {}
vEvent::~vEvent() {}

event<> vEvent::clone()
{
    return std::make_shared<vEvent>(*this);
}

void vEvent::encode(yarp::os::Bottle &b) const
{
#ifdef TIME32BIT
    b.addInt(stamp&0x7FFFFFFF);
#else
    b.addInt((32<<26)|(stamp&0x00ffffff));
#endif
}

bool vEvent::decode(const yarp::os::Bottle &packet, int &pos)
{
    if(pos + 1 <= packet.size()) {

        //TODO: this needs to take into account the code aswell
#ifdef TIME32BIT
        stamp = packet.get(pos).asInt()&0x00ffffff;
#else
        stamp = packet.get(pos).asInt()&0x7FFFFFFF;
#endif
        pos += 1;
        return true;
    }
    return false;

}

yarp::os::Property vEvent::getContent() const
{
    yarp::os::Property prop;
    prop.put("type", getType().c_str());
    prop.put("stamp", (int)stamp);

    return prop;
}


std::string vEvent::getType() const
{
    return "TS";
}

int vEvent::getChannel() const
{
    return -1;
}

void vEvent::setChannel()
{

}


bool temporalSortStraight(const event<> &e1, const event<> &e2) {
    return e2->stamp > e1->stamp;
}

bool temporalSortWrap(const event<> &e1, const event<> &e2)
{

    if(std::abs(e1->stamp - e2->stamp) > vtsHelper::max_stamp/2)
        return e1->stamp > e2->stamp;
    else
        return e2->stamp > e1->stamp;

}

void qsort(vQueue &q, bool respectWraps)
{

    if(respectWraps)
        std::sort(q.begin(), q.end(), temporalSortWrap);
    else
        std::sort(q.begin(), q.end(), temporalSortStraight);

}

event<> createEvent(const std::string &type)
{
    vEvent * ret = nullptr;

    ret = new AddressEvent();
    if(type == ret->getType()) return event<>(ret);
    else delete(ret);

    ret = new FlowEvent();
    if(type == ret->getType()) return event<>(ret);
    else delete(ret);

    ret = new GaussianAE();
    if(type == ret->getType()) return event<>(ret);
    else delete(ret);

    ret = new LabelledAE();
    if(type == ret->getType()) return event<>(ret);
    else delete(ret);

//    ret = new AddressEventClustered();
//    if(type == ret->getType()) return event<>(ret);
//    else delete(ret);

//    ret = new ClusterEvent();
//    if(type == ret->getType()) return event<>(ret);
//    else delete(ret);

//    ret = new ClusterEventGauss();
//    if(type == ret->getType()) return event<>(ret);
//    else delete(ret);

//    ret = new CollisionEvent();
//    if(type == ret->getType()) return event<>(ret);
//    else delete(ret);

//    ret = new InterestEvent();
//    if(type == ret->getType()) return event<>(ret);
//    else delete(ret);

//    ret = new NeuronIDEvent();
//    if(type == ret->getType()) return event<>(ret);
//    else delete(ret);
//    ret = nullptr;

    return event<>(nullptr);

}


}



#endif

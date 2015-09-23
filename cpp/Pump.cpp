/*
 * Pump.cpp
 *
 *  Created on: Sep 14, 2015
 *      Author: lieven
 */
#include "Msg.h"
#include "Handler.h"

#include "all.h"
#include "Sys.h"

extern "C" {
#include "gpio16.h"
}
uint32_t __count = 0;
//Sender sender();
static Msg msg(256);
extern "C"  IROM void MsgPump() {
	while (msg.receive()) {
		msg.rewind();
		Handler::dispatchToChilds(msg);
		msg.free();
		if ((__count++) % 1000 == 0) {
			INFO( "Count loop : %d Committed size :%d ", __count,msg._bb->getCommittedSize());
		}
	}
}

extern "C"  IROM void MsgPublish(void* src, Signal signal) {
	Msg::publish(src, signal);
}







// C includes
#include <stdio.h>
#include <stdlib.h>

// C++ includes
#include <map>
#include <string>
#include <stdexcept>

// lib includes
#include <boost/shared_ptr.hpp>

#include "auto_rabbitmq.h"
#include "rss.pb.h"

using namespace std;
using namespace boost;

class MyConfigHandler: public EventHandler<OWConfig> {
public:
	// override
	virtual void handleEvent(OWConfig *event) {
		printf("holy cow! I got a config event!\n");
		printf("the component_uri is %s\n", event->component_uri().c_str());
	}
};

class MyStatusHandler: public EventHandler<OWStatus> {
public:
	// override
	virtual void handleEvent(OWStatus *event) {
		printf("holy crap!! I got a status event!!\n");
		printf("the component_uri is %s\n", event->component_uri().c_str());
	}
};

class MyReportHandler: public EventHandler<OWReport> {
public:
	// override
	virtual void handleEvent(OWReport *event) {
		event->PrintDebugString();
	}
};

int main(void) {

	auto_rabbitmq rmq("localhost", 5672);

	if (1) {
		// consumer
		rmq.eventHandler(shared_ptr<EventHandler<OWConfig> >(new MyConfigHandler()));
		rmq.eventHandler(shared_ptr<EventHandler<OWStatus> >(new MyStatusHandler()));
		rmq.eventHandler(shared_ptr<EventHandler<OWReport> >(new MyReportHandler()));
		rmq.dispatch(); // run the main loop! typically on a different thread...
	} else {
		// producer
		OWConfig config;
		config.set_component_uri("yikes! from c++ land!!");
		rmq.event(&config);
	}

	return 0;
}

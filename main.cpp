
// C includes
#include <stdio.h>
#include <stdlib.h>

#include <event2/event.h>

// C++ includes
#include <map>
#include <string>

// lib includes
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/xpath.h>

#include "rss.pb.h"

#include <amqp.h>

using namespace std;
using namespace boost;
using namespace myrss;

//int
//mainz(void) {
//	shared_ptr<xmlDoc> doc(xmlReadFile("http://feeds.nytimes.com/nyt/rss/HomePage", 0, 0), xmlFreeDoc);
//    if (doc==0)
//    	throw runtime_error(xmlGetLastError()->message);
//
//    RssFeed feed;
//    ProtoReadXml(&feed, doc.get());
//
//    // other examples:
//    // e.g., ProtoReadOWConfig
//    // e.g., ProtoWriteOWStatus
//    // e.g., ProtoReadWeb
//
//    printf("title=%s\n", feed.channel().title().c_str());
//    printf("link=%s\n", feed.channel().link(0).c_str());
//    printf("description=%s\n", feed.channel().description().c_str());
//    printf("language=%s\n", Language_Name(feed.channel().language()).c_str());
//
//    if (feed.channel().language()==en_US)
//    	printf("btw, that's english!\n\n");
//
//    // print each item title
//    for (int index = 0; index < feed.channel().item_size(); ++index)
//        printf("title=%s\n", feed.channel().item(index).title().c_str());
//}

template<class T>
class EventHandler {
public:
	// dtor
	virtual ~EventHandler(){}
	// override
	virtual void handleEvent(T *event)=0;
};

class EventBus {

	event_base *base;

	class Dispatcher {
	public:
		virtual ~Dispatcher(){}
		virtual void dispatch(string raw) = 0;
	};

	template<class T>
	class Dispatcher0: public Dispatcher {
		shared_ptr< EventHandler<T> > handler;
	public:
		// ctor
		Dispatcher0(shared_ptr< EventHandler<T> > handler): handler(handler) {
		}
		virtual void dispatch(string raw) {
			T event;
			// fill
			handler->handleEvent(&event);
		}
	};
	map< string, shared_ptr<Dispatcher> > dispatchers;
//	void handleEvent(Message *message) {
//		string name (message->GetDescriptor()->name())
//
//		dispatchers[message->GetTypeName()]->dispatch("zzz");
//	}
public:
	// ctor
	EventBus(event_base *base): base(base) {
	}

	// client api
	template<class CmdMsg>
	void postCmdMsg(CmdMsg *cmdMsg) {
		string name(cmdMsg->GetDescriptor()->name().c_str());
		printf("postCmdMsg: name=%s\n", name.c_str());
		dispatchers[name]->dispatch("zzz");
	}
	template<class EventMsg>
	void postEventMsg(EventMsg *eventMsg) {
		string name(eventMsg->GetDescriptor()->name().c_str());
		printf("postEventMsg: name=%s\n", name.c_str());
		dispatchers[name]->dispatch("zzz");
	}

	// handler api
	template<class EventMsg>
	void addHandler(shared_ptr< EventHandler<EventMsg> > handler) {
		string name(EventMsg::descriptor()->name().c_str());

		printf("addHandler: %s\n", EventMsg::descriptor()->name().c_str());

		EventMsg event;
		event.PrintDebugString();
		dispatchers[name]=shared_ptr<Dispatcher>(new Dispatcher0<EventMsg>(handler));
	}
};

class MyResetBladeHandler: public EventHandler<ResetBladeEventMsg> {
public:
	// override
	virtual void handleEvent(ResetBladeEventMsg *event) {
		printf("MyResetBladeHandler::handleEvent\n");
	}
};

static shared_ptr<event_base> base(event_base_new(), event_base_free);

int main(void) {
//	time_t now(time(0));
//	printf("main[1] %s", ctime(&now));

	printf("amqp_version=%s\n", amqp_version());

	EventBus eventBus(base.get());

	// add handler
	eventBus.addHandler(shared_ptr<EventHandler<ResetBladeEventMsg> >(new MyResetBladeHandler()));

	// post eventMsg
	ResetBladeEventMsg eventMsg;
	eventBus.postEventMsg(&eventMsg);

	return event_base_dispatch(base.get());
}

//Thanks for reply Alan.
//
//I modified the amqp_consumer.c sample code to simulate the non-blocking behaviour as you mentioned.
//
//    /* if (!amqp_frames_enqueued(conn) && !amqp_data_in_buffer(conn)) { */
//    if (1) {
//       int sock = amqp_get_sockfd(conn);
//       printf("socket: %d\n", sock);
//
//       /* Watch socket fd to see when it has input. */
//       fd_set read_flags;
//       FD_ZERO(&read_flags);
//       FD_SET(sock), &read_flags);
//       int ret = 0;
//       do {
//          struct timeval timeout;
//
//          /* Wait upto a second. */
//          timeout.tv_sec = 1;
//          timeout.tv_usec = 0;
//
//          ret = select(sock+1, &read_flags, NULL, NULL, &timeout);
//          if (ret == -1)
//             printf("select: %s\n", strerror(errno));
//          else if (ret == 0)
//             printf("select timedout\n");
//          if (FD_ISSET(sock, &read_flags)) {
//             printf("Flag is set\n");
//          }
//       } while (ret == 0);
//    }
//
//But this always results in a timeout. Any idea where I might be going wrong? I have commented the first two checks that you mentioned just for sake of clarity on select().
//
//-Arun
